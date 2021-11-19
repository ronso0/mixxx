#pragma once

#include "engine/enginesidechaincompressor.h"
#include "control/controlpotmeter.h"
#include "control/controlpushbutton.h"

class ConfigValue;
class ControlProxy;

class EngineTalkoverDucking : public QObject, public EngineSideChainCompressor {
  Q_OBJECT
  public:

    enum TalkoverDuckSetting {
        OFF = 0,
        AUTO,
        MANUAL,
    };

    EngineTalkoverDucking(UserSettingsPointer pConfig, const QString& group);
    virtual ~EngineTalkoverDucking();

    TalkoverDuckSetting getMode() const {
        return static_cast<TalkoverDuckSetting>(int(m_pTalkoverDucking->get()));
    }

    CSAMPLE getGain(int numFrames);

  public slots:
    void slotSampleRateChanged(double);
    void slotDuckStrengthChanged(double);
    void slotDuckAttackTimeChanged(double);
    void slotDuckDecayTimeChanged(double);
    void slotDuckModeChanged(double);

  private:
    UserSettingsPointer m_pConfig;
    const QString m_group;

    ControlProxy* m_pSampleRate;
    ControlPotmeter* m_pDuckStrength;
    ControlPotmeter* m_pDuckAttackTime;
    ControlPotmeter* m_pDuckDecayTime;
    ControlPushButton* m_pTalkoverDucking;
};
