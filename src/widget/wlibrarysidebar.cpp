#include "widget/wlibrarysidebar.h"

#include <QHeaderView>
#include <QUrl>
#include <QtDebug>

#include "library/sidebaritemdelegate.h"
#include "library/sidebarmodel.h"
#include "moc_wlibrarysidebar.cpp"
#include "util/defs.h"
#include "util/dnd.h"
#include "util/duration.h"

constexpr int expand_time = 250;
// Delay for header adjust requests. 50 ms seems to be a good compromise between
// fast GUI update and long enough wait time for potential follow-up events.
constexpr int resize_header_delay = 50;

namespace {

const QColor kDefaultBookmarkColor = QColor(Qt::red);

} // anonymous namespace

WLibrarySidebar::WLibrarySidebar(QWidget* parent)
        : QTreeView(parent),
          WBaseWidget(this),
          m_pSidebarModel(nullptr),
          m_pItemDelegate(nullptr),
          m_bookmarkColor(kDefaultBookmarkColor) {
    qRegisterMetaType<FocusWidget>("FocusWidget");
    //Set some properties
    setHeaderHidden(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    //Drag and drop setup
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setAcceptDrops(true);
    setAutoScroll(true);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    // Adjust header when an item's expand state has changed
    connect(this,
            &QTreeView::expanded,
            this,
            &WLibrarySidebar::adjustHeaderStretch);
    connect(this,
            &QTreeView::collapsed,
            this,
            &WLibrarySidebar::adjustHeaderStretch);
}

void WLibrarySidebar::setModel(QAbstractItemModel* pModel) {
    SidebarModel* pSidebarModel = qobject_cast<SidebarModel*>(pModel);
    DEBUG_ASSERT(pSidebarModel);
    m_pSidebarModel = pSidebarModel;
    QTreeView::setModel(pSidebarModel);
    // Create the delegate for painting the bookmark indicator
    DEBUG_ASSERT(m_pItemDelegate == nullptr);
    m_pItemDelegate = new SidebarItemDelegate(this, pSidebarModel);
    setItemDelegateForColumn(0, m_pItemDelegate);
    m_pItemDelegate->setBookmarkColor(m_bookmarkColor);
    // Color can be set in qss via qproperty-bookmarkColor which happens
    // when the stylesheet is applied. Push it to delegate.
    connect(this,
            &WLibrarySidebar::bookmarkColorChanged,
            m_pItemDelegate,
            &SidebarItemDelegate::setBookmarkColor);
}

void WLibrarySidebar::contextMenuEvent(QContextMenuEvent *event) {
    //if (event->state() & Qt::RightButton) { //Dis shiz don werk on windowze
    QModelIndex clickedIndex = indexAt(event->pos());
    if (!clickedIndex.isValid()) {
        return;
    }
    // Use this instead of setCurrentIndex() to keep current selection
    selectionModel()->setCurrentIndex(clickedIndex, QItemSelectionModel::NoUpdate);
    event->accept();
    emit rightClicked(event->globalPos(), clickedIndex);
    //}
}

/// Drag enter event, happens when a dragged item enters the track sources view
void WLibrarySidebar::dragEnterEvent(QDragEnterEvent * event) {
    qDebug() << "WLibrarySidebar::dragEnterEvent" << event->mimeData()->formats();
    if (event->mimeData()->hasUrls()) {
        // We don't have a way to ask the LibraryFeatures whether to accept a
        // drag so for now we accept all drags. Since almost every
        // LibraryFeature accepts all files in the drop and accepts playlist
        // drops we default to those flags to DragAndDropHelper.
        QList<mixxx::FileInfo> fileInfos = DragAndDropHelper::supportedTracksFromUrls(
                event->mimeData()->urls(), false, true);
        if (!fileInfos.isEmpty()) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
    //QTreeView::dragEnterEvent(event);
}

/// Drag move event, happens when a dragged item hovers over the track sources view
void WLibrarySidebar::dragMoveEvent(QDragMoveEvent * event) {
    //qDebug() << "dragMoveEvent" << event->mimeData()->formats();
    // Start a timer to auto-expand sections the user hovers on.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = event->position().toPoint();
#else
    QPoint pos = event->pos();
#endif
    QModelIndex index = indexAt(pos);
    if (m_hoverIndex != index) {
        m_expandTimer.stop();
        m_hoverIndex = index;
        m_expandTimer.start(expand_time, this);
    }
    // This has to be here instead of after, otherwise all drags will be
    // rejected -- rryan 3/2011
    QTreeView::dragMoveEvent(event);
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        // Drag and drop within this widget
        if ((event->source() == this)
                && (event->possibleActions() & Qt::MoveAction)) {
            // Do nothing.
            event->ignore();
        } else {
            bool accepted = false;
            for (const QUrl& url : urls) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                QPoint pos = event->position().toPoint();
#else
                QPoint pos = event->pos();
#endif
                QModelIndex destIndex = indexAt(pos);
                if (m_pSidebarModel->dragMoveAccept(destIndex, url)) {
                    // We only need one URL to be valid for us
                    // to accept the whole drag...
                    // Consider that we might have a long list of files,
                    // checking all will take a lot of time that stalls
                    // Mixxx and this makes the drop feature useless.
                    // E.g. you may have tried to drag two MP3's and an EXE,
                    // the drop is accepted here, but the EXE is filtered
                    // out later after dropping
                    accepted = true;
                    break;
                }
            }
            if (accepted) {
                event->acceptProposedAction();
            } else {
                event->ignore();
            }
        }
    } else {
        event->ignore();
    }
}

/// Timer events for delayed tree item expand/collapse on drag'n'drop and
/// header width adjustment when item layout or data has changed
void WLibrarySidebar::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_expandTimer.timerId()) {
        QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
        if (viewport()->rect().contains(pos)) {
            QModelIndex index = indexAt(pos);
            if (m_hoverIndex == index) {
                setExpanded(index, !isExpanded(index));
            }
        }
        m_expandTimer.stop();
        return;
    } else if (event->timerId() == m_headerAdjustTimer.timerId()) {
        // The header timer is a repeating QBasicTimer. Stop it when we can be
        // sure that the last trigger event has been processed (add some margin
        // since it's a Qt::CoarseTimer with an imprecision of +- 5%)
        if (m_eventFrequencyTimer.elapsed().toIntegerMillis() > resize_header_delay * 1.5) {
            m_headerAdjustTimer.stop();
            return;
        }
        adjustHeaderStretch();
        return;
    }
    QTreeView::timerEvent(event);
}

/// Drag-and-drop "drop" event. Occurs when something is dropped onto the track sources view
void WLibrarySidebar::dropEvent(QDropEvent * event) {
    if (event->mimeData()->hasUrls()) {
        // Drag and drop within this widget
        if ((event->source() == this)
                && (event->possibleActions() & Qt::MoveAction)) {
            // Do nothing.
            event->ignore();
        } else {
            //Reset the selected items (if you had anything highlighted, it clears it)
            //this->selectionModel()->clear();
            //Drag-and-drop from an external application or the track table widget
            //eg. dragging a track from Windows Explorer onto the sidebar
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPoint pos = event->position().toPoint();
#else
            QPoint pos = event->pos();
#endif

            QModelIndex destIndex = indexAt(pos);
            // event->source() will return NULL if something is dropped from
            // a different application
            const QList<QUrl> urls = event->mimeData()->urls();
            if (m_pSidebarModel->dropAccept(destIndex, urls, event->source())) {
                event->acceptProposedAction();
            } else {
                event->ignore();
            }
        }
    } else {
        event->ignore();
    }
}

void WLibrarySidebar::renameSelectedItem() {
    // Rename crate or playlist (internal, external, history)
    QModelIndex selIndex = selectedIndex();
    if (!selIndex.isValid()) {
        return;
    }
    emit renameItem(selIndex);
    return;
}

void WLibrarySidebar::toggleSelectedItem() {
    const QModelIndex index = selectedIndex();
    if (!index.isValid()) {
        return;
    }
    // Activate the item so its content shows in the main library.
    emit clicked(index);
    // Update child tree of BrowseFeature items with outdated tree
    if (m_pSidebarModel->indexNeedsUpdate(index)) {
        m_pSidebarModel->updateItem(index);
        return;
    }
    // Expand or collapse the item as necessary.
    setExpanded(index, !isExpanded(index));
}

bool WLibrarySidebar::isLeafNodeSelected() {
    QModelIndex index = selectedIndex();
    if (!index.isValid()) {
        return false;
    }
    if (!index.model()->hasChildren(index)) {
        return true;
    }
    return m_pSidebarModel->hasTrackTable(index);
}

bool WLibrarySidebar::isChildIndexSelected(const QModelIndex& index) {
    // qDebug() << "WLibrarySidebar::isChildIndexSelected" << index;
    QModelIndex selIndex = selectedIndex();
    if (!selIndex.isValid()) {
        return false;
    }
    QModelIndex translated = m_pSidebarModel->translateChildIndex(index);
    if (!translated.isValid()) {
        // qDebug() << " >> index can't be translated";
        return false;
    }
    return translated == selIndex;
}

bool WLibrarySidebar::isFeatureRootIndexSelected(LibraryFeature* pFeature) {
    // qDebug() << "WLibrarySidebar::isFeatureRootIndexSelected";
    QModelIndex selIndex = selectedIndex();
    if (!selIndex.isValid()) {
        return false;
    }
    const QModelIndex rootIndex = m_pSidebarModel->getFeatureRootIndex(pFeature);
    return rootIndex == selIndex;
}

void WLibrarySidebar::setBookmarkColor(const QColor& color) {
    if (color.isValid() && m_pItemDelegate) {
        m_pItemDelegate->setBookmarkColor(color);
    }
}

void WLibrarySidebar::toggleBookmark() {
    const QModelIndex selIndex = selectedIndex();
    if (!selIndex.isValid()) {
        qWarning() << " ! WLS bookmarkSelectedItem, invalid index" << selIndex;
        return;
    }

    m_pSidebarModel->toggleBookmarkByIndex(selIndex);
    update();
}

void WLibrarySidebar::goToNextPrevBookmark(int direction) {
    // Don't use selectedIndex(). Selected item may not be the focused item, eg.
    // if we focused a bookmark item without activating it.
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        qDebug() << "WLibrarySidebar::goToNextPrevBookmark invalid index" << index;
        return;
    }

    const QModelIndex bookmarkIdx = m_pSidebarModel->getNextPrevBookmarkIndex(index, direction);
    if (!bookmarkIdx.isValid() || bookmarkIdx == index) {
        // No bookmarks stored or none of them has been found.
        // Or, we are already on the only bookmark. In that case we don't reselect because
        // that would cause resorting (and reloading tracks for Computer path indices).
        return;
    }

    // just scroll to and highlight (focus)
    // Note: scrollTo() with default hint EnsureVisible will also expand all
    // parents, which in turn emits expanded() for each index which invokes
    // LibraryFeature::onLazyChildExpandation().
    scrollTo(bookmarkIdx);
    // Use this instead of setCurrentIndex() to keep current selection
    selectionModel()->setCurrentIndex(bookmarkIdx, QItemSelectionModel::NoUpdate);
    // TODO add control [Library],goToSelectedItem ??
    // Or add wrapper goToItem() that does
    // * select & activate focused item if it's not selected
    // * expand / collapse
    // * jump to tracks if double-tapped
}

/// Invoked by actual keypresses (requires widget focus) and emulated keypresses
/// sent by LibraryControl
void WLibrarySidebar::keyPressEvent(QKeyEvent* event) {
    // TODO(XXX) Should first keyEvent ensure previous item has focus? I.e. if the selected
    // item is not focused, require second press to perform the desired action.

    QModelIndex selIndex = selectedIndex();
    if (selIndex.isValid() && event->matches(QKeySequence::Paste)) {
        m_pSidebarModel->paste(selIndex);
        return;
    }

    // Don't focus selection if we receive a modifier-only event, for example
    // Alt for bookmark actions.
    //    if (!(event->key() >= Qt::Key_Shift && event->key() <= Qt::Key_Alt) &&
    //            event->key() != Qt::Key_AltGr) {
    //        // Do not act on Modifier only, Shift, Ctrl, Meta, Alt and AltGr
    //        // avoid returning "khmer vowel sign ie (U+17C0)"
    //        // TODO move below bookmark actions?
    //        focusSelectedIndex();
    //    }

    // Alt + B: un/bookmark selected item
    // Alt + Up/Down: jump to and highlight next/previous bookmarked item
    // Press Enter to activate
    if (event->modifiers().testFlag(Qt::AltModifier)) {
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up) {
            goToNextPrevBookmark(event->key() == Qt::Key_Down ? 1 : -1);
        } else if (event->key() == Qt::Key_B) {
            toggleBookmark();
        }
        // No further Alt, might as well be a system shortcut
        return;
    }

    switch (event->key()) {
    case Qt::Key_Return:
        // If the selection is not focused, focus it and scroll to it first.
        // Happens when going to bookmark with activating it.
        if (selectFocusedIndex()) {
            return;
        }
        toggleSelectedItem();
        return;
    case Qt::Key_Down:
    case Qt::Key_Up:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
    case Qt::Key_End:
    case Qt::Key_Home: {
        // If the selection is not focused, focus it and scroll to it first.
        // Happens when going to bookmark without activating it.
        if (focusSelectedIndex()) {
            return;
        }
        // Let the tree view move up and down for us.
        QTreeView::keyPressEvent(event);
        // After the selection changed force-activate (click) the newly selected
        // item to save us from having to push "Enter".
        QModelIndex selIndex = selectedIndex();
        if (!selIndex.isValid()) {
            return;
        }
        // Ensure the new selection is visible even if it was already selected/
        // focused, like when the topmost item was selected but out of sight and
        // we pressed Up, Home or PageUp.
        scrollTo(selIndex);
        emit pressed(selIndex);
        return;
    }
    case Qt::Key_Right: {
        if (event->modifiers() & Qt::ControlModifier) {
            emit setLibraryFocus(FocusWidget::TracksTable);
        } else {
            if (focusSelectedIndex()) {
                return;
            }
            QTreeView::keyPressEvent(event);
        }
        return;
    }
    case Qt::Key_Left: {
        if (focusSelectedIndex()) {
            return;
        }
        // If an expanded item is selected let QTreeView collapse it
        QModelIndex selIndex = selectedIndex();
        if (!selIndex.isValid()) {
            return;
        }
        // collapse knot
        if (isExpanded(selIndex)) {
            QTreeView::keyPressEvent(event);
            return;
        }
        // Else jump to its parent and activate it
        QModelIndex parentIndex = selIndex.parent();
        if (parentIndex.isValid()) {
            selectIndex(parentIndex);
            emit pressed(parentIndex);
        }
        return;
    }
    case Qt::Key_Escape:
        // Focus tracks table
        emit setLibraryFocus(FocusWidget::TracksTable);
        return;
    case kRenameSidebarItemShortcutKey: { // F2
        renameSelectedItem();
        return;
    }
    case kHideRemoveShortcutKey: { // Del (macOS: Cmd+Backspace)
        // Delete crate or playlist (internal, external, history)
        if (event->modifiers() != kHideRemoveShortcutModifier) {
            return;
        }
        QModelIndex selIndex = selectedIndex();
        if (!selIndex.isValid()) {
            return;
        }
        emit deleteItem(selIndex);
        return;
    }
    default:
        QTreeView::keyPressEvent(event);
    }
}

void WLibrarySidebar::mousePressEvent(QMouseEvent* event) {
    // handle right click only in contextMenuEvent() to not select the clicked index
    if (event->buttons().testFlag(Qt::RightButton)) {
        return;
    }
    QTreeView::mousePressEvent(event);
}

void WLibrarySidebar::focusInEvent(QFocusEvent* event) {
    // Clear the current index, i.e. remove the focus indicator
    // selectionModel()->clearCurrentIndex();
    focusSelectedIndex();
    QTreeView::focusInEvent(event);
}

void WLibrarySidebar::selectIndex(const QModelIndex& index) {
    // qDebug() << "WLibrarySidebar::selectIndex" << index;
    if (!index.isValid()) {
        return;
    }
    auto* pModel = new QItemSelectionModel(model());
    pModel->select(index, QItemSelectionModel::Select);
    if (selectionModel()) {
        selectionModel()->deleteLater();
    }
    setSelectionModel(pModel);
    setCurrentIndex(index);
    // Note: scrollTo() with default hint EnsureVisible will also expand all
    // parents, which in turn emits expanded() for each index which invokes
    // LibraryFeature::onLazyChildExpandation().
    scrollTo(index);
}

/// Selects a child index from a feature and ensures visibility
void WLibrarySidebar::selectChildIndex(const QModelIndex& index, bool selectItem) {
    QModelIndex translated = m_pSidebarModel->translateChildIndex(index);
    if (!translated.isValid()) {
        return;
    }

    if (selectItem) {
        auto* pModel = new QItemSelectionModel(m_pSidebarModel);
        pModel->select(translated, QItemSelectionModel::Select);
        if (selectionModel()) {
            selectionModel()->deleteLater();
        }
        setSelectionModel(pModel);
        setCurrentIndex(translated);
    }

    // Note: scrollTo() with default hint EnsureVisible will also expand all
    // parents, which in turn emits expanded() for each index which invokes
    // LibraryFeature::onLazyChildExpandation().
    scrollTo(translated);
}

QModelIndex WLibrarySidebar::selectedIndex() {
    QModelIndexList selectedIndices = selectionModel()->selectedRows();
    if (selectedIndices.isEmpty()) {
        return QModelIndex();
    }
    QModelIndex selIndex = selectedIndices.first();
    DEBUG_ASSERT(selIndex.isValid());
    return selIndex;
}

/// Refocus the selected item after right-click
bool WLibrarySidebar::focusSelectedIndex() {
    // After the context menu was activated (and closed, with or without clicking
    // an action), the currentIndex is the right-clicked item.
    // If if the currentIndex is not selected, make the selection the currentIndex
    QModelIndex selIndex = selectedIndex();
    if (selIndex.isValid() && selIndex != selectionModel()->currentIndex()) {
        setCurrentIndex(selIndex);
        return true;
    }
    return false;
}

bool WLibrarySidebar::selectFocusedIndex() {
    const QModelIndex selIndex = selectedIndex();
    const QModelIndex focusIndex = selectionModel()->currentIndex();
    // qWarning() << " -- selected index:" << selIndex;
    // qWarning() << " -- focused index: " << focusIndex;
    if (focusIndex.isValid() && focusIndex != selIndex) {
        // qWarning() << " -- select focused index, scroll to";
        scrollTo(focusIndex);
        selectIndex(focusIndex);
        emit pressed(focusIndex);
        return true;
    }
    // qWarning() << " -- ! focused index invalid" << focusIndex;
    return false;
}

bool WLibrarySidebar::event(QEvent* pEvent) {
    switch (pEvent->type()) {
    case QEvent::ToolTip:
        updateTooltip();
        break;
    case QEvent::Resize:
    // Stretch the header when layout changes, e.g. viewport size changed due to
    // scrollbar hide/show
    case QEvent::LayoutRequest:
    case QEvent::FontChange:
    case QEvent::Polish:
    case QEvent::PolishRequest:
        queueHeaderAdjustRequest();
        break;
    default:
        break;
    }
    return QTreeView::event(pEvent);
}

void WLibrarySidebar::queueHeaderAdjustRequest() {
    m_eventFrequencyTimer.restart();
    m_headerAdjustTimer.start(resize_header_delay, this);
}

/// Ensure tree items expand horizontally so we have the entire view width respond
/// to mouse clicks, i.e. no unresponsive space right next to short items.
void WLibrarySidebar::adjustHeaderStretch() {
    // Disable stretching to trigger adjusting columns (ResizeToContents),
    // i.e. full labels without elide
    if (header()->stretchLastSection()) {
        header()->setStretchLastSection(false);
    }

    // Enable stretching if there's space for the header to expand.
    // Else, horizontal scrollbars are visible, nothing to do.
    if (header()->sectionSize(0) < header()->width()) {
        header()->setStretchLastSection(true);
    }
}

void WLibrarySidebar::slotSetFont(const QFont& font) {
    setFont(font);
    // Resize the feature icons to be a bit taller than the label's capital
    int iconSize = static_cast<int>(QFontMetrics(font).height() * 0.8);
    setIconSize(QSize(iconSize, iconSize));
}
