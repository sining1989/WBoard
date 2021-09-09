#ifndef UBSCREENMIRROR_H_
#define UBSCREENMIRROR_H_

#include <QtWidgets>
#include <QWidget>

class WBScreenMirror : public QWidget
{
    Q_OBJECT

public:
    WBScreenMirror(QWidget* parent = 0);
    virtual ~WBScreenMirror();

    virtual void paintEvent (QPaintEvent * event);
    virtual void timerEvent(QTimerEvent *event);

public slots:

    void setSourceWidget(QWidget *sourceWidget);

    void setSourceRect(const QRect& pRect)
    {
        mRect = pRect;
        mSourceWidget = 0;
    }

    void start();

    void stop();

private:

    void grabPixmap();

    int mScreenIndex;

    QWidget* mSourceWidget;

    QRect mRect;

    QPixmap mLastPixmap;

    long mTimerID;

};

#endif /* UBSCREENMIRROR_H_ */
