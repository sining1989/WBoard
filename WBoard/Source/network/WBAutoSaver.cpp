#include "WBAutoSaver.h"

#include <QtCore>

#include "core/memcheck.h"

#define AUTOSAVE_IN  1000 * 3  // seconds
#define MAXWAIT      1000 * 15 // seconds

WBAutoSaver::WBAutoSaver(QObject *parent) : QObject(parent)
{
    Q_ASSERT(parent);
}


WBAutoSaver::~WBAutoSaver()
{
    if (mTimer.isActive())
        qWarning() << "AutoSaver: still active when destroyed, changes not saved.";
}


void WBAutoSaver::changeOccurred()
{
    if (mFirstChange.isNull())
        mFirstChange.start();

    if (mFirstChange.elapsed() > MAXWAIT)
    {
        saveIfNeccessary();
    } else {
        mTimer.start(AUTOSAVE_IN, this);
    }
}


void WBAutoSaver::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mTimer.timerId())
    {
        saveIfNeccessary();
    }
    else
    {
        QObject::timerEvent(event);
    }
}


void WBAutoSaver::saveIfNeccessary()
{
    if (!mTimer.isActive())
        return;

    mTimer.stop();
    mFirstChange = QTime();

    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection))
    {
        qWarning() << "AutoSaver: error invoking slot save() on parent";
    }
}

