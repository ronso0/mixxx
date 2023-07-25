#include "widget/wtrackproperty.h"

#include <QApplication>
#include <QDebug>
#include <QMetaProperty>
#include <QStyleOption>

#include "control/controlobject.h"
#include "moc_wtrackproperty.cpp"
#include "skin/legacy/skincontext.h"
#include "track/track.h"
#include "util/dnd.h"
#include "widget/wtrackmenu.h"

namespace {
constexpr WTrackMenu::Features kTrackMenuFeatures =
        WTrackMenu::Feature::SearchRelated |
        WTrackMenu::Feature::Playlist |
        WTrackMenu::Feature::Crate |
        WTrackMenu::Feature::Metadata |
        WTrackMenu::Feature::Reset |
        WTrackMenu::Feature::Analyze |
        WTrackMenu::Feature::BPM |
        WTrackMenu::Feature::Color |
        WTrackMenu::Feature::RemoveFromDisk |
        WTrackMenu::Feature::FileBrowser |
        WTrackMenu::Feature::Properties |
        WTrackMenu::Feature::UpdateReplayGainFromPregain |
        WTrackMenu::Feature::FindOnWeb |
        WTrackMenu::Feature::SelectInLibrary;

// Duration (ms) the widget is 'selected' after left click, i.e. the duration
// a second click would open the value editor
constexpr int selectedClickTimeout = 2000;
} // namespace

WTrackProperty::WTrackProperty(
        QWidget* pParent,
        UserSettingsPointer pConfig,
        Library* pLibrary,
        const QString& group)
        : WLabel(pParent),
          m_group(group),
          m_pConfig(pConfig),
          m_pLibrary(pLibrary),
          m_propertyIsWritable(false),
          m_pSelectedClickTimer(nullptr),
          m_bSelected(false),
          m_pEditor(nullptr) {
    setAcceptDrops(true);
}

WTrackProperty::~WTrackProperty() {
    // Required to allow forward declaration of WTrackMenu in header
}

void WTrackProperty::setup(const QDomNode& node, const SkinContext& context) {
    WLabel::setup(node, context);

    QString property = context.selectString(node, "Property");
    if (property.isEmpty()) {
        return;
    }

    // Check if property with that name exists in Track class
    int propertyIndex = Track::staticMetaObject.indexOfProperty(property.toUtf8().constData());
    if (propertyIndex == -1) {
        qWarning() << "WTrackProperty: Unknown track property:" << property;
        return;
    }
    m_displayProperty = property;
    // Handle 'titleInfo' property: displays the title or, if both title & artist
    // are empty, filename. Though, this property is not writeable, so we map
    // it to 'title' for the editor.
    if (property == "titleInfo") {
        m_editProperty = "title";
    } else {
        if (!Track::staticMetaObject.property(propertyIndex).isWritable()) {
            return;
        }
        m_editProperty = m_displayProperty;
    }
    m_propertyIsWritable = true;
}

void WTrackProperty::slotTrackLoaded(TrackPointer pTrack) {
    if (!pTrack) {
        return;
    }
    m_pCurrentTrack = pTrack;
    connect(pTrack.get(),
            &Track::changed,
            this,
            &WTrackProperty::slotTrackChanged);
    updateLabel();
}

void WTrackProperty::slotLoadingTrack(TrackPointer pNewTrack, TrackPointer pOldTrack) {
    Q_UNUSED(pNewTrack);
    Q_UNUSED(pOldTrack);
    if (m_pCurrentTrack) {
        disconnect(m_pCurrentTrack.get(), nullptr, this, nullptr);
    }
    m_pCurrentTrack.reset();
    if (m_pEditor && m_pEditor->hasFocus()) {
        m_pEditor->hide();
    }
    updateLabel();
}

void WTrackProperty::slotTrackChanged(TrackId trackId) {
    Q_UNUSED(trackId);
    updateLabel();
}

void WTrackProperty::updateLabel() {
    if (m_pCurrentTrack) {
        setText(getPropertyStringFromTrack(m_displayProperty));
        return;
    }
    setText("");
}

const QString WTrackProperty::getPropertyStringFromTrack(QString& property) const {
    if (property.isEmpty() || !m_pCurrentTrack) {
        return {};
    }
    QVariant propVar = m_pCurrentTrack->property(property.toUtf8().constData());
    if (propVar.isValid() && propVar.canConvert<QString>()) {
        return propVar.toString();
    }
    return {};
}

void WTrackProperty::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons().testFlag(Qt::LeftButton) && m_pCurrentTrack) {
        DragAndDropHelper::dragTrack(m_pCurrentTrack, this, m_group);
    }
}

void WTrackProperty::mousePressEvent(QMouseEvent* event) {
    if (!event->buttons().testFlag(Qt::LeftButton) || !m_pCurrentTrack) {
        return;
    }

    // close any open editor
    WTrackPropertyEditor* otherEditor =
            qobject_cast<WTrackPropertyEditor*>(QApplication::focusWidget());
    if (otherEditor) {
        otherEditor->clearFocus();
    }

    // Don't create the editor or toggle the 'selected' state for protected
    // properties like duration.
    if (!m_propertyIsWritable) {
        return;
    }

    if (!m_pSelectedClickTimer) {
        // create & start the timer
        m_pSelectedClickTimer = new QTimer(this);
        m_pSelectedClickTimer->setSingleShot(true);
        m_pSelectedClickTimer->setInterval(selectedClickTimeout);
        m_pSelectedClickTimer->callOnTimeout(
                this, &WTrackProperty::resetSelectedState);
    } else if (m_pSelectedClickTimer->isActive()) {
        resetSelectedState();
        // create the persistent editor, populate & connect
        if (!m_pEditor) {
            m_pEditor = make_parented<WTrackPropertyEditor>(this);
            connect(m_pEditor,
                    // use custom signal. editingFinished() doesn't suit since it's
                    // also emitted weh pressing Esc (which should cancel editing)
                    &WTrackPropertyEditor::commitEditorData,
                    this,
                    &WTrackProperty::slotCommitEditorData);
        }
        // Don't let the editor expand beyond its initial size
        m_pEditor->setFixedSize(size());

        QString editText = getPropertyStringFromTrack(m_editProperty);
        if (m_displayProperty == "titleInfo" && editText.isEmpty()) {
            editText = tr("title");
        }
        m_pEditor->setText(editText);
        m_pEditor->selectAll();
        m_pEditor->show();
        m_pEditor->setFocus();
        return;
    }
    // start timer
    m_pSelectedClickTimer->start();
    m_bSelected = true;
    restyleAndRepaint();
}

void WTrackProperty::mouseDoubleClickEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    if (!m_pCurrentTrack) {
        return;
    }
    resetSelectedState();

    ensureTrackMenuIsCreated();
    m_pTrackMenu->loadTrack(m_pCurrentTrack, m_group);
    m_pTrackMenu->showDlgTrackInfo(m_displayProperty);
}

void WTrackProperty::dragEnterEvent(QDragEnterEvent* event) {
    DragAndDropHelper::handleTrackDragEnterEvent(event, m_group, m_pConfig);
}

void WTrackProperty::dropEvent(QDropEvent* event) {
    DragAndDropHelper::handleTrackDropEvent(event, *this, m_group, m_pConfig);
}

void WTrackProperty::contextMenuEvent(QContextMenuEvent* event) {
    event->accept();
    if (m_pCurrentTrack) {
        ensureTrackMenuIsCreated();
        m_pTrackMenu->loadTrack(m_pCurrentTrack, m_group);
        // Create the right-click menu
        m_pTrackMenu->popup(event->globalPos());
    }
}

void WTrackProperty::ensureTrackMenuIsCreated() {
    if (m_pTrackMenu.get() == nullptr) {
        m_pTrackMenu = make_parented<WTrackMenu>(
                this, m_pConfig, m_pLibrary, kTrackMenuFeatures);
    }
}

void WTrackProperty::slotCommitEditorData(const QString& text) {
    // use real track data instead of text() to be independent from display text
    if (m_pCurrentTrack && text != getPropertyStringFromTrack(m_editProperty)) {
        const QVariant var(QVariant::fromValue(text));
        m_pCurrentTrack->setProperty(
                m_editProperty.toUtf8().constData(),
                var);
        // Track::changed() will update label
    }
}

void WTrackProperty::resetSelectedState() {
    if (m_pSelectedClickTimer) {
        m_pSelectedClickTimer->stop();
        // explicitly disconnect() queued signals? not crucial
        // here since timeOut() just calls resetSelectedState()
    }
    m_bSelected = false;
    restyleAndRepaint();
}

void WTrackProperty::restyleAndRepaint() {
    emit selectedStateChanged(isSelected());

    style()->unpolish(this);
    style()->polish(this);
    // These calls don't always trigger the repaint, so call it explicitly.
    repaint();
}

WTrackPropertyEditor::WTrackPropertyEditor(QWidget* pParent)
        : QLineEdit(pParent) {
    installEventFilter(this);
}

bool WTrackPropertyEditor::eventFilter(QObject* pObj, QEvent* pEvent) {
    if (pEvent->type() == QEvent::KeyPress) {
        // The widget only receives keystrokes when in edit mode.
        // Esc will close & reset.
        // Enter/Return confirms.
        // Any other keypress is forwarded.
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
        const int key = keyEvent->key();
        switch (key) {
        case Qt::Key_Escape:
            hide();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            hide();
            emit commitEditorData(text());
            ControlObject::set(ConfigKey("[Library]", "refocus_prev_widget"), 1);
            return true;
        default:
            break;
        }
    } else if (pEvent->type() == QEvent::FocusOut) {
        // Close and commit if any other widget gets focus
        if (isVisible()) {
            hide();
            emit commitEditorData(text());
        }
    }
    return QLineEdit::eventFilter(pObj, pEvent);
}
