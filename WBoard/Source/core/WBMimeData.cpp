#include "WBMimeData.h"

#include <QtWidgets>

#include "core/WBApplication.h"
#include "domain/WBItem.h"

#include "core/memcheck.h"

WBMimeDataItem::WBMimeDataItem(WBDocumentProxy* proxy, int sceneIndex)
    : mProxy(proxy)
    , mSceneIndex(sceneIndex)
{
    // NOOP
}

WBMimeDataItem::~WBMimeDataItem()
{
    // NOOP
}

WBMimeData::WBMimeData(const QList<WBMimeDataItem> &items)
    : QMimeData()
    , mItems(items)
{
    setData(WBApplication::mimeTypeUniboardPage, QByteArray());
}

WBMimeData::~WBMimeData()
{
    // NOOP
}

WBMimeDataGraphicsItem::WBMimeDataGraphicsItem(QList<WBItem*> pItems)
{
        mItems = pItems;
}

WBMimeDataGraphicsItem::~WBMimeDataGraphicsItem()
{
// Explanation: selected items are owned by the scene and handled by this class
}
