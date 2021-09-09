#ifndef WBSVGSUBSETADAPTOR_H_
#define WBSVGSUBSETADAPTOR_H_

#include <QtWidgets>
#include <QtXml>
#include <QGraphicsItem>

#include "frameworks/WBGeometryUtils.h"

class WBGraphicsSvgItem;
class WBGraphicsPolygonItem;
class WBGraphicsPixmapItem;
class WBGraphicsPDFItem;
class WBGraphicsWidgetItem;
class WBGraphicsMediaItem;
class WBGraphicsVideoItem;
class WBGraphicsAudioItem;
class WBGraphicsAppleWidgetItem;
class WBGraphicsW3CWidgetItem;
class WBGraphicsTextItem;
class WBGraphicsCurtainItem;
class WBGraphicsRuler;
class WBGraphicsCompass;
class WBGraphicsProtractor;
class WBGraphicsScene;
class WBDocumentProxy;
class WBGraphicsStroke;
class WBPersistenceManager;
class WBGraphicsTriangle;
class WBGraphicsCache;
class WBGraphicsGroupContainerItem;
class WBGraphicsStrokesGroup;

class WBSvgSubsetAdaptor
{
    private:
        WBSvgSubsetAdaptor() {;}
        virtual ~WBSvgSubsetAdaptor() {;}

    public:
        static WBGraphicsScene* loadScene(WBDocumentProxy* proxy, const int pageIndex);
        static QByteArray loadSceneAsText(WBDocumentProxy* proxy, const int pageIndex);
        static WBGraphicsScene* loadScene(WBDocumentProxy* proxy, const QByteArray& pArray);

        static void persistScene(WBDocumentProxy* proxy, WBGraphicsScene* pScene, const int pageIndex);
        static void upgradeScene(WBDocumentProxy* proxy, const int pageIndex);

        static QUuid sceneUuid(WBDocumentProxy* proxy, const int pageIndex);
        static void setSceneUuid(WBDocumentProxy* proxy, const int pageIndex, QUuid pUuid);

        static void convertPDFObjectsToImages(WBDocumentProxy* proxy);
        static void convertSvgImagesToImages(WBDocumentProxy* proxy);

        static const QString nsSvg;
        static const QString nsXLink;
        static const QString nsXHtml;
        static const QString nsUb;
        static const QString xmlTrue;
        static const QString xmlFalse;

        static const QString sFontSizePrefix;
        static const QString sPixelUnit;
        static const QString sFontWeightPrefix;
        static const QString sFontStylePrefix;

    private:
        static QDomDocument loadSceneDocument(WBDocumentProxy* proxy, const int pPageIndex);

        static QString uniboardDocumentNamespaceUriFromVersion(int fileVersion);

        static const QString sFormerUniboardDocumentNamespaceUri;

        static QString toSvgTransform(const QMatrix& matrix);
        static QMatrix fromSvgTransform(const QString& transform);

    class WBSvgSubsetReader
    {
        public:
            WBSvgSubsetReader(WBDocumentProxy* proxy, const QByteArray& pXmlData);

            virtual ~WBSvgSubsetReader(){}

            WBGraphicsScene* loadScene(WBDocumentProxy *proxy);

        private:
            WBGraphicsPolygonItem* polygonItemFromLineSvg(const QColor& pDefaultBrushColor);

            WBGraphicsPolygonItem* polygonItemFromPolygonSvg(const QColor& pDefaultBrushColor);

            QList<WBGraphicsPolygonItem*> polygonItemsFromPolylineSvg(const QColor& pDefaultColor);

            WBGraphicsPixmapItem* pixmapItemFromSvg();

            WBGraphicsSvgItem* svgItemFromSvg();

            WBGraphicsPDFItem* pdfItemFromPDF();

            WBGraphicsMediaItem* videoItemFromSvg();

            WBGraphicsMediaItem* audioItemFromSvg();

            WBGraphicsAppleWidgetItem* graphicsAppleWidgetFromSvg();

            WBGraphicsW3CWidgetItem* graphicsW3CWidgetFromSvg();

            WBGraphicsTextItem* textItemFromSvg();

            WBGraphicsCurtainItem* curtainItemFromSvg();

            WBGraphicsRuler* rulerFromSvg();

            WBGraphicsCompass* compassFromSvg();

            WBGraphicsProtractor* protractorFromSvg();

            WBGraphicsTriangle* triangleFromSvg();

            WBGraphicsCache* cacheFromSvg();

            void readGroupRoot();
            QGraphicsItem *readElementFromGroup();
            WBGraphicsGroupContainerItem* readGroup();

            void graphicsItemFromSvg(QGraphicsItem* gItem);

            QXmlStreamReader mXmlReader;
            int mFileVersion;
            WBDocumentProxy *mProxy;
            QString mDocumentPath;

            QColor mGroupDarkBackgroundColor;
            QColor mGroupLightBackgroundColor;
            qreal mGroupZIndex;
            bool mGroupHasInfo;

            bool saveSceneAfterLoading;

            QString mNamespaceUri;
            WBGraphicsScene *mScene;

            QHash<QString,WBGraphicsStrokesGroup*> mStrokesList;
    };

    class WBSvgSubsetWriter
    {
        public:
            WBSvgSubsetWriter(WBDocumentProxy* proxy, WBGraphicsScene* pScene, const int pageIndex);

            bool persistScene(WBDocumentProxy *proxy, int pageIndex);

            virtual ~WBSvgSubsetWriter(){}

        private:
            void persistGroupToDom(QGraphicsItem *groupItem, QDomElement *curParent, QDomDocument *curDomDocument);
            void persistStrokeToDom(QGraphicsItem *strokeItem, QDomElement *curParent, QDomDocument *curDomDocument);
            void polygonItemToSvgPolygon(WBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo);
            void polygonItemToSvgLine(WBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo);
            void strokeToSvgPolyline(WBGraphicsStroke* stroke, bool groupHoldsInfo);
            void strokeToSvgPolygon(WBGraphicsStroke* stroke, bool groupHoldsInfo);

            inline QString pointsToSvgPointsAttribute(QVector<QPointF> points)
            {
                WBGeometryUtils::crashPointList(points);

                int pointsCount = points.size();
                QString svgPoints;

                int length = 0;
                QString sBuf;

                for(int j = 0; j < pointsCount; j++)
                {
                    sBuf = "%1,%2 ";
                    const QPointF & point = points.at(j);

                    QString temp1 =  "%1", temp2 = "%2";

                    temp1 = temp1.arg(point.x());
                    temp2 = temp2.arg(point.y());

                    QLocale loc(QLocale::C);
                    sBuf = sBuf.arg(loc.toFloat(temp1)).arg(loc.toFloat(temp2));

                    svgPoints.insert(length, sBuf);
                    length += sBuf.length();
                }
                return svgPoints;
            }

            inline qreal trickAlpha(qreal alpha)
            {
                    qreal trickAlpha = alpha;
                    if(trickAlpha >= 0.2 && trickAlpha < 0.6){
                            trickAlpha /= 5;
                    }else if (trickAlpha < 0.8)
                        trickAlpha /= 3;

                    return trickAlpha;
            }

            void pixmapItemToLinkedImage(WBGraphicsPixmapItem *pixmapItem);
            void svgItemToLinkedSvg(WBGraphicsSvgItem *svgItem);
            void pdfItemToLinkedPDF(WBGraphicsPDFItem *pdfItem);
            void videoItemToLinkedVideo(WBGraphicsVideoItem *videoItem);
            void audioItemToLinkedAudio(WBGraphicsAudioItem *audioItem);
            void graphicsItemToSvg(QGraphicsItem *item);
            void graphicsAppleWidgetToSvg(WBGraphicsAppleWidgetItem *item);
            void graphicsW3CWidgetToSvg(WBGraphicsW3CWidgetItem *item);
            void graphicsWidgetToSvg(WBGraphicsWidgetItem *item);
            void textItemToSvg(WBGraphicsTextItem *item);
            void curtainItemToSvg(WBGraphicsCurtainItem *item);
            void rulerToSvg(WBGraphicsRuler *item);
            void compassToSvg(WBGraphicsCompass *item);
            void protractorToSvg(WBGraphicsProtractor *item);
            void cacheToSvg(WBGraphicsCache* item);
            void triangleToSvg(WBGraphicsTriangle *item);
            void writeSvgElement(WBDocumentProxy *proxy);

    private:
        WBGraphicsScene* mScene;
        QXmlStreamWriter mXmlWriter;
        QString mDocumentPath;
        int mPageIndex;

    };
};

#endif /* WBSVGSUBSETADAPTOR_H_ */
