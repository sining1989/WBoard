#include <QDebug>

#include "WBGraphicsCache.h"

#include "core/WBApplication.h"

#include "board/WBBoardController.h"
#include "board/WBBoardView.h"
#include "domain/WBGraphicsScene.h"

#include "core/memcheck.h"

QMap<WBGraphicsScene*, WBGraphicsCache*> WBGraphicsCache::sInstances;

WBGraphicsCache* WBGraphicsCache::instance(WBGraphicsScene *scene)
{
    if (!sInstances.contains(scene))
        sInstances.insert(scene, new WBGraphicsCache(scene));
    return sInstances[scene];
}

WBGraphicsCache::WBGraphicsCache(WBGraphicsScene *scene) : QGraphicsRectItem()
  , mMaskColor(Qt::black)
  , mMaskShape(eMaskShape_Circle)
  , mShapeWidth(100)
  , mDrawMask(false)
  , mScene(scene)
{
    // Get the board size and pass it to the shape
    QRect boardRect = WBApplication::boardController->displayView()->rect();
    setRect(-15*boardRect.width(), -15*boardRect.height(), 30*boardRect.width(), 30*boardRect.height());
    setData(Qt::UserRole, QVariant("Cache"));
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Cache)); //Necessary to set if we want z value to be assigned correctly
}

WBGraphicsCache::~WBGraphicsCache()
{
    sInstances.remove(mScene);
}

WBItem* WBGraphicsCache::deepCopy() const
{
    WBGraphicsCache* copy = new WBGraphicsCache(mScene);

    copyItemParameters(copy);

    return copy;
}

void WBGraphicsCache::copyItemParameters(WBItem *copy) const
{
    WBGraphicsCache *cp = dynamic_cast<WBGraphicsCache*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());
    }
}

QColor WBGraphicsCache::maskColor()
{
    return mMaskColor;
}

void WBGraphicsCache::setMaskColor(QColor color)
{
    mMaskColor = color;
    update();
}

eMaskShape WBGraphicsCache::maskshape()
{
    return mMaskShape;
}

void WBGraphicsCache::setMaskShape(eMaskShape shape)
{
    mMaskShape = shape;
    update();
}

void WBGraphicsCache::init()
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void WBGraphicsCache::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(mMaskColor);
    painter->setPen(mMaskColor);

    // Draw the hole
    QPainterPath path;
    path.addRect(rect());

    if(mDrawMask)
    {
        if(eMaskShape_Circle == mMaskShape)
        {
            path.addEllipse(mShapePos, mShapeWidth, mShapeWidth);
        }
        else if(eMaskShap_Rectangle == mMaskShape)
        {
            path.addRect(mShapePos.x() - mShapeWidth, mShapePos.y() - mShapeWidth, 2*mShapeWidth, 2*mShapeWidth);
        }
        path.setFillRule(Qt::OddEvenFill);
    }

    painter->drawPath(path);
}

void WBGraphicsCache::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    mShapePos = event->pos();
    mDrawMask = true;

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

void WBGraphicsCache::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    mShapePos = event->pos();

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

void WBGraphicsCache::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    mDrawMask = false;

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

int WBGraphicsCache::shapeWidth()
{
    return mShapeWidth;
}

void WBGraphicsCache::setShapeWidth(int width)
{
    mShapeWidth = width;
    update();
}

QRectF WBGraphicsCache::updateRect(QPointF currentPoint)
{
    QRectF r;
    int x;
    int y;

    x = qMin(currentPoint.x() - mShapeWidth, mOldShapePos.x() - mOldShapeWidth);
    y = qMin(currentPoint.y() - mShapeWidth, mOldShapePos.y() - mOldShapeWidth);
    r = QRect(  x,
                y,
                qAbs(currentPoint.x() - mOldShapePos.x()) + 2*mShapeWidth,
                qAbs(currentPoint.y() - mOldShapePos.y()) + 2*mShapeWidth);
    return r;
}
