#pragma once

#include <QPushButton>
#include <QTimer>

#include "control/pollingcontrolproxy.h"
#include "util/parented_ptr.h"
#include "widget/wwidget.h"

class QPushButton;
class QDoubleSpinBox;
class QStackedLayout;
class SkinContext;

// Custom PushButton which emits a custom signal when right-clicked
class TapPushButton : public QPushButton {
    Q_OBJECT
  public:
    explicit TapPushButton(QWidget* parent = 0)
            : QPushButton(parent) {
    }

  protected:
    void mousePressEvent(QMouseEvent* e) override;

  signals:
    void rightClicked();
};

class WBpmEditor : public WWidget {
    Q_OBJECT
  public:
    explicit WBpmEditor(const QString& group, QWidget* pParent = nullptr);

    void setup(const QDomNode& node, const SkinContext& context);

    enum class Mode {
        Listen,
        Select,
        Tap,
        Edit,
    };

    void setTapButtonTooltip(const QString& tooltip);
    void setEditButtonTooltip(const QString& tooltip);

  private slots:
    void slotEditigFinished();
    void switchMode(Mode mode);

  private:
    bool eventFilter(QObject* pObj, QEvent* pEvent) override;

    QString m_group;

    PollingControlProxy m_tempoTapCO;
    PollingControlProxy m_bpmTapCO;
    PollingControlProxy m_trackLoadedCO;
    PollingControlProxy m_bpmCO;
    PollingControlProxy m_fileBpmCO;
    PollingControlProxy m_rateRatioCO;

    parented_ptr<QStackedLayout> m_pModeLayout;
    std::unique_ptr<QPushButton> m_pClickOverlay;
    std::unique_ptr<QWidget> m_pSelectWidget;
    std::unique_ptr<QPushButton> m_pTapSelectButton;
    std::unique_ptr<QPushButton> m_pEditSelectButton;
    std::unique_ptr<TapPushButton> m_pTapButton;
    std::unique_ptr<QDoubleSpinBox> m_pEditBox;

    QTimer m_hideTimer;
};
