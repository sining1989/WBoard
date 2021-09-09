#include "WBItem.h"

#include "core/memcheck.h"

#include "domain/WBGraphicsPixmapItem.h"
#include "domain/WBGraphicsTextItem.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsMediaItem.h"
#include "domain/WBGraphicsStrokesGroup.h"
#include "domain/WBGraphicsGroupContainerItem.h"
#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsScene.h"
#include "tools/WBGraphicsCurtainItem.h"
#include "domain/WBGraphicsItemDelegate.h"

WBItem::WBItem()
    : mUuid(QUuid())
    , mRenderingQuality(WBItem::RenderingQualityNormal)
{
    // NOOP
}

WBItem::~WBItem()
{
    // NOOP
}

WBGraphicsItem::~WBGraphicsItem()
{
    if (mDelegate!=NULL)
        delete mDelegate;
}

void WBGraphicsItem::setDelegate(WBGraphicsItemDelegate* delegate)
{
    Q_ASSERT(mDelegate==NULL);
    mDelegate = delegate;
}

WBGraphicsItemDelegate *WBGraphicsItem::Delegate() const
{
    return mDelegate;
}

void WBGraphicsItem::assignZValue(QGraphicsItem *item, qreal value)
{
    item->setZValue(value);
    item->setData(WBGraphicsItemData::ItemOwnZValue, value);
}

bool WBGraphicsItem::isFlippable(QGraphicsItem *item)
{
    return item->data(WBGraphicsItemData::ItemFlippable).toBool();
}

bool WBGraphicsItem::isRotatable(QGraphicsItem *item)
{
    return item->data(WBGraphicsItemData::ItemRotatable).toBool();
}

bool WBGraphicsItem::isLocked(QGraphicsItem *item)
{
    return item->data(WBGraphicsItemData::ItemLocked).toBool();
}

QUuid WBGraphicsItem::getOwnUuid(QGraphicsItem *item)
{
    QString idCandidate = item->data(WBGraphicsItemData::ItemUuid).toString();
    return idCandidate == QUuid().toString() ? QUuid() : QUuid(idCandidate);
}

void WBGraphicsItem::remove(bool canUndo)
{
    if (Delegate())
        Delegate()->remove(canUndo);
}

WBGraphicsItemDelegate *WBGraphicsItem::Delegate(QGraphicsItem *pItem)
{
    WBGraphicsItemDelegate *result = 0;

    switch (static_cast<int>(pItem->type())) {
    case WBGraphicsPixmapItem::Type :
        result = (static_cast<WBGraphicsPixmapItem*>(pItem))->Delegate();
        break;
    case WBGraphicsTextItem::Type :
        result = (static_cast<WBGraphicsTextItem*>(pItem))->Delegate();
        break;
    case WBGraphicsSvgItem::Type :
        result = (static_cast<WBGraphicsSvgItem*>(pItem))->Delegate();
        break;
    case WBGraphicsMediaItem::Type:
    case WBGraphicsVideoItem::Type:
    case WBGraphicsAudioItem::Type:
        result = (static_cast<WBGraphicsMediaItem*>(pItem))->Delegate();
        break;
    case WBGraphicsStrokesGroup::Type :
        result = (static_cast<WBGraphicsStrokesGroup*>(pItem))->Delegate();
        break;
    case WBGraphicsGroupContainerItem::Type :
        result = (static_cast<WBGraphicsGroupContainerItem*>(pItem))->Delegate();
        break;
    case WBGraphicsWidgetItem::Type :
        result = (static_cast<WBGraphicsWidgetItem*>(pItem))->Delegate();
        break;
    case WBGraphicsCurtainItem::Type :
        result = (static_cast<WBGraphicsCurtainItem*>(pItem))->Delegate();
        break;
    }

    return result;
}
