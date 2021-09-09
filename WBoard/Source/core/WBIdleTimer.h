#ifndef WBIDLETIMER_H_
#define WBIDLETIMER_H_

#include <QObject>
#include <QDateTime>

class QEvent;

class WBIdleTimer : public QObject
{
    Q_OBJECT

    public:
        WBIdleTimer(QObject *parent = 0);
        virtual ~WBIdleTimer();

    protected:
        bool eventFilter(QObject *obj, QEvent *event);
        virtual void timerEvent(QTimerEvent *event);

    private:
        QDateTime mLastInputEventTime;
        bool mCursorIsHidden;
};
#endif /* WBIDLETIMER_H_ */
