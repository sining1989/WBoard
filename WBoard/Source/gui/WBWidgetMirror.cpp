#include "WBWidgetMirror.h"

#include "core/memcheck.h"

WBWidgetMirror::WBWidgetMirror(QWidget* sourceWidget, QWidget* parent)
    : QWidget(parent, 0)
    , mSourceWidget(sourceWidget)
{
    mSourceWidget->installEventFilter(this);
}

WBWidgetMirror::~WBWidgetMirror()
{
    // NOOP
}

bool WBWidgetMirror::eventFilter(QObject *obj, QEvent *event)
{
    bool result = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::Paint && obj == mSourceWidget)
    {
        QPaintEvent *paintEvent = static_cast<QPaintEvent *>(event);
        update(paintEvent->rect());
    }

    return result;
}

void WBWidgetMirror::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(Qt::black);

    mSourceWidget->render(&painter, event->rect().topLeft(), QRegion(event->rect()));
}


void WBWidgetMirror::setSourceWidget(QWidget *sourceWidget)
{
    if (mSourceWidget)
    {
        mSourceWidget->removeEventFilter(this);
    }

    mSourceWidget = sourceWidget;

    mSourceWidget->installEventFilter(this);

    update();
}

