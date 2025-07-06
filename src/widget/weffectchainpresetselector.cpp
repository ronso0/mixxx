#include "widget/weffectchainpresetselector.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QScreen>
#include <QStyleOption>
#include <QStylePainter>
#include <QTimer>

#include "effects/chains/quickeffectchain.h"
#include "effects/effectsmanager.h"
#include "effects/presets/effectchainpreset.h"
#include "effects/presets/effectpreset.h"
#include "moc_weffectchainpresetselector.cpp"
#include "util/widgethelper.h"
#include "widget/effectwidgetutils.h"

class QPaintEvent;

WEffectChainPresetSelector::WEffectChainPresetSelector(
        QWidget* pParent, EffectsManager* pEffectsManager)
        : QComboBox(pParent),
          WBaseWidget(this),
          m_bQuickEffectChain(false),
          m_pChainPresetManager(pEffectsManager->getChainPresetManager()),
          m_pEffectsManager(pEffectsManager) {
    // Prevent this widget from getting focused by Tab/Shift+Tab
    // to avoid interfering with using the library via keyboard.
    // Allow click focus though so the list can always be opened by mouse,
    // see https://github.com/mixxxdj/mixxx/issues/10184
    setFocusPolicy(Qt::ClickFocus);
}

void WEffectChainPresetSelector::setup(const QDomNode& node, const SkinContext& context) {
    m_pChain = EffectWidgetUtils::getEffectChainFromNode(
            node, context, m_pEffectsManager);

    VERIFY_OR_DEBUG_ASSERT(m_pChain != nullptr) {
        SKIN_WARNING(node,
                context,
                QStringLiteral("EffectChainPresetSelector node could not "
                               "attach to EffectChain"));
        return;
    }

    auto* pQuickEffectChain = qobject_cast<QuickEffectChain*>(m_pChain.data());
    if (pQuickEffectChain) {
        connect(m_pChainPresetManager.data(),
                &EffectChainPresetManager::quickEffectChainPresetListUpdated,
                this,
                &WEffectChainPresetSelector::populate);
        m_bQuickEffectChain = true;
    } else {
        connect(m_pChainPresetManager.data(),
                &EffectChainPresetManager::effectChainPresetListUpdated,
                this,
                &WEffectChainPresetSelector::populate);
    }
    connect(m_pChain.data(),
            &EffectChain::chainPresetChanged,
            this,
            &WEffectChainPresetSelector::slotChainPresetChanged);
    connect(this,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &WEffectChainPresetSelector::slotEffectChainPresetSelected);

    populate();
}

void WEffectChainPresetSelector::populate() {
    blockSignals(true);
    clear();

    QFontMetrics metrics(font());

    QList<EffectChainPresetPointer> presetList;
    if (m_bQuickEffectChain) {
        presetList = m_pEffectsManager->getChainPresetManager()->getQuickEffectPresetsSorted();
    } else {
        presetList = m_pEffectsManager->getChainPresetManager()->getPresetsSorted();
    }

    const EffectsBackendManagerPointer pBackendManager = m_pEffectsManager->getBackendManager();
    QStringList effectNames;
    for (int i = 0; i < presetList.size(); i++) {
        auto pChainPreset = presetList.at(i);
        QString elidedDisplayName = metrics.elidedText(pChainPreset->name(),
                Qt::ElideMiddle,
                view()->width() - 2);
        addItem(elidedDisplayName, QVariant(pChainPreset->name()));
        QString tooltip =
                QStringLiteral("<b>") + pChainPreset->name() + QStringLiteral("</b>");
        for (const auto& pEffectPreset : pChainPreset->effectPresets()) {
            if (!pEffectPreset->isEmpty()) {
                EffectManifestPointer pManifest = pBackendManager->getManifest(pEffectPreset);
                if (pManifest) {
                    effectNames.append(pManifest->name());
                }
            }
        }
        if (effectNames.size() > 1) {
            tooltip.append("<br/>");
            tooltip.append(effectNames.join("<br/>"));
        }
        effectNames.clear();
        setItemData(i, tooltip, Qt::ToolTipRole);
    }

    slotChainPresetChanged(m_pChain->presetName());
    blockSignals(false);

    // Will default to 0,0 if this is not visible
    // m_lastTopLeft = mapToGlobal(geometry().topLeft());

    const QScreen* const pScreen = mixxx::widgethelper::getScreen(*this);
    QRect screenGeometry;
    VERIFY_OR_DEBUG_ASSERT(pScreen) {
        qWarning() << "Assuming screen size of 800x600px.";
        screenGeometry = QRect(0, 0, 800, 600);
    }
    else {
        screenGeometry = pScreen->geometry();
    }

    // Position it roughly above the artist/title row
    int yPos = screenGeometry.center().y() - 365;
    int xPos = screenGeometry.center().x();
    if (m_pChain->getGroup().endsWith("nel1]]") ||
            m_pChain->getGroup().endsWith("nel3]]")) {
        m_lastTopLeft = QPoint(xPos - 135, yPos);
    } else { // Channel2/4
        m_lastTopLeft = QPoint(xPos + 165, yPos);
    }
    // if (m_pChain->getGroup().endsWith("nel1]]")) {
    //    qWarning() << "     .";
    //    qWarning() << "     setup, m_lastTopLeft =" << m_lastTopLeft;
    //    qWarning() << "     .";
    // }
}

void WEffectChainPresetSelector::slotEffectChainPresetSelected(int index) {
    Q_UNUSED(index);
    m_pChain->loadChainPreset(
            m_pChainPresetManager->getPreset(currentData().toString()));
    // Clicking a chain item moves keyboard focus to the list view.
    // Move focus back to the previously focused library widget.
    ControlObject::set(ConfigKey("[Library]", "refocus_prev_widget"), 1);
}

void WEffectChainPresetSelector::slotChainPresetChanged(const QString& name) {
    setCurrentIndex(findData(name));
    setBaseTooltip(itemData(currentIndex(), Qt::ToolTipRole).toString());

    if (m_bQuickEffectChain && !isVisible()) {
        // Show tooltip with preset name
        // if (m_pChain->getGroup().contains("nel1]]")) {
        //    qWarning() << "     .";
        //    qWarning() << "     on change, m_lastTopLeft =" << m_lastTopLeft;
        //    qWarning() << "     baseTooltip:" << baseTooltip();
        //    qWarning() << "     .";
        // }

        // Only works while the widget is visible..
        // QToolTip::showText(m_lastTopLeft, baseTooltip(), this);
        // passing nullptr might work, but then we wouldn't inherit the stylesheet
        //
        // Alt: use QLabel. doesn't work either, won't show up.
        // Because this is hidden??
        QLabel* pLabel = new QLabel(this);
        pLabel->setObjectName("QuickEffectPresetTooltip");
        pLabel->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
        const QString tooltip = baseTooltip();
        pLabel->setText(tooltip);
        pLabel->move(m_lastTopLeft);
        pLabel->show();
        // if (m_pChain->getGroup().contains("nel1]]")) {
        //    qWarning() << "     label is vis:" << pLabel->isVisible();
        //    qWarning() << "     label pos:" << pLabel->pos();
        //    qWarning() << "     label pos:" << mapToGlobal(pLabel->pos());
        //    qWarning() << "     .";
        // }
        QTimer::singleShot(1500,
                Qt::CoarseTimer,
                this,
                [pLabel]() { pLabel->hide(); });
    }
}

bool WEffectChainPresetSelector::event(QEvent* pEvent) {
    if (pEvent->type() == QEvent::ToolTip) {
        updateTooltip();
    } else if (pEvent->type() == QEvent::Wheel && !hasFocus()) {
        // don't change preset by scrolling hovered preset selector
        return true;
    } else if (pEvent->type() == QEvent::Hide) {
        // Store geometry.
        // When hidden, display Tooltip when effect
        // m_lastTopLeft = mapToGlobal(geometry().topLeft());
        // if (m_pChain->getGroup().contains("nel1]]")) {
        //    qWarning() << "     .";
        //    qWarning() << "     on hide, m_lastTopLeft =" << m_lastTopLeft;
        //    qWarning() << "     .";
        // }
    }

    return QComboBox::event(pEvent);
}

void WEffectChainPresetSelector::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e);
    // The default paint implementation aligns the text based on the layout direction.
    // Override to allow qss to align the text of the closed combobox with the
    // Quick effect controls in the mixer.
    QStylePainter painter(this);
    QStyleOptionComboBox comboStyle;
    // Initialize the style and draw the frame, down-arrow etc.
    // Note: using 'comboStyle.initFrom(this)' and 'painter.drawComplexControl(...)
    // here would not paint the hover style of the down arrow.
    initStyleOption(&comboStyle);
    style()->drawComplexControl(QStyle::CC_ComboBox, &comboStyle, &painter, this);

    QStyleOptionButton buttonStyle;
    buttonStyle.initFrom(this);
    QRect buttonRect = style()->subControlRect(
            QStyle::CC_ComboBox, &comboStyle, QStyle::SC_ComboBoxEditField, this);
    buttonStyle.rect = buttonRect;
    QFontMetrics metrics(font());
    // Since the chain selector and the popup can differ in width,
    // elide the button text independently from the popup display name.
    buttonStyle.text = metrics.elidedText(
            currentData().toString(),
            Qt::ElideRight,
            buttonRect.width());
    // Draw the text for the selector button. Alternative: painter.drawControl(...)
    style()->drawControl(QStyle::CE_PushButtonLabel, &buttonStyle, &painter, this);
}
