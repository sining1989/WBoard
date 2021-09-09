#include "WBGraphicsPDFItem.h"

#include <QtWidgets>

#include "WBGraphicsScene.h"
#include "WBGraphicsPixmapItem.h"
#include "WBGraphicsItemDelegate.h"

#include "core/memcheck.h"

WBGraphicsPDFItem::WBGraphicsPDFItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem* parent)
    : GraphicsPDFItem(renderer, pageNumber, parent)
{
    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object); //deprecated
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::BackgroundItem)); //Necessary to set if we want z value to be assigned correctly

    setDelegate(new WBGraphicsItemDelegate(this, 0, GF_COMMON));
}


WBGraphicsPDFItem::~WBGraphicsPDFItem()
{
}


QVariant WBGraphicsPDFItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return GraphicsPDFItem::itemChange(change, newValue);
}

void WBGraphicsPDFItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid));
}

void WBGraphicsPDFItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        // NOOP
    }
    else
    {
        GraphicsPDFItem::mousePressEvent(event);
    }
}


void WBGraphicsPDFItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP
    }
    else
    {
        GraphicsPDFItem::mouseMoveEvent(event);
    }
}


void WBGraphicsPDFItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    GraphicsPDFItem::mouseReleaseEvent(event);
}

void WBGraphicsPDFItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    GraphicsPDFItem::paint(painter, option, widget);
    Delegate()->postpaint(painter, option, widget);
}

WBItem* WBGraphicsPDFItem::deepCopy() const
{
    WBGraphicsPDFItem *copy =  new WBGraphicsPDFItem(mRenderer, mPageNumber, parentItem());

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;
}

void WBGraphicsPDFItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsPDFItem *cp = dynamic_cast<WBGraphicsPDFItem*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setSourceUrl(this->sourceUrl());
        cp->setZValue(this->zValue());
    }
}

void WBGraphicsPDFItem::setRenderingQuality(RenderingQuality pRenderingQuality)
{
    WBItem::setRenderingQuality(pRenderingQuality);

    if (pRenderingQuality == RenderingQualityHigh)
    {
        setCacheMode(QGraphicsItem::NoCache);
    }
    else
    {
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
}


WBGraphicsScene* WBGraphicsPDFItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}


WBGraphicsPixmapItem* WBGraphicsPDFItem::toPixmapItem() const
{   
    QPixmap pixmap(mRenderer->pageSizeF(mPageNumber).toSize());
    QPainter painter(&pixmap);
    mRenderer->render(&painter, mPageNumber);

    WBGraphicsPixmapItem *pixmapItem =  new WBGraphicsPixmapItem();
    pixmapItem->setPixmap(pixmap);

    pixmapItem->setPos(this->pos());
    pixmapItem->setTransform(this->transform());
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));

    return pixmapItem;
}


