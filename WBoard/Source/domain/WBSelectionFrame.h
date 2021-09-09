#ifndef WBSELECTIONFRAME_H
#define WBSELECTIONFRAME_H

#include <QGraphicsRectItem>
#include <QtWidgets>
#include <core/WB.h>

#include "domain/WBGraphicsScene.h"

class DelegateButton;
class WBGraphicsItemDelegate;

class WBSelectionFrame : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    enum {om_idle, om_moving, om_rotating} mOperationMode;
    enum { Type = WBGraphicsItemType::SelectionFrameType };

    WBSelectionFrame();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

    void setLocalBrush(const QBrush &pBrush) {mLocalBrush = pBrush;}
    QBrush localBrush() const {return mLocalBrush;}
//    void setEnclosedItems(const QList<WBGraphicsItemDelegate*> pEncItems) {mEnclosedtems = pEncItems; updateRect();}
    void setEnclosedItems(const QList<QGraphicsItem*> pGraphicsItems);
    void updateRect();
    void updateScale();
    bool isEmpty() const {return this->rect().isEmpty();}
    virtual int type() const {return Type;}

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void setAntiScaleRatio(qreal pAntiscaleRatio) {mAntiscaleRatio = pAntiscaleRatio;}
    void onZoomChanged(qreal pZoom);
    void remove();
    void duplicate();
    void increaseZlevelUp();
    void increaseZlevelTop();
    void increaseZlevelDown();
    void increaseZlevelBottom();
    void groupItems();

private:
    void addSelectionUndo(QList<QGraphicsItem*> items, WBZLayerController::moveDestination dest);
    void placeButtons();
    void placeExceptionButton(DelegateButton *pButton, QTransform pTransform);
    void clearButtons();
    inline int adjThickness() const {return mThickness * mAntiscaleRatio;}
    inline WBGraphicsScene* ubscene();
    void setCursorFromAngle(QString angle);

    QList<QGraphicsItem*> sortedByZ(const QList<QGraphicsItem*> &pItems);
    QList<DelegateButton*> buttonsForFlags(WBGraphicsFlags fls);

    QList<QGraphicsItem*> enclosedGraphicsItems();

private:
    int mThickness;
    qreal mAntiscaleRatio;
    QList<WBGraphicsItemDelegate*> mEnclosedtems;
    QBrush mLocalBrush;

    QPointF mPressedPos;
    QPointF mLastMovedPos;
    QPointF mLastTranslateOffset;
    qreal mRotationAngle;

    bool mIsLocked;

    QList<DelegateButton*> mButtons;

    DelegateButton *mDeleteButton;
    DelegateButton *mDuplicateButton;
    DelegateButton *mZOrderUpButton;
    DelegateButton *mZOrderDownButton;
    DelegateButton *mGroupButton;

    DelegateButton *mRotateButton;

};

#endif // WBSELECTIONFRAME_H
