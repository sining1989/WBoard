#ifndef WBFLOATINGPALLETTE_H_
#define WBFLOATINGPALLETTE_H_

#include <QWidget>
#include <QPoint>

typedef enum
{
    eMinimizedLocation_None,
    eMinimizedLocation_Left,
    eMinimizedLocation_Top,
    eMinimizedLocation_Right,
    eMinimizedLocation_Bottom
}eMinimizedLocation;

class WBFloatingPalette : public QWidget
{
    Q_OBJECT

public:
    WBFloatingPalette(Qt::Corner = Qt::TopLeftCorner, QWidget *parent = 0);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    void addAssociatedPalette(WBFloatingPalette* pOtherPalette);
    void removeAssociatedPalette(WBFloatingPalette* pOtherPalette);

    virtual void adjustSizeAndPosition(bool pUp = true);

    void setCustomPosition(bool pFlag);

    QSize preferredSize();

    void setBackgroundBrush(const QBrush& brush);
    void setGrip(bool newGrip);

    void setMinimizePermission(bool permission);

protected:
    virtual void enterEvent(QEvent *event);
    virtual void showEvent(QShowEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    virtual int radius();
    virtual int border();
    virtual int gripSize();

    QBrush mBackgroundBrush;
    bool mbGrip;
    static const int sLayoutContentMargin = 12;
    static const int sLayoutSpacing = 15;
    void moveInsideParent(const QPoint &position);
    bool mCustomPosition;
    bool mIsMoving;

    virtual int getParentRightOffset();

    eMinimizedLocation minimizedLocation(){return mMinimizedLocation;}

private:
    void removeAllAssociatedPalette();
    void minimizePalette(const QPoint& pos);

    QList<WBFloatingPalette*> mAssociatedPalette;
    QPoint mDragPosition;
    bool mCanBeMinimized;
    eMinimizedLocation mMinimizedLocation;
    Qt::Corner mDefaultPosition;

signals:
    void mouseEntered();
    void minimizeStart(eMinimizedLocation location);
    void maximizeStart();
    void maximized();
    void moving();
};

#endif /* WBFLOATINGPALLETTE_H_ */
