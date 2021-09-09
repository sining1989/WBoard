#ifndef WBMOUSEPRESSFILTER_H_
#define WBMOUSEPRESSFILTER_H_

#include <QtWidgets>

class WBMousePressFilter : public QObject
{
    Q_OBJECT

    public:
        WBMousePressFilter(QObject* parent = 0);
        virtual ~WBMousePressFilter();

    protected:
        bool eventFilter(QObject *obj, QEvent *event);

    protected slots:
        void mouseDownElapsed();

    private:
        QObject* mObject;
        QMouseEvent* mPendingEvent;
};

#endif /* WBMOUSEPRESSFILTER_H_ */
