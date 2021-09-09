#include "WBChaseWidget.h"

#include <QtWidgets>
#include <QApplication>

#include "core/memcheck.h"

WBChaseWidget::WBChaseWidget(QWidget *parent, QPixmap pixmap, bool pixmapEnabled)
    : QWidget(parent)
    , mSegment(0)
    , mDelay(100)
    , mStep(40)
    , mTimerID(-1)
    , mAnimated(false)
    , mPixmap(pixmap)
    , mPixmapEnabled(pixmapEnabled)
{
    // NOOP
}


void WBChaseWidget::setAnimated(bool value)
{
    if (mAnimated == value)
        return;
    mAnimated = value;
    if (mTimerID != -1) 
    {
        killTimer(mTimerID);
        mTimerID = -1;
    }
    if (mAnimated) 
    {
        mSegment = 0;
        mTimerID = startTimer(mDelay);
    }
    update();
}


void WBChaseWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    if (mPixmapEnabled && !mPixmap.isNull()) 
    {
        p.drawPixmap(0, 0, mPixmap);
        return;
    }

    const int extent = qMin(width() - 8, height() - 8);
    const int displ = extent / 4;
    const int ext = extent / 4 - 1;

    p.setRenderHint(QPainter::Antialiasing, true);

    if(mAnimated)
        p.setPen(Qt::gray);
    else
        p.setPen(QPen(palette().dark().color()));

    p.translate(width() / 2, height() / 2); // center

    for (int segment = 0; segment < segmentCount(); ++segment) 
    {
        p.rotate(QApplication::isRightToLeft() ? mStep : -mStep);
        if(mAnimated)
            p.setBrush(colorForSegment(segment));
        else
            p.setBrush(palette().background());
        p.drawEllipse(QRect(displ, -ext / 2, ext, ext));
    }
}


QSize WBChaseWidget::sizeHint() const
{
    return QSize(32, 32);
}


void WBChaseWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mTimerID) 
    {
        ++mSegment;
        update();
    }
    QWidget::timerEvent(event);
}


QColor WBChaseWidget::colorForSegment(int seg) const
{
    int index = ((seg + mSegment) % segmentCount());
    int comp = qMax(0, 255 - (index * (255 / segmentCount())));
    return QColor(comp, comp, comp, 255);
}


int WBChaseWidget::segmentCount() const
{
    return 360 / mStep;
}


void WBChaseWidget::setPixmapEnabled(bool enable)
{
    mPixmapEnabled = enable;
}

