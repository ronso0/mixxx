#include "widget/wtime.h"

#include <QLocale>
#include <QTime>
#include <QtDebug>

#include "moc_wtime.cpp"
#include "util/cmdlineargs.h"

namespace {
// TODO add note why not to use QLocale::LongFormat
const QString kShowSecondsFormat = QStringLiteral("h:mm:ss");
const QString kNoSecondsFormat = QStringLiteral("h:mm");
} //anonymous namespace

WTime::WTime(QWidget* parent)
        : WLabel(parent),
          m_eTimeFormat(mixxx::ClockFormat::HideSeconds),
          m_sTimeFormat(kNoSecondsFormat),
          m_iInterval(s_iMinuteInterval) {
    m_pTimer = new QTimer(this);
}

WTime::~WTime() {
    delete m_pTimer;
}

void WTime::setup(const QDomNode& node, const SkinContext& context) {
    WLabel::setup(node, context);
    setTimeFormat(node, context);
    m_pTimer->start(m_iInterval);
    connect(m_pTimer, &QTimer::timeout, this, &WTime::refreshTime);
    refreshTime();
}

void WTime::setTimeFormat(const QDomNode& node, const SkinContext& context) {
    // if a custom format is defined, all other formatting flags are ignored
    // and the respective toggle in Preferences > Interface is disabled
    m_allowSecondsControl = new ControlProxy(
            ConfigKey("[Controls]", "clock_allow_seconds"), this);

    QString customFormat;
    if (context.hasNodeSelectString(node, "CustomFormat", &customFormat)) {
        // set the time format to the custom format
        // Any resolution smaller than seconds does not work since m_iInterval
        // would need to be adjusted in order to yield appropriate updates
        // (doesn't make much sense anyway to show fractional seconds on the clock)
        m_sTimeFormat = customFormat;
        return;
    }

    // check if seconds should be shown or hidden explicitly
    QString secondsFormat = context.selectString(node, "ShowSeconds");
    if (secondsFormat == "true" || secondsFormat == "yes") {
        m_sTimeFormat = kShowSecondsFormat;
        m_iInterval = s_iSecondInterval;
        return;
    }
    if (secondsFormat == "false" || secondsFormat == "no") {
        m_sTimeFormat = kNoSecondsFormat;
        m_iInterval = s_iMinuteInterval;
        return;
    }

    // Otherwise allow Preferences > Interface to switch the format, connect
    // value change signals and set format interval and format string accordingly
    m_allowSecondsControl->set(1.0);
    m_timeFormatControl = new ControlProxy(
            ConfigKey("[Controls]", "clock_show_seconds"), this);
    m_timeFormatControl->connectValueChanged(this, &WTime::switchTimeFormat);
    switchTimeFormat(m_timeFormatControl->get());
}

void WTime::switchTimeFormat(double format) {
    mixxx::ClockFormat newFormat = static_cast<mixxx::ClockFormat>(format);
    if (newFormat == m_eTimeFormat) {
        return;
    }

    m_eTimeFormat = newFormat;
    if (m_eTimeFormat == mixxx::ClockFormat::ShowSeconds) {
        m_sTimeFormat = kShowSecondsFormat;
        m_iInterval = s_iSecondInterval;
    } else {
        m_sTimeFormat = kNoSecondsFormat;
        m_iInterval = s_iMinuteInterval;
    }
    m_pTimer->stop();
    m_pTimer->start(m_iInterval);
}

void WTime::refreshTime() {
    QTime time = QTime::currentTime();
    QString timeString = time.toString(m_sTimeFormat);
    if (text() != timeString) {
        setText(timeString);
        //if (CmdlineArgs::Instance().getDeveloper()) {
        //    qDebug() << "WTime::refreshTime" << timeString << font().family();
        //}
    }
}
