#pragma once

#include <QComboBox>

#include "effects/defs.h"
#include "widget/wbasewidget.h"

class EffectsManager;
class QDomNode;
class SkinContext;

class WEffectSelector : public QComboBox, public WBaseWidget {
    Q_OBJECT
  public:
    WEffectSelector(QWidget* pParent, EffectsManager* pEffectsManager);

    Q_PROPERTY(bool effectEnabled READ isEffectEnabled WRITE slotEffectEnabledChanged
                    NOTIFY effectEnabledChanged);

    bool isEffectEnabled() const {
        return m_effectEnabled;
    }

    void setup(const QDomNode& node, const SkinContext& context);

  signals:
    void effectEnabledChanged(bool enabled);

  private slots:
    void slotEffectUpdated();
    void slotEffectSelected(int newIndex);
    void slotEffectEnabledChanged(bool enabled);
    void populate();
    bool event(QEvent* pEvent) override;

  private:
    EffectsManager* m_pEffectsManager;
    VisibleEffectsListPointer m_pVisibleEffectsList;
    EffectSlotPointer m_pEffectSlot;
    bool m_effectEnabled;
};
