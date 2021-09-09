#ifndef WBSPINNINGWHEEL_H_
#define WBSPINNINGWHEEL_H_

#include <QWidget>

class WBSpinningWheel : public QWidget
{
    Q_OBJECT

public:
    WBSpinningWheel(QWidget *parent = 0);
    virtual ~WBSpinningWheel();
    virtual void paintEvent(QPaintEvent *event);

public slots:
    void startAnimation();
    void stopAnimation();

protected:
    virtual QSize sizeHint() const;
    virtual void timerEvent(QTimerEvent *event);

private:
    QAtomicInt mPosition;
    int mTimerID;
};

#endif /* WBSPINNINGWHEEL_H_ */

