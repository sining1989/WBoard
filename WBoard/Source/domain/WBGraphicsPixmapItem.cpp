#include "WBGraphicsPixmapItem.h"

#include <QtWidgets>
#include <QMimeData>
#include <QDrag>

#include "WBGraphicsScene.h"

#include "WBGraphicsItemDelegate.h"

#include "frameworks/WBFileSystemUtils.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"

#include "board/WBBoardController.h"

#include "core/memcheck.h"

WBGraphicsPixmapItem::WBGraphicsPixmapItem(QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
{
    setDelegate(new WBGraphicsItemDelegate(this, 0, GF_COMMON
                                           | GF_FLIPPABLE_ALL_AXIS
                                           | GF_REVOLVABLE
                                           | GF_RESPECT_RATIO
                                           | GF_TOOLBAR_USED));

    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);
    setTransformationMode(Qt::SmoothTransformation);

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setData(WBGraphicsItemData::ItemCanBeSetAsBackground, true);

    setUuid(QUuid::createUuid()); //more logical solution is in creating uuid for element in element's constructor
}

WBGraphicsPixmapItem::~WBGraphicsPixmapItem()
{
}

QVariant WBGraphicsPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsPixmapItem::itemChange(change, newValue);
}

void WBGraphicsPixmapItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid));
}

void WBGraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData* pMime = new QMimeData();
    pMime->setImageData(pixmap().toImage());
    Delegate()->setMimeData(pMime);
    qreal k = (qreal)pixmap().width() / 100.0;

    QSize newSize((int)(pixmap().width() / k), (int)(pixmap().height() / k));

    Delegate()->setDragPixmap(pixmap().scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    if (Delegate()->mousePressEvent(event))
    {
        //NOOP
    }
    else
    {
//        QGraphicsPixmapItem::mousePressEvent(event);
    }
}

void WBGraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP;
    }
    else
    {
        QGraphicsPixmapItem::mouseMoveEvent(event);
    }
}

void WBGraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}


void WBGraphicsPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);

    styleOption.state &= ~QStyle::State_Selected;
    QGraphicsPixmapItem::paint(painter, &styleOption, widget);
    Delegate()->postpaint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, true);
}


WBItem* WBGraphicsPixmapItem::deepCopy() const
{
   WBGraphicsPixmapItem* copy = new WBGraphicsPixmapItem();

   copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

   copyItemParameters(copy);

   return copy;
}

void WBGraphicsPixmapItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsPixmapItem *cp = dynamic_cast<WBGraphicsPixmapItem*>(copy);
    if (cp)
    {
        cp->setPixmap(this->pixmap());
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));
        cp->setSourceUrl(this->sourceUrl());

        cp->setZValue(this->zValue());
    }
}

WBGraphicsScene* WBGraphicsPixmapItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}


void WBGraphicsPixmapItem::setOpacity(qreal op)
{
    QGraphicsPixmapItem::setOpacity(op);
}


qreal WBGraphicsPixmapItem::opacity() const
{
    return QGraphicsPixmapItem::opacity();
}


void WBGraphicsPixmapItem::clearSource()
{
    QString fileName = WBPersistenceManager::imageDirectory + "/" + uuid().toString() + ".png";
    QString diskPath =  WBApplication::boardController->selectedDocument()->persistencePath() + "/" + fileName;
    WBFileSystemUtils::deleteFile(diskPath);
}
