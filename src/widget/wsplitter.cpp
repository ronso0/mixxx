#include "widget/wsplitter.h"

#include <QEvent>
#include <QList>

#include "moc_wsplitter.cpp"
#include "skin/legacy/skincontext.h"

WSplitter::WSplitter(QWidget* pParent, UserSettingsPointer pConfig)
        : QSplitter(pParent),
          WBaseWidget(this),
          m_pConfig(pConfig) {
    connect(this, &WSplitter::splitterMoved, this, &WSplitter::slotSplitterMoved);
}

void WSplitter::setup(const QDomNode& node, const SkinContext& context) {
    // Load split sizes
    QString sizesJoined;
    QString msg;
    bool ok = false;

    // Default orientation is horizontal. For vertical splitters, the orientation must be set
    // before calling setSizes() for reloading the saved state to work.
    QString layout;
    if (context.hasNodeSelectString(node, "Orientation", &layout)) {
        if (layout == "vertical") {
            setOrientation(Qt::Vertical);
        } else if (layout == "horizontal") {
            setOrientation(Qt::Horizontal);
        }
    }

    // Try to load last values stored in mixxx.cfg
    QString splitSizesConfigKey;
    if (context.hasNodeSelectString(node, "SplitSizesConfigKey", &splitSizesConfigKey)) {
        m_configKey = ConfigKey::parseCommaSeparated(splitSizesConfigKey);

        if (m_pConfig->exists(m_configKey)) {
            sizesJoined = m_pConfig->getValueString(m_configKey);
            msg = "Reading .cfg file: '" + m_configKey.group + " " +
                    m_configKey.item + " " + sizesJoined +
                    "' does not match the number of children nodes:" +
                    QString::number(count());
            ok = true;
        }
    }

    // nothing in mixxx.cfg? Load default values
    if (!ok && context.hasNodeSelectString(node, "SplitSizes", &sizesJoined)) {
        msg = "<SplitSizes> for <Splitter> (" + sizesJoined +
                ") does not match the number of children nodes:" +
                QString::number(count());
    }

    // found some value for splitsizes?
    if (!sizesJoined.isEmpty()) {
        const QStringList sizesSplit = sizesJoined.split(",");
        QList<int> sizesList;
        ok = false;
        for (const QString& sizeStr : sizesSplit) {
            sizesList.push_back(sizeStr.toInt(&ok));
            if (!ok) {
                break;
            }
        }
        if (sizesList.length() != count()) {
            SKIN_WARNING(node, context, msg);
            ok = false;
        }
        if (ok) {
            this->setSizes(sizesList);
        }
    }

    // Which children can be collapsed?
    QString collapsibleJoined;
    if (context.hasNodeSelectString(node, "Collapsible", &collapsibleJoined)) {
        const QStringList collapsibleSplit = collapsibleJoined.split(",");
        QList<bool> collapsibleList;
        ok = false;
        for (const QString& collapsibleStr : collapsibleSplit) {
            collapsibleList.push_back(collapsibleStr.toInt(&ok)>0);
            if (!ok) {
                break;
            }
        }
        if (collapsibleList.length() != count()) {
            msg = "<Collapsible> for <Splitter> (" + collapsibleJoined +
                    ") does not match the number of children nodes:" +
                    QString::number(count());
            SKIN_WARNING(node, context, msg);
            ok = false;
        }
        if (ok) {
            int i = 0;
            for (bool collapsible : collapsibleList) {
                setCollapsible(i++, collapsible);
            }
        }
    }

    // If we have two children, allow to collapse the left/top section with
    // double-click on the handle
    qWarning() << "     Splitter count:" << count();
    if (count() == 2) {
        // Apparently the handle between two widgets belongs to the second one,
        // so if we want to control the handle between widget 0 and 1 we
        // need to check out handle(1) ¯\_(ツ)_/¯
        qWarning() << "     -> inst. evFilt for handle 1";
        handle(1)->installEventFilter(this);
    }
}

void WSplitter::slotSplitterMoved() {
    if (!m_configKey.group.isEmpty() && !m_configKey.item.isEmpty()) {
        QStringList sizeStrList;
        const auto sizesIntList = sizes();
        for (const int& sizeInt : sizesIntList) {
            sizeStrList.push_back(QString::number(sizeInt));
        }
        QString sizesStr = sizeStrList.join(",");
        m_pConfig->set(m_configKey, ConfigValue(sizesStr));
    }
}

bool WSplitter::eventFilter(QObject* pObj, QEvent* pEvent) {
    if (pEvent->type() == QEvent::MouseButtonDblClick &&
            count() == 2 &&
            isCollapsible(0)) {
        QList<int> currSizes = sizes();
        qWarning() << "     Splitter DblClick" << pObj << "| curr sizes:" << currSizes;

        bool leftCollapsed = currSizes[0] == 0;
        if (leftCollapsed && m_prevSizes[0] > 0) {
            // re-expand first child
            qWarning() << "     -> Expand. restore sizes:" << m_prevSizes;
            setSizes(m_prevSizes);
        } else {
            qWarning() << "     -> Collapse. curr sizes:" << sizes();
            // collapse first child
            m_prevSizes = sizes();
            // Create list of proportional sizes
            // 0: collapse, 1: 100 % of remaining space
            // TODO multiple 1 means children will share space equally, which would
            // could resize when the splitter ahs more that two children
            QList<int> newSizes;
            newSizes.append(0);
            int i = 1;
            while (newSizes.size() < count()) {
                // for (int i = 1; i < count(); i++) {
                qWarning() << "     -> insert prop.size #" << i;
                newSizes.append(1);
                i++;
            }
            setSizes(newSizes);
            qWarning() << "     -> Collapsed, new sizes:" << sizes();
        }
        return false;
    }
    return QSplitter::eventFilter(pObj, pEvent);
}

bool WSplitter::event(QEvent* pEvent) {
    if (pEvent->type() == QEvent::ToolTip) {
        updateTooltip();
    }
    return QSplitter::event(pEvent);
}
