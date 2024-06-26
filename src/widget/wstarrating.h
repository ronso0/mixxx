#pragma once

#include "control/controlpushbutton.h"
#include "library/starrating.h"
#include "widget/wwidget.h"

class ControlObject;
class ControlPushButton;
class QDomNode;
class SkinContext;

class WStarRating : public WWidget {
    Q_OBJECT
  public:
    WStarRating(const QString& group, QWidget* pParent);

    virtual void setup(const QDomNode& node, const SkinContext& context);
    QSize sizeHint() const override;

  public slots:
    void slotSetRating(int starCount);

  signals:
    void ratingChanged(int starCount);

  private slots:
    void slotStarsUp(double v);
    void slotStarsDown(double v);

  protected:
    void paintEvent(QPaintEvent* e) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent * /*unused*/) override;
    void fillDebugTooltip(QStringList* debug) override;

  private:
    int m_starCount;

    StarRating m_visualStarRating;
    mutable QRect m_contentRect;

    void updateVisualRating(int starCount);
    void resetVisualRating() {
        updateVisualRating(m_starCount);
    }

    std::unique_ptr<ControlPushButton> m_pStarsUp;
    std::unique_ptr<ControlPushButton> m_pStarsDown;
};
