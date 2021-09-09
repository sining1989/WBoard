#ifndef WBCHASEWIDGET_H
#define WBCHASEWIDGET_H

#include <QtWidgets>
#include <QWidget>


class WBChaseWidget : public QWidget
{
    Q_OBJECT

public:
    WBChaseWidget(QWidget *parent = 0, QPixmap pixmap = QPixmap(), bool pixmapEnabled = false);

    void setAnimated(bool value);
    void setPixmapEnabled(bool enable);
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    int segmentCount() const;
    QColor colorForSegment(int segment) const;

    int mSegment;
    int mDelay;
    int mStep;
    int mTimerID;
    bool mAnimated;
    QPixmap mPixmap;
    bool mPixmapEnabled;
};

#endif //WBCHASEWIDGET_H
