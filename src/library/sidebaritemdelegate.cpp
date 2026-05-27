#include "library/sidebaritemdelegate.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

#include "library/sidebarmodel.h"
#include "moc_sidebaritemdelegate.cpp"
#include "util/assert.h"
#include "widget/wlibrarysidebar.h"

SidebarItemDelegate::SidebarItemDelegate(
        WLibrarySidebar* pSidebarWidget,
        SidebarModel* pSidebarModel)
        : QStyledItemDelegate(pSidebarWidget),
          m_pSidebarModel(pSidebarModel) {
    DEBUG_ASSERT(m_pSidebarModel);
}

// Used to paint SidebarBookmarks
void SidebarItemDelegate::paint(
        QPainter* pPainter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    QStyledItemDelegate::paint(pPainter, option, index);
}

// Used to catch clicks on TreeItem icons. Implemented only for BrowseFeature
// folder items that need an update. Click does force-rebuild the child tree.
bool SidebarItemDelegate::editorEvent(QEvent* pEvent,
        QAbstractItemModel* pModel,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) {
    // Only act on left click
    // Note: act on release in case we implement drag'n'drop for items.
    if (pEvent->type() == QEvent::MouseButtonPress) {
        if (m_pSidebarModel->data(index, SidebarModel::NeedsUpdateRole).toBool()) {
            QMouseEvent* pME = static_cast<QMouseEvent*>(pEvent);
            // Right click should be be handled by WLibrarySidebar and sidebar models!
            VERIFY_OR_DEBUG_ASSERT(pME->button() == Qt::LeftButton) {
                return false;
            }
            // Check if it's a click on the icon
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);
            const QWidget* pWidget = opt.widget;
            const QRect iconRect = pWidget->style()->subElementRect(
                    QStyle::SE_ItemViewItemDecoration,
                    &opt,
                    pWidget);
            if (iconRect.contains(pME->pos())) {
                // Enforce update (tree rebuild)
                qWarning() << "SidebarItemDelegate::editorEvent -> updateitem";
                m_pSidebarModel->updateItem(index);
            }
        }
    }
    return QStyledItemDelegate::editorEvent(pEvent, pModel, option, index);
}
