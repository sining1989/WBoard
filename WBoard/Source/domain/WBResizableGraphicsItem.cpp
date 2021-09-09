#include "WBResizableGraphicsItem.h"

#include "core/memcheck.h"

WBResizableGraphicsItem::WBResizableGraphicsItem()
{
    // NOOP

}

WBResizableGraphicsItem::~WBResizableGraphicsItem()
{
    // NOOP
}

void WBResizableGraphicsItem::resize(const QSizeF& pSize)
{
    resize(pSize.width(), pSize.height());
}
