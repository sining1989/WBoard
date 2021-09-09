#ifndef WBCIRCLEFRAME_H_
#define WBCIRCLEFRAME_H_

#include <QtWidgets>
#include <QFrame>

class WBCircleFrame : public QFrame
{
    public:
        WBCircleFrame(QWidget* parent);
        virtual ~WBCircleFrame();

        qreal currentPenWidth;
        qreal maxPenWidth;

    protected:
        virtual void paintEvent (QPaintEvent * event);
};

#endif /* WBCIRCLEFRAME_H_ */
