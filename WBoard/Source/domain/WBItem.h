#ifndef WBITEM_H
#define WBITEM_H

#include <QtWidgets>
#include "core/WB.h"
#include "domain/WBGraphicsItemDelegate.h"

class WBGraphicsScene;
class WBGraphicsItem;

class WBItem
{
    protected:
        WBItem();

    public:
        virtual ~WBItem();

        enum RenderingQuality
        {
            RenderingQualityNormal = 0, RenderingQualityHigh
        };

        virtual QUuid uuid() const
        {
            return mUuid;
        }

        virtual void setUuid(const QUuid& pUuid)
        {
			mUuid = pUuid;
        }

        virtual RenderingQuality renderingQuality() const
        {
            return mRenderingQuality;
        }

        virtual void setRenderingQuality(RenderingQuality pRenderingQuality)
        {
            mRenderingQuality = pRenderingQuality;
        }

        virtual WBItem* deepCopy() const = 0;

        virtual void copyItemParameters(WBItem *copy) const = 0;

        virtual WBGraphicsScene* scene() 
        {
            return 0;
        }

        virtual QUrl sourceUrl() const
        {
            return mSourceUrl;
        }

        virtual void setSourceUrl(const QUrl& pSourceUrl)
        {
            mSourceUrl = pSourceUrl;
        }

    protected:
        QUuid mUuid;

        RenderingQuality mRenderingQuality;

        QUrl mSourceUrl;
};

class WBGraphicsItem
{
protected:
    WBGraphicsItem() : mDelegate(NULL)
    {

    }
    virtual ~WBGraphicsItem();
    void setDelegate(WBGraphicsItemDelegate* mDelegate);

public:
    virtual int type() const = 0;

    WBGraphicsItemDelegate *Delegate() const;

    static void assignZValue(QGraphicsItem*, qreal value);
    static bool isRotatable(QGraphicsItem *item);
    static bool isFlippable(QGraphicsItem *item);
    static bool isLocked(QGraphicsItem *item);
    static QUuid getOwnUuid(QGraphicsItem *item);

    static WBGraphicsItemDelegate *Delegate(QGraphicsItem *pItem);

    void remove(bool canUndo = true);

    virtual void clearSource(){}

private:
    WBGraphicsItemDelegate* mDelegate;
};

#endif // WBITEM_H
