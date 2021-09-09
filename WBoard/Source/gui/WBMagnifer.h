#ifndef WBMAGNIFIER_H
#define WBMAGNIFIER_H

#include <QtWidgets>
#include <QWidget>

class WBMagnifierParams
{
public :
    int x;
    int y;
    qreal zoom;
    qreal sizePercentFromScene;
};

class WBMagnifier : public QWidget
{
    Q_OBJECT
    
public:
    enum DrawingMode
    {
        circular = 0,
        rectangular,
        modesCount
    };

public:
    WBMagnifier(QWidget *parent = 0, bool isInteractive = false);
    ~WBMagnifier();

    void setSize(qreal percentFromScene);
    void createMask();
    void setZoom(qreal zoom);

    void setGrabView(QWidget *view);
    void setMoveView(QWidget *view) {mView = view;}
    void setDrawingMode(int mode);

    void grabPoint();
    void grabPoint(const QPoint &point);
    void grabNMove(const QPoint &pGrab, const QPoint &pMove, bool needGrab = true, bool needMove = true);

    WBMagnifierParams params;

signals:
    void magnifierMoved_Signal(QPoint newPos);
    void magnifierClose_Signal();
    void magnifierZoomIn_Signal();
    void magnifierZoomOut_Signal();
    void magnifierResized_Signal(qreal newPercentSize);
    void magnifierDrawingModeChange_Signal(int mode);
    
public slots:
    void slot_refresh();

private:
    void calculateButtonsPositions();

protected:
    void paintEvent(QPaintEvent *);

    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );

    QPoint mMousePressPos;
    qreal mMousePressDelta;
    bool mShouldMoveWidget;
    bool mShouldResizeWidget;

    int m_iButtonInterval;
    QPixmap *sClosePixmap;
    QRect sClosePixmapButtonRect;
    QPixmap *sIncreasePixmap;
    QRect sIncreasePixmapButtonRect;
    QPixmap *sDecreasePixmap;
    QRect sDecreasePixmapButtonRect;
    QPixmap *sChangeModePixmap;
    QRect sChangeModePixmapButtonRect;
    QPixmap *mResizeItem;
    QRect mResizeItemButtonRect;

    bool isCusrsorAlreadyStored;
    QCursor mOldCursor;
    QCursor mResizeCursor;

private:
    DrawingMode mDrawingMode;

    QTimer mRefreshTimer;
    bool m_isInteractive;

    QPoint updPointGrab;
    QPoint updPointMove;
    
    QPixmap pMap;
    QBitmap bmpMask;
    QPen borderPen;

    QWidget *gView;
    QWidget *mView;
};

#endif // WBMAGNIFIER_H
