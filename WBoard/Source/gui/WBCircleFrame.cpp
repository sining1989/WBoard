#include "WBCircleFrame.h"
#include "core/memcheck.h"

WBCircleFrame::WBCircleFrame(QWidget* parent)
    : QFrame(parent)
{
    // NOOP
}

WBCircleFrame::~WBCircleFrame()
{
    // NOOP
}


void WBCircleFrame::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::black));

    qreal diameter = (width() - 2) / maxPenWidth * currentPenWidth;
    diameter = qMax(2.0, diameter);
    qreal x = (width() - diameter) / 2;

    painter.drawEllipse(x, x, diameter, diameter);
}
