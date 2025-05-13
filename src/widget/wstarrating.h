#pragma once

#include "library/starrating.h"
#include "widget/wwidget.h"

class QDomNode;
class SkinContext;

class WStarRating : public WWidget {
    Q_OBJECT
  public:
    WStarRating(QWidget* pParent);

    virtual void setup(const QDomNode& node, const SkinContext& context);
    QSize sizeHint() const override;

    int getRating() {
        return m_starCount;
    }
    void incRating() {
        slotSetRating(m_starCount + 1);
    }
    void decRating() {
        slotSetRating(m_starCount - 1);
    }

  public slots:
    void slotSetRating(int starCount);

  signals:
    void ratingChangeRequest(int starCount);

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
};
