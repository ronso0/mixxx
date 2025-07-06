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

#ifdef __STEM__
    VERIFY_OR_DEBUG_ASSERT(m_pChain != nullptr) {
        SKIN_WARNING(node,
                context,
                QStringLiteral("EffectChainPresetSelector node could not "
                               "attach to EffectChain"));
        return;
    }
#else
    if (m_pChain == nullptr) {
        // This happens if the skin has stem nodes but Mixxx has no stem support.
        return;
    }
#endif

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
    // Show/hide the chains list
    connect(m_pChain.data(),
            &EffectChain::presetListShowRequest,
            this,
            &WEffectChainPresetSelector::slotPresetListShowRequest);
    // Callback when list is shown/hidden
    connect(this,
            &WEffectChainPresetSelector::presetListVisibleChanged,
            m_pChain.data(),
            &EffectChain::slotPresetListVisibleChanged);

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

    // Show Tooltip-like popup with preset name
    // for hidden main deck Quick Effect chains
    if (!m_bQuickEffectChain || isVisible()) {
        return;
    }

    QWindow* pWindow = mixxx::widgethelper::getWindow(*this);
    if (!pWindow) {
        return;
    }

    // QToolTip::showText(m_lastTopLeft, baseTooltip(), this);
    // only works while the widget is visible.
    // passing nullptr might work, but then we wouldn't inherit the stylesheet
    //
    // Use QLabel.
    auto* pLabel = new QLabel(mixxx::widgethelper::getSkinWidget());
    pLabel->setObjectName("QuickEffectPresetTooltip");
    pLabel->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    const QString tooltip = baseTooltip();
    pLabel->setText(tooltip);
    pLabel->setStyleSheet(QStringLiteral(
            "QLabel { margin: 3px;};"));
    // Show first so the label is rendered and we have a geometry that allows us
    // to position the label at specific points for left/right decks
    pLabel->show();

    QRect geometry = pWindow->geometry();
    // qWarning() << "     geom:" << geometry;

    // Calculate the window center (QRect::center() doesn't seem to work as expected)
    int winCenterY = geometry.top() + static_cast<int>(geometry.height() / 2);
    int winCenterX = geometry.left() + static_cast<int>(geometry.width() / 2);
    // qWarning() << "     cenX:" << winCenterX;
    // qWarning() << "     cenY:" << winCenterY;
    // if left:
    int labelWidth = pLabel->width();
    QPoint topLeft;
    if (m_pChain->getGroup().contains("Channel1]]") ||
            m_pChain->getGroup().contains("Channel3]]")) {
        topLeft = QPoint(winCenterX - labelWidth - 50, winCenterY - 100);
    } else { // Channel2/4
        topLeft = QPoint(winCenterX + 50, winCenterY - 100);
    }

    pLabel->move(topLeft);
    // Auto-hide after 1 second
    QTimer::singleShot(1000,
            Qt::CoarseTimer,
            this,
            [pLabel]() { pLabel->hide(); });
}

void WEffectChainPresetSelector::slotPresetListShowRequest(bool show) {
    if (!isVisible()) {
        return;
    }
    if (show) {
        showPopup();
    } else {
        hidePopup();
    }
}

/// This opens the popup. Overrides showPopup() so we can set the visibility control,
/// both when clicking the down arrow and when triggering the control.
void WEffectChainPresetSelector::showPopup() {
    if (count() > 0) {
        QComboBox::showPopup();
        emit presetListVisibleChanged(true);
    }
}

/// Same as showPopup(), override to set the visibility control for both GUI and
/// control changes
void WEffectChainPresetSelector::hidePopup() {
    QComboBox::hidePopup();
    emit presetListVisibleChanged(false);
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
