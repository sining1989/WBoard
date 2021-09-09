#include "WBIdleTimer.h"

#include <QApplication>
#include <QInputEvent>
#include <QTimer>
#include <QWidget>

#include "WBApplication.h"
#include "WBApplicationController.h"
#include "board/WBBoardController.h"
#include "board/WBBoardView.h"

#include "core/memcheck.h"

WBIdleTimer::WBIdleTimer(QObject *parent)
     : QObject(parent)
     , mCursorIsHidden(false)
{
    startTimer(100);
}

WBIdleTimer::~WBIdleTimer()
{
    // NOOP
}

bool WBIdleTimer::eventFilter(QObject *obj, QEvent *event)
{
    // if the event is an input event (mouse / tablet / keyboard)
    if (dynamic_cast<QInputEvent*>(event))
    {
        mLastInputEventTime = QDateTime::currentDateTime();
        if (mCursorIsHidden)
        {
            QApplication::restoreOverrideCursor();
            mCursorIsHidden = false;
        }
    }

    return QObject::eventFilter(obj, event);
}

void WBIdleTimer::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

        bool hasWaitCuror = (WBApplication::overrideCursor() &&
                (WBApplication::overrideCursor()->shape() == Qt::WaitCursor));

    if (!mCursorIsHidden
            && (mLastInputEventTime.secsTo(QDateTime::currentDateTime()) >= 2)
                        && WBApplication::boardController
                        && WBApplication::boardController->controlView()
                        && WBApplication::boardController->controlView()->hasFocus()
                        && !hasWaitCuror)
    {
        QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        mCursorIsHidden = true;
    }
}
