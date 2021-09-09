#include "WBGraphicsStrokesGroup.h"
#include "WBGraphicsStroke.h"

#include "domain/WBGraphicsPolygonItem.h"

#include "core/memcheck.h"

WBGraphicsStrokesGroup::WBGraphicsStrokesGroup(QGraphicsItem *parent)
    :QGraphicsItemGroup(parent), WBGraphicsItem()
{
    setDelegate(new WBGraphicsItemDelegate(this, 0, GF_COMMON
                                           | GF_RESPECT_RATIO
                                           | GF_REVOLVABLE
                                           | GF_FLIPPABLE_ALL_AXIS));

    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);

    setUuid(QUuid::createUuid());
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    mDebugText = NULL;
    debugTextEnabled = false; // set to true to get a graphical display of strokes' Z-levels
}

WBGraphicsStrokesGroup::~WBGraphicsStrokesGroup()
{
}

void WBGraphicsStrokesGroup::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void WBGraphicsStrokesGroup::setColor(const QColor &color, colorType pColorType)
{
    //TODO Implement common mechanism of managing groups, drop WBGraphicsStroke if it's obsolete
    //Using casting for the moment
    foreach (QGraphicsItem *item, childItems()) {
        if (item->type() == WBGraphicsPolygonItem::Type) {
            WBGraphicsPolygonItem *curPolygon = static_cast<WBGraphicsPolygonItem *>(item);

            switch (pColorType) {
            case currentColor :
                curPolygon->setColor(color);
                break;
            case colorOnLightBackground :
                 curPolygon->setColorOnLightBackground(color);
                break;
            case colorOnDarkBackground :
                 curPolygon->setColorOnDarkBackground(color);
                break;
            }
        }
    }

    if (mDebugText)
        mDebugText->setBrush(QBrush(color));
}

QColor WBGraphicsStrokesGroup::color(colorType pColorType) const
{
    QColor result;

    foreach (QGraphicsItem *item, childItems()) {
        if (item->type() == WBGraphicsPolygonItem::Type) {
            WBGraphicsPolygonItem *curPolygon = static_cast<WBGraphicsPolygonItem *>(item);

            switch (pColorType) {
            case currentColor :
                result = curPolygon->color();
                break;
            case colorOnLightBackground :
                result = curPolygon->colorOnLightBackground();
                break;
            case colorOnDarkBackground :
                result = curPolygon->colorOnDarkBackground();
                break;
            }

        }
    }

    return result;
}

void WBGraphicsStrokesGroup::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->startUndoStep();

    QGraphicsItemGroup::mousePressEvent(event);
    event->accept();

    setSelected(false);
}

void WBGraphicsStrokesGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!isLocked(this)) {
        QGraphicsItemGroup::mouseMoveEvent(event);

        event->accept();
        setSelected(false);
    }
}

void WBGraphicsStrokesGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->commitUndoStep();
    event->accept();

    Delegate()->mouseReleaseEvent(event);
    QGraphicsItemGroup::mouseReleaseEvent(event);
}

WBItem* WBGraphicsStrokesGroup::deepCopy() const
{
    QTransform groupTransform = transform();
    QPointF groupPos = pos();

    WBGraphicsStrokesGroup* copy = new WBGraphicsStrokesGroup();
    copyItemParameters(copy);
    copy->resetTransform();
    copy->setPos(0,0);

    const_cast<WBGraphicsStrokesGroup*>(this)->resetTransform();
    const_cast<WBGraphicsStrokesGroup*>(this)->setPos(0,0);

    QList<QGraphicsItem*> chl = childItems();

    WBGraphicsStroke* newStroke = new WBGraphicsStroke;

    foreach(QGraphicsItem *child, chl)
    {
        WBGraphicsPolygonItem *polygon = dynamic_cast<WBGraphicsPolygonItem*>(child);

        if (polygon){
            WBGraphicsPolygonItem *polygonCopy = dynamic_cast<WBGraphicsPolygonItem*>(polygon->deepCopy());
            if (polygonCopy)
            {
                QGraphicsItem* pItem = dynamic_cast<QGraphicsItem*>(polygonCopy);
                copy->addToGroup(pItem);
                polygonCopy->setStrokesGroup(copy);
                polygonCopy->setStroke(newStroke);
            }
        }
    }
    const_cast<WBGraphicsStrokesGroup*>(this)->setTransform(groupTransform);
    const_cast<WBGraphicsStrokesGroup*>(this)->setPos(groupPos);
    copy->setTransform(groupTransform);
    copy->setPos(groupPos);

    return copy;
}

void WBGraphicsStrokesGroup::copyItemParameters(WBItem *copy) const
{
    QGraphicsItem *cp = dynamic_cast<QGraphicsItem*>(copy);
    if(NULL != cp)
    {
        cp->setTransform(transform());
        cp->setPos(pos());

        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));
        cp->setZValue(this->zValue());
    }
}

void WBGraphicsStrokesGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    QStyle::State svState = option->state;
    styleOption.state &= ~QStyle::State_Selected;
    QGraphicsItemGroup::paint(painter, &styleOption, widget);
    //Restoring state
    styleOption.state |= svState;

    Delegate()->postpaint(painter, &styleOption, widget);
}

QVariant WBGraphicsStrokesGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (debugTextEnabled && change == ItemZValueChange) {
        double newZ = qvariant_cast<double>(value);

        WBGraphicsPolygonItem * poly = NULL;
        if (childItems().size() > 2)
            poly = dynamic_cast<WBGraphicsPolygonItem*>(childItems()[1]);

        if (poly) {
            if (!mDebugText) {
                mDebugText = new QGraphicsSimpleTextItem("None", this);
                mDebugText->setPos(poly->boundingRect().topLeft() + QPointF(10, 10));
                mDebugText->setBrush(QBrush(poly->color()));
            }
            mDebugText->setText(QString("Z: %1").arg(newZ));
        }
    }

    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsItemGroup::itemChange(change, newValue);
}

QPainterPath WBGraphicsStrokesGroup::shape() const
{
    QPainterPath path;

    if (isSelected())
    {
        path.addRect(boundingRect());
    }
    else
    {
        foreach(QGraphicsItem* item, childItems())
        {
            path.addPath(item->shape());
        }
    }

    return path;
}
