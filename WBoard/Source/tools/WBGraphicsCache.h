#ifndef WBGRAPHICSCACHE_H
#define WBGRAPHICSCACHE_H

#include <QColor>
#include <QGraphicsSceneMouseEvent>

#include "domain/WBItem.h"
#include "core/WB.h"

typedef enum
{
    eMaskShape_Circle,
    eMaskShap_Rectangle
}eMaskShape;

class WBGraphicsCache : public QGraphicsRectItem, public WBItem
{
public:
    static WBGraphicsCache* instance(WBGraphicsScene *scene);
    ~WBGraphicsCache();

    enum { Type = WBGraphicsItemType::cacheItemType };

    virtual int type() const{ return Type;}

    virtual WBItem* deepCopy() const;

    virtual void copyItemParameters(WBItem *copy) const;

    QColor maskColor();
    void setMaskColor(QColor color);
    eMaskShape maskshape();
    void setMaskShape(eMaskShape shape);
    int shapeWidth();
    void setShapeWidth(int width);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    static QMap<WBGraphicsScene*, WBGraphicsCache*> sInstances;

    QColor mMaskColor;
    eMaskShape mMaskShape;
    int mShapeWidth;
    bool mDrawMask;
    QPointF mShapePos;
    int mOldShapeWidth;
    QPointF mOldShapePos;
    WBGraphicsScene* mScene;

    WBGraphicsCache(WBGraphicsScene *scene);
    
    void init();
    QRectF updateRect(QPointF currentPoint);
};

#endif // WBGRAPHICSCACHE_H
