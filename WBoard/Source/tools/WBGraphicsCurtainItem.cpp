#include "WBGraphicsCurtainItem.h"

#include <QtWidgets>

#include "domain/WBGraphicsScene.h"

#include "WBGraphicsCurtainItemDelegate.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"
#include "core/WBSettings.h"
#include "board/WBBoardController.h"
#include "board/WBBoardView.h"

#include "core/memcheck.h"

const QColor WBGraphicsCurtainItem::sDrawColor = Qt::white;
const QColor WBGraphicsCurtainItem::sDarkBackgroundDrawColor = Qt::black;
const QColor WBGraphicsCurtainItem::sOpaqueControlColor = QColor(191,191,191,255);
const QColor WBGraphicsCurtainItem::sDarkBackgroundOpaqueControlColor = QColor(63,63,63,255);

WBGraphicsCurtainItem::WBGraphicsCurtainItem(QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
{
    WBGraphicsCurtainItemDelegate* delegate = new WBGraphicsCurtainItemDelegate(this, 0);
    delegate->init();
    setDelegate(delegate);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

#if QT_VERSION >= 0x040600 // needs Qt 4.6.0 or better
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif

    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Tool);
    setPen(Qt::NoPen);
    this->setAcceptHoverEvents(true);

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Curtain)); //Necessary to set if we want z value to be assigned correctly
}

WBGraphicsCurtainItem::~WBGraphicsCurtainItem()
{
}

QVariant WBGraphicsCurtainItem::itemChange(GraphicsItemChange change, const QVariant &value)
{

    QVariant newValue = value;

    if (Delegate())
    {
        newValue = Delegate()->itemChange(change, value);
    }

    return QGraphicsRectItem::itemChange(change, newValue);
}

void WBGraphicsCurtainItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void WBGraphicsCurtainItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        //NOOP
    }
    else
    {
        QGraphicsRectItem::mousePressEvent(event);
    }
}

void WBGraphicsCurtainItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP;
    }
    else
    {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}

void WBGraphicsCurtainItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsRectItem::mouseReleaseEvent(event);
}


void WBGraphicsCurtainItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QColor color = drawColor();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    if(widget == WBApplication::boardController->controlView()->viewport())
    {
        color = WBSettings::paletteColor;
        if(!WBApplication::applicationController->displayManager()->hasDisplay())
        {
            color = opaqueControlColor();
        }
    }

    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    painter->fillRect(rect(), color);
    Delegate()->postpaint(painter, option, widget);
}


WBItem* WBGraphicsCurtainItem::deepCopy() const
{
   WBGraphicsCurtainItem* copy = new WBGraphicsCurtainItem();

    copyItemParameters(copy);

   return copy;
}

void WBGraphicsCurtainItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsCurtainItem *cp = dynamic_cast<WBGraphicsCurtainItem*>(copy);
    if (cp)
    {
        cp->setRect(this->rect());
        cp->setPos(this->pos());
        cp->setBrush(this->brush());
        cp->setPen(this->pen());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setZValue(this->zValue());
    }
}

QColor WBGraphicsCurtainItem::drawColor() const
{
    WBGraphicsScene* pScene = static_cast<WBGraphicsScene*>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundDrawColor : sDrawColor;
}


QColor WBGraphicsCurtainItem::opaqueControlColor() const
{
    WBGraphicsScene* pScene = static_cast<WBGraphicsScene*>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundOpaqueControlColor : sOpaqueControlColor;
}


void WBGraphicsCurtainItem::triggerRemovedSignal()
{
    emit removed();
}
