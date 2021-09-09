#ifndef WBGRAPHICSSCENE_H_
#define WBGRAPHICSSCENE_H_

#include <QtWidgets>

#include "frameworks/WBCoreGraphicsScene.h"

#include "core/WB.h"

#include "WBItem.h"
#include "tools/WBGraphicsCurtainItem.h"

class WBGraphicsPixmapItem;
class WBGraphicsProxyWidget;
class WBGraphicsSvgItem;
class WBGraphicsPolygonItem;
class WBGraphicsMediaItem;
class WBGraphicsWidgetItem;
class WBGraphicsW3CWidgetItem;
class WBGraphicsAppleWidgetItem;
class WBToolWidget;
class WBGraphicsPDFItem;
class WBGraphicsTextItem;
class WBGraphicsRuler;
class WBGraphicsProtractor;
class WBGraphicsCompass;
class WBDocumentProxy;
class WBGraphicsCurtainItem;
class WBGraphicsStroke;
class WBMagnifierParams;
class WBMagnifier;
class WBGraphicsCache;
class WBGraphicsGroupContainerItem;
class WBSelectionFrame;
class WBBoardView;

const double PI = 4.0 * atan(1.0);

class WBZLayerController : public QObject
{
    Q_OBJECT

public:
    struct ItemLayerTypeData {
        ItemLayerTypeData() : bottomLimit(0), topLimit(0), curValue(0), incStep(1) {}
        ItemLayerTypeData(qreal bot, qreal top, qreal increment = 1) : bottomLimit(bot), topLimit(top), curValue(bot), incStep(increment) {}
        qreal bottomLimit;
        qreal topLimit;
        qreal curValue;
        qreal incStep;
    };

    enum moveDestination {
        up,
        down,
        top,
        bottom
    };

    typedef QMap<itemLayerType::Enum, ItemLayerTypeData> ScopeMap;

    WBZLayerController(QGraphicsScene *scene);

    qreal getBottomLimit(itemLayerType::Enum key) const {return scopeMap.value(key).bottomLimit;}
    qreal getTopLimit(itemLayerType::Enum key) const {return scopeMap.value(key).topLimit;}
    bool validLayerType(itemLayerType::Enum key) const {return scopeMap.contains(key);}

    static qreal errorNum() {return errorNumber;}

    qreal generateZLevel(itemLayerType::Enum key);
    qreal generateZLevel(QGraphicsItem *item);

    qreal changeZLevelTo(QGraphicsItem *item, moveDestination dest);
    itemLayerType::Enum typeForData(QGraphicsItem *item) const;
    void setLayerType(QGraphicsItem *pItem, itemLayerType::Enum pNewType);
    void shiftStoredZValue(QGraphicsItem *item, qreal zValue);

    bool zLevelAvailable(qreal z);

private:
    ScopeMap scopeMap;
    static qreal errorNumber;
    QGraphicsScene *mScene;
};

class WBGraphicsScene: public WBCoreGraphicsScene, public WBItem
{
    Q_OBJECT

    public:
		enum clearCase {
			clearItemsAndAnnotations = 0, 
			clearAnnotations, 
			clearItems,
			clearBackground
		};

        void enableUndoRedoStack(){mUndoRedoStackEnabled = true;}
        void setURStackEnable(bool enable){mUndoRedoStackEnabled = enable;}
        bool isURStackIsEnabled(){return mUndoRedoStackEnabled;}

        WBGraphicsScene(WBDocumentProxy *parent, bool enableUndoRedoStack = true);
        virtual ~WBGraphicsScene();

        virtual WBItem* deepCopy() const;

        virtual void copyItemParameters(WBItem *copy) const {Q_UNUSED(copy);}

        WBGraphicsScene* sceneDeepCopy() const;

        void clearContent(clearCase pCase = clearItemsAndAnnotations);

        bool inputDevicePress(const QPointF& scenePos, const qreal& pressure = 1.0);
        bool inputDeviceMove(const QPointF& scenePos, const qreal& pressure = 1.0);
        bool inputDeviceRelease();

        void leaveEvent (QEvent* event);

        void addItem(QGraphicsItem* item);
        void removeItem(QGraphicsItem* item);

        void addItems(const QSet<QGraphicsItem*>& item);
        void removeItems(const QSet<QGraphicsItem*>& item);

        WBGraphicsWidgetItem* addWidget(const QUrl& pWidgetUrl, const QPointF& pPos = QPointF(0, 0));
        WBGraphicsAppleWidgetItem* addAppleWidget(const QUrl& pWidgetUrl, const QPointF& pPos = QPointF(0, 0));
        WBGraphicsW3CWidgetItem* addW3CWidget(const QUrl& pWidgetUrl, const QPointF& pPos = QPointF(0, 0));
        void addGraphicsWidget(WBGraphicsWidgetItem* graphicsWidget, const QPointF& pPos = QPointF(0, 0));

        QPointF lastCenter();
        void setLastCenter(QPointF center);

        WBGraphicsMediaItem* addMedia(const QUrl& pMediaFileUrl, bool shouldPlayAsap, const QPointF& pPos = QPointF(0, 0));
        WBGraphicsMediaItem* addVideo(const QUrl& pVideoFileUrl, bool shouldPlayAsap, const QPointF& pPos = QPointF(0, 0));
        WBGraphicsMediaItem* addAudio(const QUrl& pAudioFileUrl, bool shouldPlayAsap, const QPointF& pPos = QPointF(0, 0));
        WBGraphicsSvgItem* addSvg(const QUrl& pSvgFileUrl, const QPointF& pPos = QPointF(0, 0), const QByteArray pData = QByteArray());
        WBGraphicsTextItem* addText(const QString& pString, const QPointF& pTopLeft = QPointF(0, 0));

        WBGraphicsTextItem*  addTextWithFont(const QString& pString, const QPointF& pTopLeft = QPointF(0, 0)
                , int pointSize = -1, const QString& fontFamily = "", bool bold = false, bool italic = false);
        WBGraphicsTextItem* addTextHtml(const QString &pString = QString(), const QPointF& pTopLeft = QPointF(0, 0));

        WBGraphicsW3CWidgetItem* addOEmbed(const QUrl& pContentUrl, const QPointF& pPos = QPointF(0, 0));

        WBGraphicsGroupContainerItem *createGroup(QList<QGraphicsItem*> items);
        void addGroup(WBGraphicsGroupContainerItem *groupItem);

        QGraphicsItem* setAsBackgroundObject(QGraphicsItem* item, bool pAdaptTransformation = false, bool expand = false);
        void unsetBackgroundObject();

        QGraphicsItem* backgroundObject() const
        {
            return mBackgroundObject;
        }

        bool isBackgroundObject(const QGraphicsItem* item) const
        {
            return item == mBackgroundObject;
        }

        QGraphicsItem* scaleToFitDocumentSize(QGraphicsItem* item, bool center = false, int margin = 0, bool expand = false);

        QRectF normalizedSceneRect(qreal ratio = -1.0);

        QGraphicsItem *itemForUuid(QUuid uuid);

        void moveTo(const QPointF& pPoint);
        void drawLineTo(const QPointF& pEndPoint, const qreal& pWidth, bool bLineStyle);
        void drawLineTo(const QPointF& pEndPoint, const qreal& pStartWidth, const qreal& endWidth, bool bLineStyle);
        void eraseLineTo(const QPointF& pEndPoint, const qreal& pWidth);
        void drawArcTo(const QPointF& pCenterPoint, qreal pSpanAngle);
        void drawCurve(const QList<QPair<QPointF, qreal> > &points);
        void drawCurve(const QList<QPointF>& points, qreal startWidth, qreal endWidth);

        bool isEmpty() const;

        void setDocument(WBDocumentProxy* pDocument);

        WBDocumentProxy* document() const
        {
            return mDocument;
        }

        bool isDarkBackground() const
        {
            return mDarkBackground;
        }

        bool isLightBackground() const
        {
            return !mDarkBackground;
        }

        WBPageBackground pageBackground() const
        {
            return mPageBackground;
        }

        int backgroundGridSize() const
        {
            return mBackgroundGridSize;
        }

        bool hasBackground()
        {
            return (mBackgroundObject != 0);
        }

        void addRuler(QPointF center);
        void addProtractor(QPointF center);
        void addCompass(QPointF center);
        void addTriangle(QPointF center);
        void addMagnifier(WBMagnifierParams params);

        void addMask(const QPointF &center = QPointF());
        void addCache();

        QList<QGraphicsItem*> getFastAccessItems()
        {
            return mFastAccessItems;
        }

        class SceneViewState
        {
            public:
                SceneViewState()
                {
                    zoomFactor = 1;
                    horizontalPosition = 0;
                    verticalPostition = 0;
                    mLastSceneCenter = QPointF();
                }

                SceneViewState(qreal pZoomFactor, int pHorizontalPosition, int pVerticalPostition, QPointF sceneCenter = QPointF())
                {
                    zoomFactor = pZoomFactor;
                    horizontalPosition = pHorizontalPosition;
                    verticalPostition = pVerticalPostition;
                    mLastSceneCenter = sceneCenter;
                }

                QPointF lastSceneCenter()
                {
                    return mLastSceneCenter;
                }

                void setLastSceneCenter(QPointF center)
                {
                    mLastSceneCenter = center;
                }

                QPointF mLastSceneCenter;

                qreal zoomFactor;
                int horizontalPosition;
                int verticalPostition;
        };

        SceneViewState viewState() const
        {
            return mViewState;
        }

        void setViewState(const SceneViewState& pViewState)
        {
            mViewState = pViewState;
        }

        virtual void setRenderingQuality(WBItem::RenderingQuality pRenderingQuality);

        QList<QUrl> relativeDependencies() const;

        QSize nominalSize();

        QSize sceneSize();

        void setNominalSize(const QSize& pSize);

        void setNominalSize(int pWidth, int pHeight);

        qreal changeZLevelTo(QGraphicsItem *item, WBZLayerController::moveDestination dest, bool addUndo=false);

        enum RenderingContext
        {
            Screen = 0, NonScreen, PdfExport, Podcast
        };

        void setRenderingContext(RenderingContext pRenderingContext)
        {
            mRenderingContext = pRenderingContext;
        }

        RenderingContext renderingContext() const
        {
            return mRenderingContext;
        }

        QSet<QGraphicsItem*> tools(){ return mTools;}

        void registerTool(QGraphicsItem* item)
        {
            mTools << item;
        }

        const QPointF& previousPoint()
        {
            return mPreviousPoint;
        }

        void setSelectedZLevel(QGraphicsItem *item);
        void setOwnZlevel(QGraphicsItem *item);

        static QUuid getPersonalUuid(QGraphicsItem *item);

        WBGraphicsPolygonItem* polygonToPolygonItem(const QPolygonF pPolygon);
        void clearSelectionFrame();
        WBBoardView *controlView();
        void notifyZChanged(QGraphicsItem *item, qreal zValue);
        void deselectAllItemsExcept(QGraphicsItem* graphicsItem);

        QRectF annotationsBoundingRect() const;

	public slots:
        void updateSelectionFrame();
        void updateSelectionFrameWrapper(int);
        void initStroke();
        void hideTool();

        void setBackground(bool pIsDark, WBPageBackground pBackground);
        void setBackgroundZoomFactor(qreal zoom);
        void setBackgroundGridSize(int pSize);
        void setDrawingMode(bool bModeDesktop);
        void deselectAllItems();

        WBGraphicsPixmapItem* addPixmap(const QPixmap& pPixmap,
										QGraphicsItem* replaceFor,
										const QPointF& pPos = QPointF(0,0),
										qreal scaleFactor = 1.0,
										bool pUseAnimation = false,
										bool useProxyForDocumentPath = false);

        void textUndoCommandAdded(WBGraphicsTextItem *textItem);

        void setToolCursor(int tool);

        void selectionChangedProcessing();
        void moveMagnifier();
        void moveMagnifier(QPoint newPos, bool forceGrab = false);
        void closeMagnifier();
        void zoomInMagnifier();
        void zoomOutMagnifier();
        void changeMagnifierMode(int mode);
        void resizedMagnifier(qreal newPercent);

    protected:
        WBGraphicsPolygonItem* lineToPolygonItem(const QLineF& pLine, const qreal& pWidth);
        WBGraphicsPolygonItem* lineToPolygonItem(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth);

        WBGraphicsPolygonItem* arcToPolygonItem(const QLineF& pStartRadius, qreal pSpanAngle, qreal pWidth);
        WBGraphicsPolygonItem* curveToPolygonItem(const QList<QPair<QPointF, qreal> > &points);
        WBGraphicsPolygonItem* curveToPolygonItem(const QList<QPointF> &points, qreal startWidth, qreal endWidth);
        void addPolygonItemToCurrentStroke(WBGraphicsPolygonItem* polygonItem);

        void initPolygonItem(WBGraphicsPolygonItem*);

        void drawEraser(const QPointF& pEndPoint, bool pressed = true);
        void redrawEraser(bool pressed);
        void hideEraser();
        void drawPointer(const QPointF& pEndPoint, bool isFirstDraw = false);
        void drawMarkerCircle(const QPointF& pEndPoint);
        void drawPenCircle(const QPointF& pEndPoint);
        void hideMarkerCircle();
        void hidePenCircle();
        void DisposeMagnifierQWidgets();

        virtual void keyReleaseEvent(QKeyEvent * keyEvent);

        void recolorAllItems();

        virtual void drawItems (QPainter * painter, int numItems,
                               QGraphicsItem * items[], const QStyleOptionGraphicsItem options[], QWidget * widget = 0);

        QGraphicsItem* rootItem(QGraphicsItem* item) const;

        virtual void drawBackground(QPainter *painter, const QRectF &rect);

    private:
        void setDocumentUpdated();
        void createEraiser();
        void createPointer();
        void createMarkerCircle();
        void createPenCircle();
        void updateEraserColor();
        void updateMarkerCircleColor();
        void updatePenCircleColor();
        bool hasTextItemWithFocus(WBGraphicsGroupContainerItem* item);
        void simplifyCurrentStroke();

        QGraphicsEllipseItem* mEraser;
        QGraphicsEllipseItem* mPointer;
        QGraphicsEllipseItem* mMarkerCircle;
        QGraphicsEllipseItem* mPenCircle;

        QSet<QGraphicsItem*> mAddedItems;
        QSet<QGraphicsItem*> mRemovedItems;

        WBDocumentProxy* mDocument;

        bool mDarkBackground;
        WBPageBackground mPageBackground;
        int mBackgroundGridSize;

        bool mIsDesktopMode;
        qreal mZoomFactor;

        QGraphicsItem* mBackgroundObject;

        QPointF mPreviousPoint;
        qreal mPreviousWidth;
        qreal mDistanceFromLastStrokePoint;

        QList<WBGraphicsPolygonItem*> mPreviousPolygonItems;

        SceneViewState mViewState;

        bool mInputDeviceIsPressed;

        QSet<QGraphicsItem*> mTools;

        WBGraphicsPolygonItem *mArcPolygonItem;

        QSize mNominalSize;

        RenderingContext mRenderingContext;

        WBGraphicsStroke* mCurrentStroke;

        int mItemCount;

        QList<QGraphicsItem*> mFastAccessItems;

        bool mHasCache;
        bool mUndoRedoStackEnabled;

        WBMagnifier *magniferControlViewWidget;
        WBMagnifier *magniferDisplayViewWidget;

        WBZLayerController *mZLayerController;
        WBGraphicsPolygonItem* mpLastPolygon;
        WBGraphicsPolygonItem* mTempPolygon;

        bool mDrawWithCompass;
        WBGraphicsPolygonItem *mCurrentPolygon;
        WBSelectionFrame *mSelectionFrame;
};

#endif /* WBGRAPHICSSCENE_H_ */
