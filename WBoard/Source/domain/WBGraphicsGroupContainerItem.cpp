#include "WBGraphicsGroupContainerItem.h"

#include <QtWidgets>

#include "WBGraphicsMediaItem.h"
#include "WBGraphicsTextItem.h"
#include "domain/WBGraphicsItemDelegate.h"
#include "domain/WBGraphicsGroupContainerItemDelegate.h"
#include "domain/WBGraphicsScene.h"

#include "core/memcheck.h"

WBGraphicsGroupContainerItem::WBGraphicsGroupContainerItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mCurrentItem(NULL)
{
    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);

    setDelegate(new WBGraphicsGroupContainerItemDelegate(this, 0));

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    WBGraphicsGroupContainerItem::setAcceptHoverEvents(true);

    setUuid(QUuid::createUuid());

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
}

WBGraphicsGroupContainerItem::~WBGraphicsGroupContainerItem()
{
}

void WBGraphicsGroupContainerItem::addToGroup(QGraphicsItem *item)
{
    if (!item) {
        qWarning("WBGraphicsGroupContainerItem::addToGroup: cannot add null item");
        return;
    }
    if (item == this) {
        qWarning("WBGraphicsGroupContainerItem::addToGroup: cannot add a group to itself");
        return;
    }

    //Check if group is allready rotatable or flippable
    if (childItems().count()) {
        if (WBGraphicsItem::isFlippable(this) && !WBGraphicsItem::isFlippable(item)) {
            Delegate()->setUBFlag(GF_FLIPPABLE_ALL_AXIS, false);
        }
        if (WBGraphicsItem::isRotatable(this) && !WBGraphicsItem::isRotatable(item)) {
            Delegate()->setUBFlag(GF_REVOLVABLE, false);
        }
        if (!WBGraphicsItem::isLocked(this) && WBGraphicsItem::isLocked(item)) {
            Delegate()->setLocked(true);
        }
    }
    else {
        Delegate()->setUBFlag(GF_FLIPPABLE_ALL_AXIS, WBGraphicsItem::isFlippable(item));
        Delegate()->setUBFlag(GF_REVOLVABLE, WBGraphicsItem::isRotatable(item));
        Delegate()->setLocked(WBGraphicsItem::isLocked(item));
    }        

    if (item->data(WBGraphicsItemData::ItemLayerType) == WBItemLayerType::Control)
        setData(WBGraphicsItemData::ItemLayerType, item->data(WBGraphicsItemData::ItemLayerType));

    // COMBINE
    bool ok;
    QTransform itemTransform = item->itemTransform(this, &ok);

    if (!ok) {
        qWarning("WBGraphicsGroupContainerItem::addToGroup: could not find a valid transformation from item to group coordinates");
        return;
    }

    //setting item flags to given item
    item->setSelected(false);
    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
    item->setFlag( QGraphicsItem::ItemIsMovable, false);
    item->setFlag(QGraphicsItem::ItemIsFocusable, true);

    QTransform newItemTransform(itemTransform);
    item->setPos(mapFromItem(item, 0, 0));

    if (item->scene()) {
        item->scene()->removeItem(item);
    }

    if (corescene())
        corescene()->removeItemFromDeletion(item);
    item->setParentItem(this);

    // removing position from translation component of the new transform
    if (!item->pos().isNull())
        newItemTransform *= QTransform::fromTranslate(-item->x(), -item->y());

    // removing additional transformations properties applied with itemTransform()
    QPointF origin = item->transformOriginPoint();
    QMatrix4x4 m;
    QList<QGraphicsTransform*> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
        transformList.at(i)->applyTo(&m);
    newItemTransform *= m.toTransform().inverted();
    newItemTransform.translate(origin.x(), origin.y());
    newItemTransform.rotate(-item->rotation());
    newItemTransform.scale(1/item->scale(), 1/item->scale());
    newItemTransform.translate(-origin.x(), -origin.y());

    // ### Expensive, we could maybe use dirtySceneTransform bit for optimization

    item->setTransform(newItemTransform);
    //    item->d_func()->setIsMemberOfGroup(true);
    prepareGeometryChange();
    itemsBoundingRect |= itemTransform.mapRect(item->boundingRect() | item->childrenBoundingRect());
    update();
}
void WBGraphicsGroupContainerItem::removeFromGroup(QGraphicsItem *item)
{
    if (!item) {
        qDebug() << "can't specify the item because of the null pointer";
        return;
    }

    WBCoreGraphicsScene *groupScene = corescene();
    if (groupScene)
    {
        groupScene->addItemToDeletion(item);
    }

    pRemoveFromGroup(item);

    item->setFlags(ItemIsSelectable | ItemIsFocusable);

}

void WBGraphicsGroupContainerItem::deselectCurrentItem()
{
    if (mCurrentItem && (mCurrentItem->type() == WBGraphicsMediaItem::Type
                         || mCurrentItem->type() == WBGraphicsVideoItem::Type
                         || mCurrentItem->type() == WBGraphicsAudioItem::Type))
    {
        dynamic_cast<WBGraphicsMediaItem*>(mCurrentItem)->Delegate()->getToolBarItem()->hide();

        mCurrentItem->setSelected(false);
        mCurrentItem = NULL;
    }
}

QRectF WBGraphicsGroupContainerItem::boundingRect() const
{
    return itemsBoundingRect;
}

void WBGraphicsGroupContainerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    Q_UNUSED(option);

    Delegate()->postpaint(painter, option, widget);
}

WBCoreGraphicsScene *WBGraphicsGroupContainerItem::corescene()
{
    WBCoreGraphicsScene *castScene = dynamic_cast<WBCoreGraphicsScene*>(QGraphicsItem::scene());

    return castScene;
}

WBGraphicsGroupContainerItem *WBGraphicsGroupContainerItem::deepCopyNoChildDuplication() const
{
    WBGraphicsGroupContainerItem *copy = new WBGraphicsGroupContainerItem();

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;
}


WBGraphicsGroupContainerItem *WBGraphicsGroupContainerItem::deepCopy() const
{
    WBGraphicsGroupContainerItem *copy = new WBGraphicsGroupContainerItem();

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    foreach (QGraphicsItem *it, childItems()) {
        WBItem *childAsUBItem = dynamic_cast<WBItem*>(it);
        if (childAsUBItem) {
            QGraphicsItem *cloneItem = dynamic_cast<QGraphicsItem*>(childAsUBItem->deepCopy());
            copy->addToGroup(cloneItem);
        }
    }
    copyItemParameters(copy);

    return copy;
}



void WBGraphicsGroupContainerItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsGroupContainerItem *cp = dynamic_cast<WBGraphicsGroupContainerItem*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));
    }
}

void WBGraphicsGroupContainerItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void WBGraphicsGroupContainerItem::destroy(bool canUndo) {

    foreach (QGraphicsItem *item, childItems()) {
        pRemoveFromGroup(item);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsFocusable, true);
    }

    remove(canUndo);
}

void WBGraphicsGroupContainerItem::clearSource()
{
    foreach(QGraphicsItem *child, childItems())
    {
        WBGraphicsItem *item = dynamic_cast<WBGraphicsItem *>(child);
        if (item)
        {
            item->clearSource();
        }
    }
}

void WBGraphicsGroupContainerItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event)) {
        //NOOP
    } else {

    QGraphicsItem::mousePressEvent(event);
        setSelected(true);
    }


}

void WBGraphicsGroupContainerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event)) {
        // NOOP;
    } else {
        QGraphicsItem::mouseMoveEvent(event);
    }

}

void WBGraphicsGroupContainerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//    mDelegate->mouseReleaseEvent(event);
    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant WBGraphicsGroupContainerItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);

    foreach(QGraphicsItem *child, childItems())
    {
        WBGraphicsItem *item = dynamic_cast<WBGraphicsItem*>(child);
        if (item)
        {
            item->Delegate()->positionHandles();
        }
    }

    if (QGraphicsItem::ItemSelectedChange == change)
    {
        deselectCurrentItem();
    }

    return QGraphicsItem::itemChange(change, newValue);
}

void WBGraphicsGroupContainerItem::pRemoveFromGroup(QGraphicsItem *item)
{
    if (!item) {
        qWarning("QGraphicsItemGroup::removeFromGroup: cannot remove null item");
        return;
    }

    QGraphicsItem *newParent = parentItem();

    if (childItems().count()) {
        if (!WBGraphicsItem::isFlippable(item) || !WBGraphicsItem::isRotatable(item)) {
            bool flippableNow = true;
            bool rotatableNow = true;
            bool lockedNow = false;

            foreach (QGraphicsItem *item, childItems()) {
                if (!WBGraphicsItem::isFlippable(item)) {
                    flippableNow = false;
                }
                if (!WBGraphicsItem::isRotatable(item)) {
                    rotatableNow = false;
                }
                if(WBGraphicsItem::isLocked(item))
                    lockedNow = true;

                if (!rotatableNow && !flippableNow && lockedNow) {
                    break;
                }

            }
            Delegate()->setUBFlag(GF_FLIPPABLE_ALL_AXIS, flippableNow);
            Delegate()->setUBFlag(GF_REVOLVABLE, rotatableNow);
            Delegate()->setLocked(lockedNow);
        }
    }

    // COMBINE
    bool ok;
    QTransform itemTransform;
    if (newParent)
        itemTransform = item->itemTransform(newParent, &ok);
    else
        itemTransform = item->sceneTransform();

    QPointF oldPos = item->mapToItem(newParent, 0, 0);
    item->setParentItem(newParent);
    item->setPos(oldPos);

    WBGraphicsScene *Scene = dynamic_cast<WBGraphicsScene *>(item->scene());
    if (Scene)
    {
        Scene->addItem(item);
    }

    // removing position from translation component of the new transform
    if (!item->pos().isNull())
        itemTransform *= QTransform::fromTranslate(-item->x(), -item->y());

    // removing additional transformations properties applied
    // with itemTransform() or sceneTransform()
    QPointF origin = item->transformOriginPoint();
    QMatrix4x4 m;
    QList<QGraphicsTransform*> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
        transformList.at(i)->applyTo(&m);
    itemTransform *= m.toTransform().inverted();
    itemTransform.translate(origin.x(), origin.y());
    itemTransform.rotate(-item->rotation());
    itemTransform.scale(1 / item->scale(), 1 / item->scale());
    itemTransform.translate(-origin.x(), -origin.y());

    // ### Expensive, we could maybe use dirtySceneTransform bit for optimization

    item->setTransform(itemTransform);
//    item->d_func()->setIsMemberOfGroup(item->group() != 0);

    // ### Quite expensive. But removeFromGroup() isn't called very often.
    prepareGeometryChange();
    itemsBoundingRect = childrenBoundingRect();

    item->setFlag(ItemIsMovable, true);
}
