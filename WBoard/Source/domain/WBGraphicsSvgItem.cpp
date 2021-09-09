#include "WBGraphicsSvgItem.h"

#include <QtWidgets>

#include "WBGraphicsScene.h"
#include "WBGraphicsItemDelegate.h"
#include "WBGraphicsPixmapItem.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"

#include "board/WBBoardController.h"

#include "frameworks/WBFileSystemUtils.h"

#include "core/memcheck.h"

WBGraphicsSvgItem::WBGraphicsSvgItem(const QString& pFilePath, QGraphicsItem* parent)
    : QGraphicsSvgItem(pFilePath, parent)
{
    init();

    QFile f(pFilePath);

    if (f.open(QIODevice::ReadOnly))
    {
        mFileData = f.readAll();
        f.close();
    }
}

WBGraphicsSvgItem::WBGraphicsSvgItem(const QByteArray& pFileData, QGraphicsItem* parent)
    : QGraphicsSvgItem(parent)
{
    init();

    QSvgRenderer* renderer = new QSvgRenderer(pFileData, this);

    setSharedRenderer(renderer);
    mFileData = pFileData;
}


void WBGraphicsSvgItem::init()
{
    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);

    setDelegate(new WBGraphicsItemDelegate(this, 0, GF_COMMON
                                           | GF_RESPECT_RATIO
                                           | GF_REVOLVABLE));
    WBGraphicsFlags dfl = Delegate()->ubflags();
    Delegate()->setUBFlags(dfl | GF_FLIPPABLE_ALL_AXIS | GF_REVOLVABLE);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setMaximumCacheSize(boundingRect().size().toSize() * WB_MAX_ZOOM);

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly

    setData(WBGraphicsItemData::ItemCanBeSetAsBackground, true);

    setUuid(QUuid::createUuid());
}

WBGraphicsSvgItem::~WBGraphicsSvgItem()
{
}


QByteArray WBGraphicsSvgItem::fileData() const
{
    return mFileData;
}


QVariant WBGraphicsSvgItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsSvgItem::itemChange(change, newValue);
}


void WBGraphicsSvgItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData* pMime = new QMimeData();
    QPixmap pixmap = toPixmapItem()->pixmap();
    pMime->setImageData(pixmap.toImage());
    Delegate()->setMimeData(pMime);
    qreal k = (qreal)pixmap.width() / 100.0;

    QSize newSize((int)(pixmap.width() / k), (int)(pixmap.height() / k));

    Delegate()->setDragPixmap(pixmap.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    if (!Delegate()->mousePressEvent(event))
        QGraphicsSvgItem::mousePressEvent(event);
}


void WBGraphicsSvgItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP;
    }
    else
    {
        QGraphicsSvgItem::mouseMoveEvent(event);
    }
}


void WBGraphicsSvgItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsSvgItem::mouseReleaseEvent(event);
}


void WBGraphicsSvgItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    QGraphicsSvgItem::paint(painter, &styleOption, widget);
    Delegate()->postpaint(painter, option, widget);
}


WBItem* WBGraphicsSvgItem::deepCopy() const
{
    WBGraphicsSvgItem* copy = new WBGraphicsSvgItem(this->fileData());

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;

}

void WBGraphicsSvgItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsSvgItem *cp = dynamic_cast<WBGraphicsSvgItem*>(copy);
    if (cp)
    {
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

void WBGraphicsSvgItem::setRenderingQuality(RenderingQuality pRenderingQuality)
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


WBGraphicsScene* WBGraphicsSvgItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}



WBGraphicsPixmapItem* WBGraphicsSvgItem::toPixmapItem() const
{
    QImage image(renderer()->viewBox().size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    renderer()->render(&painter);

    WBGraphicsPixmapItem *pixmapItem =  new WBGraphicsPixmapItem();
    pixmapItem->setPixmap(QPixmap::fromImage(image));

    pixmapItem->setPos(this->pos());
    pixmapItem->setTransform(this->transform());
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));

    return pixmapItem;
}

void WBGraphicsSvgItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}


void WBGraphicsSvgItem::clearSource()
{
    QString fileName = WBPersistenceManager::imageDirectory + "/" + uuid().toString() + ".svg";
    QString diskPath =  WBApplication::boardController->selectedDocument()->persistencePath() + "/" + fileName;
    WBFileSystemUtils::deleteFile(diskPath);
}
