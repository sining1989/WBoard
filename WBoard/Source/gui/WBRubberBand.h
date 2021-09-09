#ifndef WBRUBBERBAND_H_
#define WBRUBBERBAND_H_

#include <QRubberBand>

class WBRubberBand : public QRubberBand
{
    Q_OBJECT

public:
    WBRubberBand(Shape s, QWidget * p = 0);
    virtual ~WBRubberBand();

private:
    enum enm_resizingMode
    {
        None,
        Top,
        TopLeft,
        TopRight,
        Bottom,
        BottomLeft,
        BottomRight,
        Left,
        Right
    };

    enm_resizingMode determineResizingMode(QPoint pos);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    QStyle* customStyle;
    enm_resizingMode mResizingMode;
    int mResizingBorderHeight;
    bool mMouseIsPressed;
    QPoint mLastPressedPoint;
    QPoint mLastMousePos;
};

#endif /* WBRUBBERBAND_H_ */
