#pragma once

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMouseEvent>

#include "preferences/dialog/dlgprefdeck.h"
#include "preferences/usersettings.h"
#include "widget/trackdroptarget.h"
#include "wnumber.h"

class ControlProxy;

class WNumberPos : public WNumber, public TrackDropTarget {
    Q_OBJECT

  public:
    explicit WNumberPos(QWidget* parent,
            const QString& group,
            UserSettingsPointer config);

  signals:
    void trackDropped(const QString& filename, const QString& group) override;
    void cloneDeck(const QString& sourceGroup, const QString& targetGroup) override;

  protected:
    void mousePressEvent(QMouseEvent* pEvent) override;

  private slots:
    void setValue(double dValue) override;
    void slotSetTimeElapsed(double);
    void slotTimeRemainingUpdated(double);
    void slotSetDisplayMode(double);
    void slotSetTimeFormat(double);

  private:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    const QString m_group;
    const UserSettingsPointer m_pConfig;

    TrackTime::DisplayMode m_displayMode;
    TrackTime::DisplayFormat m_displayFormat;

    double m_dOldTimeElapsed;
    ControlProxy* m_bTrackLoaded;
    ControlProxy* m_pTimeElapsed;
    ControlProxy* m_pTimeRemaining;
    ControlProxy* m_pShowTrackTimeRemaining;
    ControlProxy* m_pTimeFormat;
};
