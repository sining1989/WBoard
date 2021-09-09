#ifndef WBGRAPHICSSTROKE_H_
#define WBGRAPHICSSTROKE_H_

#include <QtWidgets>

#include "core/WB.h"

class WBGraphicsPolygonItem;
class WBGraphicsScene;

class WBGraphicsStroke
{
    friend class WBGraphicsPolygonItem;

    public:
        WBGraphicsStroke(WBGraphicsScene* scene = NULL);
        virtual ~WBGraphicsStroke();

        bool hasPressure();

        QList<WBGraphicsPolygonItem*> polygons() const;

        void remove(WBGraphicsPolygonItem* polygonItem); 

        WBGraphicsStroke *deepCopy();

        bool hasAlpha() const;

        void clear();

        QList<QPair<QPointF, qreal> > addPoint(const QPointF& point, qreal width, bool interpolate = false);

        const QList<QPair<QPointF, qreal> >& points() { return mDrawnPoints; }

        WBGraphicsStroke* simplify();

    protected:
        void addPolygon(WBGraphicsPolygonItem* pol);

    private:
        WBGraphicsScene * mScene;

        QList<WBGraphicsPolygonItem*> mPolygons;

        QList<QPair<QPointF, qreal> > mReceivedPoints;

        QList<QPair<QPointF, qreal> > mDrawnPoints;

        qreal mAntiScaleRatio;
};

#endif /* WBGRAPHICSSTROKE_H_ */
