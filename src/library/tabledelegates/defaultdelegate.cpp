#include "library/tabledelegates/defaultdelegate.h"

#include <QPainter>

#include "moc_defaultdelegate.cpp"

DefaultDelegate::DefaultDelegate(QTableView* pTableView)
        : QStyledItemDelegate(pTableView),
          m_pTableView(pTableView) {
}

void DefaultDelegate::paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    setHighlightedTextColor(opt, index);

    QStyledItemDelegate::paint(painter, opt, index);
}

void DefaultDelegate::setHighlightedTextColor(
        QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    // Blend the original `selection-color` with the 'played' or 'missing' color
    // from the model.
    // Get the palette's text color
    QColor hlcol = option.palette.highlightedText().color();
    auto colorData = index.data(Qt::ForegroundRole);
    if (colorData.canConvert<QColor>()) {
        const QColor fgCol = colorData.value<QColor>();
        if (fgCol == hlcol) {
            return;
        }
        // 70/30 seems to be a good ratio for LateNight PaleMoon
        hlcol = QColor(
                static_cast<int>(fgCol.red() * 0.8 + hlcol.red() * 0.2),
                static_cast<int>(fgCol.green() * 0.8 + hlcol.green() * 0.2),
                static_cast<int>(fgCol.blue() * 0.8 + hlcol.blue() * 0.2));
        option.palette.setColor(QPalette::Normal, QPalette::HighlightedText, hlcol);
    }
}
