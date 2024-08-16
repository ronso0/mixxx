#pragma once

#include <QWidgetAction>

#include "util/parented_ptr.h"

class WStarRating;

class WStarRatingAction : public QWidgetAction {
    Q_OBJECT
  public:
    explicit WStarRatingAction(QWidget* parent = nullptr);

    void setRating(const int rating);

    QSize sizeHint();

    int getRating();
    void incRating();
    void decRating();

  signals:
    void ratingSet(const int rating);

  private:
    parented_ptr<WStarRating> m_pStarRating;
};
