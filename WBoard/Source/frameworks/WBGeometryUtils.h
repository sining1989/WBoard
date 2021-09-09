#ifndef WBGEOMETRYUTILS_H_
#define WBGEOMETRYUTILS_H_

#include <QtWidgets>

class WBGeometryUtils
{
    private:
        WBGeometryUtils();
        virtual ~WBGeometryUtils();

    public:
        static QPolygonF lineToPolygon(const QLineF& pLine, const qreal& pWidth);
        static QPolygonF lineToPolygon(const QLineF& pLine, const qreal& pStartWidth, const qreal& pEndWidth);
        static QRectF lineToInnerRect(const QLineF& pLine, const qreal& pWidth);

        static QPolygonF arcToPolygon(const QLineF& startRadius, qreal spanAngle, qreal width);

        static QPolygonF lineToPolygon(const QPointF& pStart, const QPointF& pEnd,
                const qreal& pStartWidth, const qreal& pEndWidth);
        static QPolygonF curveToPolygon(const QList<QPointF>& points, qreal startWidth, qreal endWidth);
        static QPolygonF curveToPolygon(const QList<QPair<QPointF, qreal> >& points, bool roundStart, bool roundEnd);

        static QPointF pointConstrainedInRect(QPointF point, QRectF rect);
        static QPoint pointConstrainedInRect(QPoint point, QRect rect);

        static void crashPointList(QVector<QPointF> &points);

        static qreal angle(const QPointF& p1, const QPointF& p2, const QPointF& p3);

        static QList<QPointF> quadraticBezier(const QPointF& p0, const QPointF& p1, const QPointF& p2, unsigned int nPoints);

        const static int centimeterGraduationHeight;
        const static int halfCentimeterGraduationHeight;
        const static int millimeterGraduationHeight;
        const static int millimetersPerCentimeter;
        const static int millimetersPerHalfCentimeter;
        const static float inchSize;
};

#endif /* WBGEOMETRYUTILS_H_ */
