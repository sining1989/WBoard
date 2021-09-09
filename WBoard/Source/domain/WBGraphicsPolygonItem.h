#ifndef WBGRAPHICSPOLYGONITEM_H
#define WBGRAPHICSPOLYGONITEM_H

#include <QtWidgets>

#include "core/WB.h"
#include "WBItem.h"
#include "WBGraphicsStrokesGroup.h"
#include "domain/WBGraphicsGroupContainerItem.h"

class WBItem;
class WBGraphicsScene;
class WBGraphicsStroke;

class WBGraphicsPolygonItem : public QGraphicsPolygonItem, public WBItem
{
    public:
        WBGraphicsPolygonItem(QGraphicsItem * parent = 0 );
        WBGraphicsPolygonItem(const QLineF& line, qreal pWidth);
        WBGraphicsPolygonItem(const QLineF& pLine, qreal pStartWidth, qreal pEndWidth);
        WBGraphicsPolygonItem(const QPolygonF & polygon, QGraphicsItem * parent = 0);

        ~WBGraphicsPolygonItem();

        void initialize();

        void setUuid(const QUuid &pUuid);

        void setStrokesGroup(WBGraphicsStrokesGroup* group);
        WBGraphicsStrokesGroup* strokesGroup() const{return mpGroup;}
        void setColor(const QColor& color);

        QColor color() const;

        virtual WBGraphicsScene* scene();

        inline void subtract(WBGraphicsPolygonItem *pi)
        {
            if (boundingRect().intersects(pi->boundingRect()))
            {
                QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

                if (polygon() != subtractedPolygon)
                {
                    mIsNominalLine = false;
                    QGraphicsPolygonItem::setPolygon(subtractedPolygon);
                }
            }
        }

        inline void subtractIntersecting(WBGraphicsPolygonItem *pi)
        {
            QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

            if (polygon() != subtractedPolygon)
            {
                mIsNominalLine = false;
                QGraphicsPolygonItem::setPolygon(subtractedPolygon);
            }
        }

        enum { Type = WBGraphicsItemType::PolygonItemType };

        virtual int type() const
        {
            return Type;
        }

        void setPolygon(const QPolygonF pPolygon)
        {
            mIsNominalLine = false;
            QGraphicsPolygonItem::setPolygon(pPolygon);
        }

        virtual WBItem* deepCopy() const;

        virtual void copyItemParameters(WBItem *copy) const;

        QLineF originalLine() { return mOriginalLine;}
        qreal originalWidth() { return mOriginalWidth;}
        bool isNominalLine() {return mIsNominalLine;}

        void setNominalLine(bool isNominalLine) { mIsNominalLine = isNominalLine; }

        QColor colorOnDarkBackground() const
        {
            return mColorOnDarkBackground;
        }

        void setColorOnDarkBackground(QColor pColorOnDarkBackground)
        {
            mColorOnDarkBackground = pColorOnDarkBackground;
        }

        QColor colorOnLightBackground() const
        {
            return mColorOnLightBackground;
        }

        void setColorOnLightBackground(QColor pColorOnLightBackground)
        {
            mColorOnLightBackground = pColorOnLightBackground;
        }

        void setStroke(WBGraphicsStroke* stroke);
        WBGraphicsStroke* stroke() const;

    protected:
        void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget);

    private:
        void clearStroke();

        bool mHasAlpha;

        QLineF mOriginalLine;
        qreal mOriginalWidth;
        bool mIsNominalLine;

        QColor mColorOnDarkBackground;
        QColor mColorOnLightBackground;

        WBGraphicsStroke* mStroke;
        WBGraphicsStrokesGroup* mpGroup;

};

#endif // WBGRAPHICSPOLYGONITEM_H
