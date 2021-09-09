#ifndef WBGRAPHICSSVGITEM_H_
#define WBGRAPHICSSVGITEM_H_

#include <QtWidgets>
#include <QtSvg>

#include "WBItem.h"

#include "core/WB.h"

class WBGraphicsItemDelegate;
class WBGraphicsPixmapItem;

class WBGraphicsSvgItem: public QGraphicsSvgItem, public WBItem, public WBGraphicsItem
{
    public:
        WBGraphicsSvgItem(const QString& pFile, QGraphicsItem* parent = 0);
        WBGraphicsSvgItem(const QByteArray& pFileData, QGraphicsItem* parent = 0);

        void init();

        virtual ~WBGraphicsSvgItem();

        QByteArray fileData() const;

        void setFileData(const QByteArray& pFileData)
        {
            mFileData = pFileData;
        }

        enum { Type = WBGraphicsItemType::SvgItemType };

        virtual int type() const
        {
            return Type;
        }

        virtual WBItem* deepCopy() const;

        virtual void copyItemParameters(WBItem *copy) const;

        virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

        virtual WBGraphicsScene* scene();

        virtual WBGraphicsPixmapItem* toPixmapItem() const;

        virtual void setUuid(const QUuid &pUuid);

        virtual void clearSource();

    protected:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        QByteArray mFileData;
};

#endif /* WBGRAPHICSSVGITEM_H_ */
