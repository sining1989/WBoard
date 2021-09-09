#include <QApplication>

#include "WBMousePressFilter.h"

#include "core/memcheck.h"

WBMousePressFilter::WBMousePressFilter(QObject* parent)
    : QObject(parent)
    , mPendingEvent(0)
{
    // NOOP
}


WBMousePressFilter::~WBMousePressFilter()
{
    // NOOP
}


bool WBMousePressFilter::eventFilter(QObject *obj, QEvent *event)
{
    const bool isMousePress = event->type() == QEvent::MouseButtonPress;
    const bool isMouseRelease = event->type() == QEvent::MouseButtonRelease;
    const bool isMouseMove = event->type() == QEvent::MouseMove;

    if (isMousePress)
    {
        QMouseEvent * mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent)
        {
            if (mPendingEvent)
            {
                delete mPendingEvent;
            }

            mPendingEvent = new QMouseEvent(QEvent::MouseButtonDblClick,
                mouseEvent->pos(), mouseEvent->globalPos(),
                mouseEvent->button(), mouseEvent->buttons(),
                mouseEvent->modifiers());

            mObject = obj;

            QTimer::singleShot(1000, this, SLOT(mouseDownElapsed()));
        }
    }
    else if (isMouseRelease || isMouseMove)
    {
        if (mPendingEvent)
        {
            QTabletEvent * tabletEvent = static_cast<QTabletEvent *>(event);
            QPoint point = tabletEvent->globalPos() - mPendingEvent->globalPos();
            if (isMouseRelease || point.manhattanLength() > QApplication::startDragDistance())
            {
                delete mPendingEvent;
                mPendingEvent = 0;
                mObject = 0;
            }
        }
    }

    return false;
}


void WBMousePressFilter::mouseDownElapsed()
{
    if (mPendingEvent && mObject)
    {
        QApplication::postEvent(mObject, mPendingEvent);
        mPendingEvent = 0;
        mObject = 0;
    }
}
