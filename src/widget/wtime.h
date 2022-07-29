// WTime is a widget showing the current time
// In skins it is represented by a <Time> node.

#pragma once

#include <QTimer>

#include "control/controlproxy.h"
#include "preferences/constants.h"
#include "skin/legacy/skincontext.h"
#include "widget/wlabel.h"

class WTime: public WLabel {
    Q_OBJECT
  public:
    explicit WTime(QWidget *parent=nullptr);
    ~WTime() override;

    void setup(const QDomNode& node, const SkinContext& context) override;

  private slots:
    void refreshTime();
    void switchTimeFormat(double format);

  private:
    void setTimeFormat(const QDomNode& node, const SkinContext& context);

    QTimer* m_pTimer;
    mixxx::ClockFormat m_eTimeFormat;
    QString m_sTimeFormat;
    ControlProxy* m_timeFormatControl;
    ControlProxy* m_allowSecondsControl;

    // m_iInterval defines how often the time will be updated
    short m_iInterval;
    // m_iInterval is set to s_iSecondInterval if seconds are shown
    // otherwise, m_iInterval = s_iMinuteInterval
    static constexpr short s_iSecondInterval = 100;
    static constexpr short s_iMinuteInterval = 1000;
};
