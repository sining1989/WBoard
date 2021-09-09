#include "WBGraphicsScene.h"

#include <QtWidgets>
#include <QtWebEngine>
#include <QtSvg>
#include <QGraphicsView>
#include <QGraphicsVideoItem>

#include "frameworks/WBGeometryUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"
#include "core/WBPersistenceManager.h"
#include "core/WBTextTools.h"

#include "gui/WBMagnifer.h"
#include "gui/WBMainWindow.h"
#include "gui/WBToolWidget.h"
#include "gui/WBResources.h"

#include "tools/WBGraphicsRuler.h"
#include "tools/WBGraphicsProtractor.h"
#include "tools/WBGraphicsCompass.h"
#include "tools/WBGraphicsTriangle.h"
#include "tools/WBGraphicsCurtainItem.h"
#include "tools/WBGraphicsCache.h"

#include "document/WBDocumentProxy.h"

#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"
#include "board/WBBoardView.h"

#include "WBGraphicsItemUndoCommand.h"
#include "WBGraphicsItemGroupUndoCommand.h"
#include "WBGraphicsTextItemUndoCommand.h"
#include "WBGraphicsProxyWidget.h"
#include "WBGraphicsPixmapItem.h"
#include "WBGraphicsSvgItem.h"
#include "WBGraphicsPolygonItem.h"
#include "WBGraphicsMediaItem.h"
#include "WBGraphicsWidgetItem.h"
#include "WBGraphicsPDFItem.h"
#include "WBGraphicsTextItem.h"
#include "WBGraphicsStrokesGroup.h"
#include "WBSelectionFrame.h"
#include "WBGraphicsItemZLevelUndoCommand.h"

#include "domain/WBGraphicsGroupContainerItem.h"

#include "WBGraphicsStroke.h"

#include "core/memcheck.h"


#define DEFAULT_Z_VALUE 0.0

qreal WBZLayerController::errorNumber = -20000001.0;

WBZLayerController::WBZLayerController(QGraphicsScene *scene) :
    mScene(scene)

{
    scopeMap.insert(itemLayerType::NoLayer,        ItemLayerTypeData( errorNumber, errorNumber));
    scopeMap.insert(itemLayerType::BackgroundItem, ItemLayerTypeData(-1000000.0, -1000000.0 ));
    // DEFAULT_Z_VALUE isn't used because it allows to easily identify new objects
    scopeMap.insert(itemLayerType::ObjectItem,     ItemLayerTypeData(-1000000.0,  DEFAULT_Z_VALUE - 1.0));
    scopeMap.insert(itemLayerType::DrawingItem,    ItemLayerTypeData( DEFAULT_Z_VALUE + 1.0, 1000000.0 ));
    scopeMap.insert(itemLayerType::ToolItem,       ItemLayerTypeData( 1000000.0,  1000100.0 ));
    scopeMap.insert(itemLayerType::CppTool,        ItemLayerTypeData( 1000100.0,  1000200.0 ));
    scopeMap.insert(itemLayerType::Curtain,        ItemLayerTypeData( 1000200.0,  1001000.0 ));
    scopeMap.insert(itemLayerType::Eraiser,        ItemLayerTypeData( 1001000.0,  1001100.0 ));
    scopeMap.insert(itemLayerType::Pointer,        ItemLayerTypeData( 1001100.0,  1001200.0 ));
    scopeMap.insert(itemLayerType::Cache,          ItemLayerTypeData( 1001300.0,  1001400.0 ));

    scopeMap.insert(itemLayerType::SelectedItem,   ItemLayerTypeData( 1001000.0,  1001000.0 ));
    scopeMap.insert(itemLayerType::SelectionFrame, ItemLayerTypeData( 1010000.0,  1010000.0 ));
}

qreal WBZLayerController::generateZLevel(itemLayerType::Enum key)
{

    if (!scopeMap.contains(key)) {
        qDebug() << "Number is out of layer scope";
        return errorNumber;
    }

    qreal result = scopeMap.value(key).curValue;
    qreal top = scopeMap.value(key).topLimit;
    qreal incrementalStep = scopeMap.value(key).incStep;

    result += incrementalStep;
    if (result >= top) {
        // If not only one variable presents in the scope, notify that values for scope are over
        if (scopeMap.value(key).topLimit != scopeMap.value(key).bottomLimit) {
            qDebug() << "new values are over for the scope" << key;
        }
        result = top - incrementalStep;
    }

    scopeMap[key].curValue = result;

    return result;
}
qreal WBZLayerController::generateZLevel(QGraphicsItem *item)
{
    qreal result = errorNumber;
    itemLayerType::Enum type = static_cast<itemLayerType::Enum>(item->data(WBGraphicsItemData::itemLayerType).toInt());

    if (validLayerType(type)) {
        result =  generateZLevel(type);
    }

    return result;
}

qreal WBZLayerController::changeZLevelTo(QGraphicsItem *item, moveDestination dest)
{
    itemLayerType::Enum curItemLayerType = typeForData(item);
    if (curItemLayerType == itemLayerType::NoLayer) {
        qDebug() << "item's layer is out of the scope. Can't implement z-layer changing operation";
        return errorNum();
    }

    //select only items wiht the same z-level as item's one and push it to sortedItems QMultiMap
    QMultiMap<qreal, QGraphicsItem*> sortedItems;
    if (mScene->items().count()) {
        foreach (QGraphicsItem *tmpItem, mScene->items()) {
            if (typeForData(tmpItem) == curItemLayerType) {
                sortedItems.insert(tmpItem->data(WBGraphicsItemData::ItemOwnZValue).toReal(), tmpItem);
            }
        }
    }

    //If only one item itself - do nothing, return it's z-value
    if (sortedItems.count() == 1 && sortedItems.values().first() == item) {
        qDebug() << "only one item exists in layer. Have nothing to change";
        return item->data(WBGraphicsItemData::ItemOwnZValue).toReal();
    }

    QMapIterator<qreal, QGraphicsItem*>iCurElement(sortedItems);

    if (dest == up) {
        qDebug() << "item data zvalue= " << item->data(WBGraphicsItemData::ItemOwnZValue).toReal();
        if (iCurElement.findNext(item)) {
            if (iCurElement.hasNext()) {
                qreal nextZ = iCurElement.peekNext().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal();
                WBGraphicsItem::assignZValue(iCurElement.peekNext().value(), item->data(WBGraphicsItemData::ItemOwnZValue).toReal());
                WBGraphicsItem::assignZValue(item, nextZ);

                iCurElement.next();

                while (iCurElement.hasNext() && iCurElement.peekNext().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal() == nextZ) {
                    WBGraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }

    } else if (dest == top) {
        if (iCurElement.findNext(item)) {
            if (iCurElement.hasNext()) {
                WBGraphicsItem::assignZValue(item, generateZLevel(item));
            }
        }

    } else if (dest == down) {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item)) {
            if (iCurElement.hasPrevious()) {
                qreal nextZ = iCurElement.peekPrevious().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal();
                WBGraphicsItem::assignZValue(iCurElement.peekPrevious().value(), item->data(WBGraphicsItemData::ItemOwnZValue).toReal());
                WBGraphicsItem::assignZValue(item, nextZ);

                while (iCurElement.hasNext() && iCurElement.peekNext().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal() == nextZ) {
                        WBGraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }

    } else if (dest == bottom) {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item)) {
            if (iCurElement.hasPrevious()) {
                qreal oldz = item->data(WBGraphicsItemData::ItemOwnZValue).toReal();
                iCurElement.toFront();
                qreal nextZ = iCurElement.next().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal();

                ItemLayerTypeData curItemLayerTypeData = scopeMap.value(curItemLayerType);

                //if we have some free space between lowest graphics item and layer's bottom bound,
                //insert element close to first element in layer
                if (nextZ > curItemLayerTypeData.bottomLimit + curItemLayerTypeData.incStep) {
                    qreal result = nextZ - curItemLayerTypeData.incStep;
                    WBGraphicsItem::assignZValue(item, result);
                } else {
                    WBGraphicsItem::assignZValue(item, nextZ);

                    bool doubleGap = false; //to detect if we can finish rundown since we can insert item to the free space

                    while (iCurElement.peekNext().value() != item) {
                        qreal curZ = iCurElement.value()->data(WBGraphicsItemData::ItemOwnZValue).toReal();
                        qreal curNextZ = iCurElement.peekNext().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal();
                        if (curNextZ - curZ >= 2 * curItemLayerTypeData.incStep) {
                            WBGraphicsItem::assignZValue(iCurElement.value(), curZ + curItemLayerTypeData.incStep);
                            doubleGap = true;
                            break;
                        } else {
                            WBGraphicsItem::assignZValue(iCurElement.value(), curNextZ);
                            iCurElement.next();
                        }
                    }
                    if (!doubleGap) {

                        WBGraphicsItem::assignZValue(iCurElement.value(), oldz);

                        while (iCurElement.hasNext() && (iCurElement.peekNext().value()->data(WBGraphicsItemData::ItemOwnZValue).toReal() == oldz)) {
                            WBGraphicsItem::assignZValue(iCurElement.next().value(), oldz);
                        }
                    }
                }
            }
        }
    }


    //clear selection of the item and then select it again to activate selectionChangeProcessing()
    item->scene()->clearSelection();
    item->setSelected(true);

    foreach (QGraphicsItem *iitem, sortedItems.values()) {
        if (iitem)
            iitem != item ? qDebug() <<  "current value" << iitem->zValue() : qDebug() << "marked value" << QString::number(iitem->zValue(), 'f');
    }

    //Return new z value assigned to item
    
    // experimental
    item->setZValue(item->data(WBGraphicsItemData::ItemOwnZValue).toReal());

    return item->data(WBGraphicsItemData::ItemOwnZValue).toReal();
}

itemLayerType::Enum WBZLayerController::typeForData(QGraphicsItem *item) const
{
    itemLayerType::Enum result = static_cast<itemLayerType::Enum>(item->data(WBGraphicsItemData::itemLayerType).toInt());

    if (!scopeMap.contains(result)) {
        result = itemLayerType::NoLayer;
    }

    return result;
}

void WBZLayerController::setLayerType(QGraphicsItem *pItem, itemLayerType::Enum pNewType)
{
   pItem->setData(WBGraphicsItemData::itemLayerType, QVariant(pNewType));
}

void WBZLayerController::shiftStoredZValue(QGraphicsItem *item, qreal zValue)
{
    itemLayerType::Enum type = typeForData(item);

    if (validLayerType(type)) {
        ItemLayerTypeData typeData = scopeMap.value(type);
        if (typeData.curValue < zValue) {
            scopeMap[type].curValue = zValue;
        }
    }
}

/**
 * @brief Returns true if the zLevel is not used by any item on the scene, or false if so.
 */
bool WBZLayerController::zLevelAvailable(qreal z)
{
    foreach(QGraphicsItem* it, dynamic_cast<WBGraphicsScene*>(mScene)->getFastAccessItems()) {
        if (it->zValue() == z)
            return false;
    }

    return true;
}

WBGraphicsScene::WBGraphicsScene(WBDocumentProxy* parent, bool enableUndoRedoStack)
    : WBCoreGraphicsScene(parent)
    , mEraser(0)
    , mPointer(0)
    , mMarkerCircle(0)
    , mPenCircle(0)
    , mDocument(parent)
    , mDarkBackground(false)
    , mPageBackground(WBPageBackground::plain)
    , mIsDesktopMode(false)
    , mZoomFactor(1)
    , mBackgroundObject(0)
    , mPreviousWidth(0)
    , mDistanceFromLastStrokePoint(0)
    , mInputDeviceIsPressed(false)
    , mArcPolygonItem(0)
    , mRenderingContext(Screen)
    , mCurrentStroke(0)
    , mItemCount(0)
    , mUndoRedoStackEnabled(enableUndoRedoStack)
    , magniferControlViewWidget(0)
    , magniferDisplayViewWidget(0)
    , mZLayerController(new WBZLayerController(this))
    , mpLastPolygon(NULL)
    , mCurrentPolygon(0)
    , mTempPolygon(NULL)
    , mSelectionFrame(0)
{
    WBCoreGraphicsScene::setObjectName("BoardScene");
    setItemIndexMethod(BspTreeIndex);

    setUuid(QUuid::createUuid());
    setDocument(parent);
    createEraiser();
    createPointer();
    createMarkerCircle();
    createPenCircle();

    if (WBApplication::applicationController)
    {
        setViewState(SceneViewState(1,
            WBApplication::applicationController->initialHScroll(),
            WBApplication::applicationController->initialVScroll()));
    }

    mBackgroundGridSize = WBSettings::settings()->crossSize;

//    Just for debug. Do not delete please
//    connect(this, SIGNAL(selectionChanged()), this, SLOT(selectionChangedProcessing()));
    connect(WBApplication::undoStack.data(), SIGNAL(indexChanged(int)), this, SLOT(updateSelectionFrameWrapper(int)));
}

WBGraphicsScene::~WBGraphicsScene()
{
    if (mCurrentStroke && mCurrentStroke->polygons().empty()){
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    if (mZLayerController)
        delete mZLayerController;
}

void WBGraphicsScene::selectionChangedProcessing()
{
    if (selectedItems().count()){
        WBApplication::showMessage("ZValue is " + QString::number(selectedItems().first()->zValue(), 'f') + "own z value is "
                                   + QString::number(selectedItems().first()->data(WBGraphicsItemData::ItemOwnZValue).toReal(), 'f'));

    }
}

void WBGraphicsScene::setLastCenter(QPointF center)
{
    mViewState.setLastSceneCenter(center);
}

QPointF WBGraphicsScene::lastCenter()
{
    return mViewState.lastSceneCenter();
}

bool WBGraphicsScene::inputDevicePress(const QPointF& scenePos, const qreal& pressure)
{
    bool accepted = false;

    if (mInputDeviceIsPressed) {
        qWarning() << "scene received input device pressed, without input device release, muting event as input device move";
        accepted = inputDeviceMove(scenePos, pressure);
    }
    else {
        mInputDeviceIsPressed = true;

        WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

        if (WBDrawingController::drawingController()->isDrawingTool()) {
            // -----------------------------------------------------------------
            // We fall here if we are using the Pen, the Marker or the Line tool
            // -----------------------------------------------------------------
            qreal width = 0;

            // delete current stroke, if not assigned to any polygon
            if (mCurrentStroke && mCurrentStroke->polygons().empty()){
                delete mCurrentStroke;
                mCurrentStroke = NULL;
            }

            // hide the marker preview circle
            if (currentTool == WBStylusTool::Marker)
                hideMarkerCircle();

            // hide the pen preview circle
            if (currentTool == WBStylusTool::Pen)
                hidePenCircle();

            // ---------------------------------------------------------------
            // Create a new Stroke. A Stroke is a collection of QGraphicsLines
            // ---------------------------------------------------------------
            mCurrentStroke = new WBGraphicsStroke(this);

            if (currentTool != WBStylusTool::Line){
                // Handle the pressure
                width = WBDrawingController::drawingController()->currentToolWidth() * pressure;
            }
            else{
                // Ignore pressure for the line tool
                width = WBDrawingController::drawingController()->currentToolWidth();
            }

            width /= WBApplication::boardController->systemScaleFactor();
            width /= WBApplication::boardController->currentZoom();

            mAddedItems.clear();
            mRemovedItems.clear();

            if (WBDrawingController::drawingController()->mActiveRuler)
                WBDrawingController::drawingController()->mActiveRuler->StartLine(scenePos, width);
            else {
                moveTo(scenePos);
                drawLineTo(scenePos, width, WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Line);

                mCurrentStroke->addPoint(scenePos, width);
            }
            accepted = true;
        }
        else if (currentTool == WBStylusTool::Eraser) {
            mAddedItems.clear();
            mRemovedItems.clear();
            moveTo(scenePos);

            qreal eraserWidth = WBSettings::settings()->currentEraserWidth();
            eraserWidth /= WBApplication::boardController->systemScaleFactor();
            eraserWidth /= WBApplication::boardController->currentZoom();

            eraseLineTo(scenePos, eraserWidth);
            drawEraser(scenePos, mInputDeviceIsPressed);

            accepted = true;
        }
        else if (currentTool == WBStylusTool::Pointer) {
            drawPointer(scenePos, true);
            accepted = true;
        }
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty()){
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    return accepted;
}

bool WBGraphicsScene::inputDeviceMove(const QPointF& scenePos, const qreal& pressure)
{
    bool accepted = false;

    WBDrawingController *dc = WBDrawingController::drawingController();
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)dc->stylusTool();

    QPointF position = QPointF(scenePos);

    if (currentTool == WBStylusTool::Eraser)
    {
        drawEraser(position, mInputDeviceIsPressed);
        accepted = true;
    }

    else if (currentTool == WBStylusTool::Marker) {
        if (mInputDeviceIsPressed)
            hideMarkerCircle();
        else {
            drawMarkerCircle(position);
            accepted = true;
        }
    }

    else if (currentTool == WBStylusTool::Pen) {
        if (mInputDeviceIsPressed)
            hidePenCircle();
        else {
            drawPenCircle(position);
            accepted = true;
        }
    }

    if (mInputDeviceIsPressed)
    {
        if (dc->isDrawingTool())
        {
            qreal width = 0;

            if (currentTool != WBStylusTool::Line){
                // Handle the pressure
                width = dc->currentToolWidth() * qMax(pressure, 0.2);
            }else{
                // Ignore pressure for line tool
                width = dc->currentToolWidth();
            }

            width /= WBApplication::boardController->systemScaleFactor();
            width /= WBApplication::boardController->currentZoom();

            if (currentTool == WBStylusTool::Line || dc->mActiveRuler)
            {
                if (WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Marker)
                if(NULL != mpLastPolygon && NULL != mCurrentStroke && mAddedItems.size() > 0){
                    WBCoreGraphicsScene::removeItemFromDeletion(mpLastPolygon);
                    mAddedItems.remove(mpLastPolygon);
                    mCurrentStroke->remove(mpLastPolygon);
                    if (mCurrentStroke->polygons().empty()){
                        delete mCurrentStroke;
                        mCurrentStroke = NULL;
                    }
                    removeItem(mpLastPolygon);
                    mPreviousPolygonItems.removeAll(mpLastPolygon);
                }

                // ------------------------------------------------------------------------
                // Here we wanna make sure that the Line will 'grip' at i*45, i*90 degrees
                // ------------------------------------------------------------------------

                QLineF radius(mPreviousPoint, position);
                qreal angle = radius.angle();
                angle = qRound(angle / 45) * 45;
                qreal radiusLength = radius.length();
                QPointF newPosition(
                    mPreviousPoint.x() + radiusLength * cos((angle * PI) / 180),
                    mPreviousPoint.y() - radiusLength * sin((angle * PI) / 180));
                QLineF chord(position, newPosition);
                if (chord.length() < qMin((int)16, (int)(radiusLength / 20)))
                    position = newPosition;
            }

            if (!mCurrentStroke)
                mCurrentStroke = new WBGraphicsStroke(this);

            if(dc->mActiveRuler){
                dc->mActiveRuler->DrawLine(position, width);
            }

            else if (currentTool == WBStylusTool::Line) {
                drawLineTo(position, width, true);
            }

            else {
                bool interpolate = false;

                if ((currentTool == WBStylusTool::Pen && WBSettings::settings()->boardInterpolatePenStrokes->get().toBool())
                    || (currentTool == WBStylusTool::Marker && WBSettings::settings()->boardInterpolateMarkerStrokes->get().toBool()))
                {
                    interpolate = true;
                }


                // Don't draw segments smaller than a certain length. This can help with performance
                // (less polygons to draw) but mostly with making the curve look smooth.

                qreal antiScaleRatio = 1./(WBApplication::boardController->systemScaleFactor() * WBApplication::boardController->currentZoom());
                qreal MIN_DISTANCE = 10*antiScaleRatio; // arbitrary. Move to settings if relevant.
                qreal distance = QLineF(mPreviousPoint, scenePos).length();

                mDistanceFromLastStrokePoint += distance;

                if (mDistanceFromLastStrokePoint > MIN_DISTANCE) {
                    QList<QPair<QPointF, qreal> > newPoints = mCurrentStroke->addPoint(scenePos, width, interpolate);
                    if (newPoints.length() > 1)
                        drawCurve(newPoints);

                    mDistanceFromLastStrokePoint = 0;
                }

                if (interpolate) {
                    // Bezier curves aren't drawn all the way to the scenePos (they stop halfway between the previous and
                    // current scenePos), so we add a line from the last drawn position in the stroke and the
                    // scenePos, to make the drawing feel more responsive. This line is then deleted if a new segment is
                    // added to the stroke. (Or it is added to the stroke when we stop drawing)

                    if (mTempPolygon) {
                        removeItem(mTempPolygon);
                        mTempPolygon = NULL;
                    }

                    QPointF lastDrawnPoint = mCurrentStroke->points().last().first;

                    mTempPolygon = lineToPolygonItem(QLineF(lastDrawnPoint, scenePos), mPreviousWidth, width);
                    addItem(mTempPolygon);
                }
            }
        }
        else if (currentTool == WBStylusTool::Eraser)
        {
            qreal eraserWidth = WBSettings::settings()->currentEraserWidth();
            eraserWidth /= WBApplication::boardController->systemScaleFactor();
            eraserWidth /= WBApplication::boardController->currentZoom();

            eraseLineTo(position, eraserWidth);
        }
        else if (currentTool == WBStylusTool::Pointer)
        {
            drawPointer(position);
        }

        accepted = true;
    }

    return accepted;
}

bool WBGraphicsScene::inputDeviceRelease()
{
    bool accepted = false;

    if (mPointer)
    {
        mPointer->hide();
        accepted = true;
    }

    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

    if (currentTool == WBStylusTool::Eraser)
        redrawEraser(false);


    WBDrawingController *dc = WBDrawingController::drawingController();

    if (dc->isDrawingTool() || mDrawWithCompass)
    {
        if(mArcPolygonItem){

            WBGraphicsStrokesGroup* pStrokes = new WBGraphicsStrokesGroup();

            // Add the arc
            mAddedItems.remove(mArcPolygonItem);
            removeItem(mArcPolygonItem);
            WBCoreGraphicsScene::removeItemFromDeletion(mArcPolygonItem);
            mArcPolygonItem->setStrokesGroup(pStrokes);
            pStrokes->addToGroup(mArcPolygonItem);

            // Add the center cross
            foreach(QGraphicsItem* item, mAddedItems){
                mAddedItems.remove(item);
                removeItem(item);
                WBCoreGraphicsScene::removeItemFromDeletion(item);
                WBGraphicsPolygonItem* pi = qgraphicsitem_cast<WBGraphicsPolygonItem*>(item);
                if (pi)
                    pi->setStrokesGroup(pStrokes);
                pStrokes->addToGroup(item);
            }

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            mDrawWithCompass = false;
        }
        else if (mCurrentStroke){
            if (mTempPolygon) {
                WBGraphicsPolygonItem * poly = dynamic_cast<WBGraphicsPolygonItem*>(mTempPolygon->deepCopy());
                removeItem(mTempPolygon);
                mTempPolygon = NULL;
                addPolygonItemToCurrentStroke(poly);
            }

            // replace the stroke by a simplified version of it
            if ((currentTool == WBStylusTool::Pen && WBSettings::settings()->boardSimplifyPenStrokes->get().toBool())
                || (currentTool == WBStylusTool::Marker && WBSettings::settings()->boardSimplifyMarkerStrokes->get().toBool()))
            {
                simplifyCurrentStroke();
            }


            WBGraphicsStrokesGroup* pStrokes = new WBGraphicsStrokesGroup();

            // Remove the strokes that were just drawn here and replace them by a stroke item
            foreach(WBGraphicsPolygonItem* poly, mCurrentStroke->polygons()){
                mPreviousPolygonItems.removeAll(poly);
                removeItem(poly);
                WBCoreGraphicsScene::removeItemFromDeletion(poly);
                poly->setStrokesGroup(pStrokes);
                pStrokes->addToGroup(poly);
            }

            // TODO LATER : Generate well pressure-interpolated polygons and create the line group with them

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            if (mCurrentStroke->polygons().empty()){
                delete mCurrentStroke;
                mCurrentStroke = 0;
            }
            mCurrentPolygon = 0;
        }
    }

    if (mRemovedItems.size() > 0 || mAddedItems.size() > 0)
    {

        if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
            WBGraphicsItemUndoCommand* udcmd = new WBGraphicsItemUndoCommand(this, mRemovedItems, mAddedItems); //deleted by the undoStack

            if(WBApplication::undoStack)
                WBApplication::undoStack->push(udcmd);
        }

        mRemovedItems.clear();
        mAddedItems.clear();
        accepted = true;
    }

    mInputDeviceIsPressed = false;

    setDocumentUpdated();

    if (mCurrentStroke && mCurrentStroke->polygons().empty()){
        delete mCurrentStroke;
    }

    mCurrentStroke = NULL;
    return accepted;
}

void WBGraphicsScene::drawEraser(const QPointF &pPoint, bool pressed)
{
    if (mEraser) {
        qreal eraserWidth = WBSettings::settings()->currentEraserWidth();
        eraserWidth /= WBApplication::boardController->systemScaleFactor();
        eraserWidth /= WBApplication::boardController->currentZoom();

        qreal eraserRadius = eraserWidth / 2;

        mEraser->setRect(QRectF(pPoint.x() - eraserRadius, pPoint.y() - eraserRadius, eraserWidth, eraserWidth));
        redrawEraser(pressed);
    }
}

void WBGraphicsScene::redrawEraser(bool pressed)
{
    if (mEraser) {
        QPen pen = mEraser->pen();

        if(pressed)
            pen.setStyle(Qt::SolidLine);
        else
            pen.setStyle(Qt::DotLine);

        mEraser->setPen(pen);
        mEraser->show();
    }
}

void WBGraphicsScene::hideEraser()
{
    if (mEraser)
        mEraser->hide();
}

void WBGraphicsScene::drawPointer(const QPointF &pPoint, bool isFirstDraw)
{
    qreal pointerDiameter = WBSettings::pointerDiameter / WBApplication::boardController->currentZoom();
    qreal pointerRadius = pointerDiameter / 2;

    if (mPointer) {
        mPointer->setRect(QRectF(pPoint.x() - pointerRadius,
                                 pPoint.y() - pointerRadius,
                                 pointerDiameter,
                                 pointerDiameter));
        if(isFirstDraw) {
            mPointer->show();
        }
    }
}

void WBGraphicsScene::drawMarkerCircle(const QPointF &pPoint)
{
    if (mMarkerCircle) {
        qreal markerDiameter = WBSettings::settings()->currentMarkerWidth();
        markerDiameter /= WBApplication::boardController->systemScaleFactor();
        markerDiameter /= WBApplication::boardController->currentZoom();
        qreal markerRadius = markerDiameter/2;

        mMarkerCircle->setRect(QRectF(pPoint.x() - markerRadius, pPoint.y() - markerRadius,
                                      markerDiameter, markerDiameter));
        mMarkerCircle->show();
    }

}

void WBGraphicsScene::drawPenCircle(const QPointF &pPoint)
{
    if (mPenCircle && WBSettings::settings()->showPenPreviewCircle->get().toBool() &&
        WBSettings::settings()->currentPenWidth() >= WBSettings::settings()->penPreviewFromSize->get().toInt()) {
        qreal penDiameter = WBSettings::settings()->currentPenWidth();
        penDiameter /= WBApplication::boardController->systemScaleFactor();
        penDiameter /= WBApplication::boardController->currentZoom();
        qreal penRadius = penDiameter/2;

        mPenCircle->setRect(QRectF(pPoint.x() - penRadius, pPoint.y() - penRadius,
                                      penDiameter, penDiameter));

        if (controlView())
            if (controlView()->viewport())
                controlView()->viewport()->setCursor(QCursor (Qt::BlankCursor));

        mPenCircle->show();
    }
    else
    {
        if (controlView())
            if (controlView()->viewport())
                controlView()->viewport()->setCursor(WBResources::resources()->penCursor);
    }

}

void WBGraphicsScene::hideMarkerCircle()
{
    if (mMarkerCircle) {
        mMarkerCircle->hide();
    }
}

void WBGraphicsScene::hidePenCircle()
{
    if (mPenCircle)
        mPenCircle->hide();
}

// call this function when user release mouse button in Magnifier mode
void WBGraphicsScene::DisposeMagnifierQWidgets()
{
    if(magniferControlViewWidget)
    {
        magniferControlViewWidget->hide();
        magniferControlViewWidget->setParent(0);
        delete magniferControlViewWidget;
        magniferControlViewWidget = NULL;
    }

    if(magniferDisplayViewWidget)
    {
        magniferDisplayViewWidget->hide();
        magniferDisplayViewWidget->setParent(0);
        delete magniferDisplayViewWidget;
        magniferDisplayViewWidget = NULL;
    }
    // some time have crash here on access to app (when call from destructor when close WBoard app)
    // so i just add try/catch section here
    try
    {
        WBApplication::app()->restoreOverrideCursor();
    }
    catch (...)
    {
    }

}

void WBGraphicsScene::moveTo(const QPointF &pPoint)
{
    mPreviousPoint = pPoint;
    mPreviousWidth = -1.0;
    mPreviousPolygonItems.clear();
    mArcPolygonItem = 0;
    mDrawWithCompass = false;
}
void WBGraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &pWidth, bool bLineStyle)
{
    drawLineTo(pEndPoint, pWidth, pWidth, bLineStyle);

}

void WBGraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &startWidth, const qreal &endWidth, bool bLineStyle)
{
    if (mPreviousWidth == -1.0)
        mPreviousWidth = startWidth;

    qreal initialWidth = startWidth;
    if (initialWidth == endWidth)
        initialWidth = mPreviousWidth;

    if (bLineStyle) {
        QSetIterator<QGraphicsItem*> itItems(mAddedItems);

        while (itItems.hasNext()) {
            QGraphicsItem* item = itItems.next();
            removeItem(item);
        }
        mAddedItems.clear();
    }

    WBGraphicsPolygonItem *polygonItem = lineToPolygonItem(QLineF(mPreviousPoint, pEndPoint), initialWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    if (!bLineStyle) {
        mPreviousPoint = pEndPoint;
        mPreviousWidth = endWidth;
    }
}

void WBGraphicsScene::drawCurve(const QList<QPair<QPointF, qreal> >& points)
{
    WBGraphicsPolygonItem* polygonItem = curveToPolygonItem(points);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousPoint = points.last().first;
    mPreviousWidth = points.last().second;
}

void WBGraphicsScene::drawCurve(const QList<QPointF>& points, qreal startWidth, qreal endWidth)
{
    WBGraphicsPolygonItem* polygonItem = curveToPolygonItem(points, startWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousWidth = endWidth;
    mPreviousPoint = points.last();
}

void WBGraphicsScene::addPolygonItemToCurrentStroke(WBGraphicsPolygonItem* polygonItem)
{
    if (!polygonItem->brush().isOpaque())
    {
        // -------------------------------------------------------------------------------------
        // Here we substract the polygons that are overlapping in order to keep the transparency
        // -------------------------------------------------------------------------------------
        for (int i = 0; i < mPreviousPolygonItems.size(); i++)
        {
            WBGraphicsPolygonItem* previous = mPreviousPolygonItems.value(i);
            polygonItem->subtract(previous);
        }
    }

    mpLastPolygon = polygonItem;
    mAddedItems.insert(polygonItem);

    // Here we add the item to the scene
    addItem(polygonItem);
    if (!mCurrentStroke)
        mCurrentStroke = new WBGraphicsStroke(this);

    polygonItem->setStroke(mCurrentStroke);

    mPreviousPolygonItems.append(polygonItem);

}

void WBGraphicsScene::eraseLineTo(const QPointF &pEndPoint, const qreal &pWidth)
{
    const QLineF line(mPreviousPoint, pEndPoint);
    mPreviousPoint = pEndPoint;

    const QPolygonF eraserPolygon = WBGeometryUtils::lineToPolygon(line, pWidth);
    const QRectF eraserBoundingRect = eraserPolygon.boundingRect();

    QPainterPath eraserPath;
    eraserPath.addPolygon(eraserPolygon);

    // Get all the items that are intersecting with the eraser path
    QList<QGraphicsItem*> collidItems = items(eraserBoundingRect, Qt::IntersectsItemBoundingRect);

    QList<WBGraphicsPolygonItem*> intersectedItems;

    typedef QList<QPolygonF> POLYGONSLIST;
    QList<POLYGONSLIST> intersectedPolygons;

    #pragma omp parallel for
    for(int i=0; i<collidItems.size(); i++)
    {
        WBGraphicsPolygonItem *pi = qgraphicsitem_cast<WBGraphicsPolygonItem*>(collidItems[i]);
        if(pi == NULL)
            continue;

        QPainterPath itemPainterPath;
        itemPainterPath.addPolygon(pi->sceneTransform().map(pi->polygon()));

        if (eraserPath.contains(itemPainterPath))
        {
            #pragma omp critical
            {
                // Compete remove item
                intersectedItems << pi;
                intersectedPolygons << QList<QPolygonF>();
            }
        }
        else if (eraserPath.intersects(itemPainterPath))
        {
            itemPainterPath.setFillRule(Qt::WindingFill);
            QPainterPath newPath = itemPainterPath.subtracted(eraserPath);
            #pragma omp critical
            {
               intersectedItems << pi;
               intersectedPolygons << newPath.simplified().toFillPolygons(pi->sceneTransform().inverted());
            }
        }
    }

    for(int i=0; i<intersectedItems.size(); i++)
    {
        // item who intersects with eraser
        WBGraphicsPolygonItem *intersectedPolygonItem = intersectedItems[i];

        if (!intersectedPolygons[i].empty())
        {
            // intersected polygons generated as QList<QPolygon> QPainterPath::toFillPolygons(),
            // so each intersectedPolygonItem has one or couple of QPolygons who should be removed from it.
            for(int j = 0; j < intersectedPolygons[i].size(); j++)
            {
                // create small polygon from couple of polygons to replace particular erased polygon
                WBGraphicsPolygonItem* polygonItem = new WBGraphicsPolygonItem(intersectedPolygons[i][j], intersectedPolygonItem->parentItem());

                intersectedPolygonItem->copyItemParameters(polygonItem);
                polygonItem->setNominalLine(false);
                polygonItem->setStroke(intersectedPolygonItem->stroke());
                polygonItem->setStrokesGroup(intersectedPolygonItem->strokesGroup());
                intersectedPolygonItem->strokesGroup()->addToGroup(polygonItem);
                mAddedItems << polygonItem;
            }
        }

        //remove full polygon item for replace it by couple of polygons which creates the same stroke without a part intersects with eraser
         mRemovedItems << intersectedPolygonItem;

        QTransform t;
        bool bApplyTransform = false;
        if (intersectedPolygonItem->strokesGroup())
        {
            if (intersectedPolygonItem->strokesGroup()->parentItem())
            {
                bApplyTransform = true;
                t = intersectedPolygonItem->sceneTransform();
            }
            intersectedPolygonItem->strokesGroup()->removeFromGroup(intersectedPolygonItem);
        }
        removeItem(intersectedPolygonItem);
        if (bApplyTransform)
            intersectedPolygonItem->setTransform(t);
    }

    if (!intersectedItems.empty())
        setModified(true);
}

void WBGraphicsScene::drawArcTo(const QPointF& pCenterPoint, qreal pSpanAngle)
{
    mDrawWithCompass = true;
    if (mArcPolygonItem)
    {
        mAddedItems.remove(mArcPolygonItem);
        removeItem(mArcPolygonItem);
        mArcPolygonItem = 0;
    }
    qreal penWidth = WBSettings::settings()->currentPenWidth();
    penWidth /= WBApplication::boardController->systemScaleFactor();
    penWidth /= WBApplication::boardController->currentZoom();

    mArcPolygonItem = arcToPolygonItem(QLineF(pCenterPoint, mPreviousPoint), pSpanAngle, penWidth);
    mArcPolygonItem->setFillRule(Qt::WindingFill);
    mArcPolygonItem->setStroke(mCurrentStroke);
    mAddedItems.insert(mArcPolygonItem);
    addItem(mArcPolygonItem);

    setDocumentUpdated();
}

void WBGraphicsScene::setBackground(bool pIsDark, WBPageBackground pBackground)
{
    bool needRepaint = false;

    if (mDarkBackground != pIsDark)
    {
        mDarkBackground = pIsDark;

        updateEraserColor();
        updateMarkerCircleColor();
        updatePenCircleColor();
        recolorAllItems();

        needRepaint = true;
        setModified(true);
    }

    if (mPageBackground != pBackground)
    {
        mPageBackground = pBackground;
        needRepaint = true;
        setModified(true);
    }

    if (needRepaint)
    {
        foreach(QGraphicsView* view, views())
        {
            view->resetCachedContent();
        }
    }
}

void WBGraphicsScene::setBackgroundZoomFactor(qreal zoom)
{
    mZoomFactor = zoom;
}


void WBGraphicsScene::setBackgroundGridSize(int pSize)
{
    if (pSize > 0) {
        mBackgroundGridSize = pSize;
        setModified(true);

        foreach(QGraphicsView* view, views())
            view->resetCachedContent();
    }
}

void WBGraphicsScene::setDrawingMode(bool bModeDesktop)
{
    mIsDesktopMode = bModeDesktop;
}

void WBGraphicsScene::recolorAllItems()
{
    QMap<QGraphicsView*, QGraphicsView::ViewportUpdateMode> previousUpdateModes;
    foreach(QGraphicsView* view, views())
    {
        previousUpdateModes.insert(view, view->viewportUpdateMode());
        view->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }

    bool currentIslight = isLightBackground();
    foreach (QGraphicsItem *item, items()) {
        if (item->type() == WBGraphicsStrokesGroup::Type) {
            WBGraphicsStrokesGroup *curGroup = static_cast<WBGraphicsStrokesGroup*>(item);
            QColor compareColor =  curGroup->color(currentIslight ? WBGraphicsStrokesGroup::colorOnDarkBackground
                                                                  : WBGraphicsStrokesGroup::colorOnLightBackground);

            if (curGroup->color() == compareColor) {
                QColor newColor = curGroup->color(!currentIslight ? WBGraphicsStrokesGroup::colorOnDarkBackground
                                                                  : WBGraphicsStrokesGroup::colorOnLightBackground);
                curGroup->setColor(newColor);
            }
        }

        if (item->type() == WBGraphicsTextItem::Type)
        {
            WBGraphicsTextItem *textItem = static_cast<WBGraphicsTextItem*>(item);
            textItem->recolor();
        }
    }

    foreach(QGraphicsView* view, views())
    {
        view->setViewportUpdateMode(previousUpdateModes.value(view));
    }
}

WBGraphicsPolygonItem* WBGraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pWidth)
{
    WBGraphicsPolygonItem *polygonItem = new WBGraphicsPolygonItem(pLine, pWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}


WBGraphicsPolygonItem* WBGraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth)
{
    WBGraphicsPolygonItem *polygonItem = new WBGraphicsPolygonItem(pLine, pStartWidth, pEndWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void WBGraphicsScene::initPolygonItem(WBGraphicsPolygonItem* polygonItem)
{
    QColor colorOnDarkBG;
    QColor colorOnLightBG;

    if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Marker)
    {
        colorOnDarkBG = WBApplication::boardController->markerColorOnDarkBackground();
        colorOnLightBG = WBApplication::boardController->markerColorOnLightBackground();
    }
    else // settings->stylusTool() == WBStylusTool::Pen + failsafe
    {
        colorOnDarkBG = WBApplication::boardController->penColorOnDarkBackground();
        colorOnLightBG = WBApplication::boardController->penColorOnLightBackground();
    }

    if (mDarkBackground)
    {
        polygonItem->setColor(colorOnDarkBG);
    }
    else
    {
        polygonItem->setColor(colorOnLightBG);
    }

    //polygonItem->setColor(QColor(rand()%256, rand()%256, rand()%256, polygonItem->brush().color().alpha()));

    polygonItem->setColorOnDarkBackground(colorOnDarkBG);
    polygonItem->setColorOnLightBackground(colorOnLightBG);

    polygonItem->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Graphic));
}

WBGraphicsPolygonItem* WBGraphicsScene::arcToPolygonItem(const QLineF& pStartRadius, qreal pSpanAngle, qreal pWidth)
{
    QPolygonF polygon = WBGeometryUtils::arcToPolygon(pStartRadius, pSpanAngle, pWidth);

    return polygonToPolygonItem(polygon);
}

WBGraphicsPolygonItem* WBGraphicsScene::curveToPolygonItem(const QList<QPair<QPointF, qreal> >& points)
{
    QPolygonF polygon = WBGeometryUtils::curveToPolygon(points, false, true);

    return polygonToPolygonItem(polygon);

}

WBGraphicsPolygonItem* WBGraphicsScene::curveToPolygonItem(const QList<QPointF>& points, qreal startWidth, qreal endWidth)
{
    QPolygonF polygon = WBGeometryUtils::curveToPolygon(points, startWidth, endWidth);

    return polygonToPolygonItem(polygon);
}

void WBGraphicsScene::clearSelectionFrame()
{
    if (mSelectionFrame) {
        mSelectionFrame->setEnclosedItems(QList<QGraphicsItem*>());
    }
}

WBBoardView *WBGraphicsScene::controlView()
{
    WBBoardView *result = 0;
    foreach (QGraphicsView *view, views()) {
        if (view->objectName() == CONTROLVIEW_OBJ_NAME) {
            result = static_cast<WBBoardView*>(view);
        }
    }

    return result;
}

void WBGraphicsScene::notifyZChanged(QGraphicsItem *item, qreal zValue)
{
    mZLayerController->shiftStoredZValue(item, zValue);
}

void WBGraphicsScene::updateSelectionFrame()
{
    if (!mSelectionFrame) {
        mSelectionFrame = new WBSelectionFrame();
        addItem(mSelectionFrame);
    }

    QList<QGraphicsItem*> selItems = selectedItems();
    switch (selItems.count()) {
    case 0 : {
        mSelectionFrame->setVisible(false);
        mSelectionFrame->setEnclosedItems(selItems);
    } break;
    case 1: {
        mSelectionFrame->setVisible(false);
        mSelectionFrame->setEnclosedItems(QList<QGraphicsItem*>());

        WBGraphicsItemDelegate *itemDelegate = WBGraphicsItem::Delegate(selItems.first());
        itemDelegate->createControls();
        selItems.first()->setVisible(true);
        itemDelegate->showControls();

    } break;
    default: {
        mSelectionFrame->setVisible(true);
        mSelectionFrame->setEnclosedItems(selItems);
    } break;
    }
}

void WBGraphicsScene::updateSelectionFrameWrapper(int)
{
    updateSelectionFrame();
}

WBGraphicsPolygonItem* WBGraphicsScene::polygonToPolygonItem(const QPolygonF pPolygon)
{
    WBGraphicsPolygonItem *polygonItem = new WBGraphicsPolygonItem(pPolygon);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void WBGraphicsScene::hideTool()
{
    hideEraser();
    hideMarkerCircle();
    hidePenCircle();
}

void WBGraphicsScene::leaveEvent(QEvent * event)
{
    Q_UNUSED(event);
    hideTool();
}

WBGraphicsScene* WBGraphicsScene::sceneDeepCopy() const
{
    WBGraphicsScene* copy = new WBGraphicsScene(this->document(), this->mUndoRedoStackEnabled);

    copy->setBackground(this->isDarkBackground(), mPageBackground);
    copy->setBackgroundGridSize(mBackgroundGridSize);
    copy->setSceneRect(this->sceneRect());

    if (this->mNominalSize.isValid())
        copy->setNominalSize(this->mNominalSize);

    QListIterator<QGraphicsItem*> itItems(this->mFastAccessItems);
    QMap<WBGraphicsStroke*, WBGraphicsStroke*> groupClone;

    while (itItems.hasNext())
    {
        QGraphicsItem* item = itItems.next();
        QGraphicsItem* cloneItem = 0;
        WBItem* ubItem = dynamic_cast<WBItem*>(item);
        WBGraphicsStroke* stroke = dynamic_cast<WBGraphicsStroke*>(item);
        WBGraphicsGroupContainerItem* group = dynamic_cast<WBGraphicsGroupContainerItem*>(item);

        if(group){
            WBGraphicsGroupContainerItem* groupCloned = group->deepCopyNoChildDuplication();
            groupCloned->resetMatrix();
            groupCloned->resetTransform();
            groupCloned->setPos(0, 0);
            bool locked = groupCloned->Delegate()->isLocked();

            foreach(QGraphicsItem* eachItem ,group->childItems()){
                QGraphicsItem* copiedChild = dynamic_cast<QGraphicsItem*>(dynamic_cast<WBItem*>(eachItem)->deepCopy());
                copy->addItem(copiedChild);
                groupCloned->addToGroup(copiedChild);
            }

            if (locked)
                groupCloned->setData(WBGraphicsItemData::ItemLocked, QVariant(true));

            copy->addItem(groupCloned);
            groupCloned->setMatrix(group->matrix());
            groupCloned->setTransform(QTransform::fromTranslate(group->pos().x(), group->pos().y()));
            groupCloned->setTransform(group->transform(), true);
        }

        if (ubItem && !stroke && !group && item->isVisible())
            cloneItem = dynamic_cast<QGraphicsItem*>(ubItem->deepCopy());

        if (cloneItem)
        {
            copy->addItem(cloneItem);

            if (isBackgroundObject(item))
                copy->setAsBackgroundObject(cloneItem);

            if (this->mTools.contains(item))
                copy->mTools << cloneItem;

            WBGraphicsPolygonItem* polygon = dynamic_cast<WBGraphicsPolygonItem*>(item);

            if(polygon)
            {
                WBGraphicsStroke* stroke = dynamic_cast<WBGraphicsStroke*>(item->parentItem());

                if (stroke)
                {
                    WBGraphicsStroke* cloneStroke = groupClone.value(stroke);

                    if (!cloneStroke)
                    {
                        cloneStroke = stroke->deepCopy();
                        groupClone.insert(stroke, cloneStroke);
                    }

                    polygon->setStroke(cloneStroke);
                }
            }
        }
    }

    return copy;
}

WBItem* WBGraphicsScene::deepCopy() const
{
    return sceneDeepCopy();
}

void WBGraphicsScene::clearContent(clearCase pCase)
{
    QSet<QGraphicsItem*> removedItems;
    WBGraphicsItemUndoCommand::GroupDataTable groupsMap;

    switch (pCase) {
    case clearBackground :
        if(mBackgroundObject){
            removeItem(mBackgroundObject);
            removedItems << mBackgroundObject;
        }
        break;

    case clearItemsAndAnnotations :
    case clearItems :
    case clearAnnotations :
        foreach(QGraphicsItem* item, items()) {

            WBGraphicsGroupContainerItem *itemGroup = item->parentItem()
                                                      ? qgraphicsitem_cast<WBGraphicsGroupContainerItem*>(item->parentItem())
                                                      : 0;
            WBGraphicsItemDelegate *curDelegate = WBGraphicsItem::Delegate(item);
            if (!curDelegate) {
                continue;
            }

            bool isGroup = item->type() == WBGraphicsGroupContainerItem::Type;
            bool isStrokesGroup = item->type() == WBGraphicsStrokesGroup::Type;

            bool shouldDelete = false;
            switch (static_cast<int>(pCase)) {
            case clearAnnotations :
                shouldDelete = isStrokesGroup;
                break;
            case clearItems :
                shouldDelete = !isGroup && !isBackgroundObject(item) && !isStrokesGroup;
                break;
            case clearItemsAndAnnotations:
                shouldDelete = !isGroup && !isBackgroundObject(item);
                break;
            }

            if(shouldDelete) {
                if (itemGroup) {
                    itemGroup->removeFromGroup(item);

                    groupsMap.insert(itemGroup, WBGraphicsItem::getOwnUuid(item));
                    if (itemGroup->childItems().count() == 1) {
                        groupsMap.insert(itemGroup, WBGraphicsItem::getOwnUuid(itemGroup->childItems().first()));
                        QGraphicsItem *lastItem = itemGroup->childItems().first();
                        bool isSelected = itemGroup->isSelected();
                        itemGroup->destroy(false);
                        lastItem->setSelected(isSelected);
                    }
                    itemGroup->Delegate()->update();
                }

                curDelegate->remove(false);
                removedItems << item;
            }
        }
        break;
    }

    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    update(sceneRect());

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented

        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, removedItems, QSet<QGraphicsItem*>(), groupsMap);
        WBApplication::undoStack->push(uc);
    }

    if (pCase == clearBackground) {
        mBackgroundObject = 0;
    }

    setDocumentUpdated();
}

WBGraphicsPixmapItem* WBGraphicsScene::addPixmap(const QPixmap& pPixmap, QGraphicsItem* replaceFor, const QPointF& pPos, qreal pScaleFactor, bool pUseAnimation, bool useProxyForDocumentPath)
{
    WBGraphicsPixmapItem* pixmapItem = new WBGraphicsPixmapItem();

    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    pixmapItem->setPixmap(pPixmap);

    QPointF half(pPixmap.width() * pScaleFactor / 2, pPixmap.height()  * pScaleFactor / 2);
    pixmapItem->setPos(pPos - half);

    addItem(pixmapItem);

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, replaceFor, pixmapItem);
        WBApplication::undoStack->push(uc);
    }

    pixmapItem->setTransform(QTransform::fromScale(pScaleFactor, pScaleFactor), true);

    if (pUseAnimation)
    {
        pixmapItem->setOpacity(0);

        QPropertyAnimation *animation = new QPropertyAnimation(pixmapItem, "opacity");
        animation->setDuration(1000);
        animation->setStartValue(0.0);
        animation->setEndValue(1.0);

        animation->start();
    }

    pixmapItem->show();
    setDocumentUpdated();

    QString documentPath;
    if(useProxyForDocumentPath)
        documentPath = this->document()->persistencePath();
    else
        documentPath = WBApplication::boardController->selectedDocument()->persistencePath();

    QString fileName = WBPersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";

    QString path = documentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(documentPath + "/" + WBPersistenceManager::imageDirectory);

        pixmapItem->pixmap().toImage().save(path, "PNG");
    }

    return pixmapItem;
}

void WBGraphicsScene::textUndoCommandAdded(WBGraphicsTextItem *textItem)
{
    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsTextItemUndoCommand* uc = new WBGraphicsTextItemUndoCommand(textItem);
        WBApplication::undoStack->push(uc);
    }
}
WBGraphicsMediaItem* WBGraphicsScene::addMedia(const QUrl& pMediaFileUrl, bool shouldPlayAsap, const QPointF& pPos)
{
    qDebug() << pMediaFileUrl.toLocalFile();
    if (!QFile::exists(pMediaFileUrl.toLocalFile()))
    if (!QFile::exists(pMediaFileUrl.toString()))
        return NULL;

    WBGraphicsMediaItem * mediaItem = WBGraphicsMediaItem::createMediaItem(pMediaFileUrl);

    if(mediaItem)
        connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), mediaItem, SLOT(activeSceneChanged()));

    mediaItem->setPos(pPos);

    mediaItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    mediaItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(mediaItem);

    mediaItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, mediaItem);
        WBApplication::undoStack->push(uc);
    }

    if (shouldPlayAsap)
        mediaItem->play();

    setDocumentUpdated();

    return mediaItem;
}

WBGraphicsMediaItem* WBGraphicsScene::addVideo(const QUrl& pVideoFileUrl, bool shouldPlayAsap, const QPointF& pPos)
{
   return addMedia(pVideoFileUrl, shouldPlayAsap, pPos);
}

WBGraphicsMediaItem* WBGraphicsScene::addAudio(const QUrl& pAudioFileUrl, bool shouldPlayAsap, const QPointF& pPos)
{
   return addMedia(pAudioFileUrl, shouldPlayAsap, pPos);
}

WBGraphicsWidgetItem* WBGraphicsScene::addWidget(const QUrl& pWidgetUrl, const QPointF& pPos)
{
    int widgetType = WBGraphicsWidgetItem::widgetType(pWidgetUrl);

    if(widgetType == WBWidgetType::Apple)
    {
        return addAppleWidget(pWidgetUrl, pPos);
    }
    else if(widgetType == WBWidgetType::W3C)
    {
        return addW3CWidget(pWidgetUrl, pPos);
    }
    else
    {
        qDebug() << "WBGraphicsScene::addWidget: Unknown widget Type";
        return 0;
    }
}

WBGraphicsAppleWidgetItem* WBGraphicsScene::addAppleWidget(const QUrl& pWidgetUrl, const QPointF& pPos)
{
    WBGraphicsAppleWidgetItem *appleWidget = new WBGraphicsAppleWidgetItem(pWidgetUrl);

    addGraphicsWidget(appleWidget, pPos);

    return appleWidget;
}

WBGraphicsW3CWidgetItem* WBGraphicsScene::addW3CWidget(const QUrl& pWidgetUrl, const QPointF& pPos)
{
    WBGraphicsW3CWidgetItem *w3CWidget = new WBGraphicsW3CWidgetItem(pWidgetUrl, 0);

    addGraphicsWidget(w3CWidget, pPos);

    return w3CWidget;
}

void WBGraphicsScene::addGraphicsWidget(WBGraphicsWidgetItem* graphicsWidget, const QPointF& pPos)
{
    graphicsWidget->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(graphicsWidget);

    qreal ssf = 1 / WBApplication::boardController->systemScaleFactor();

    graphicsWidget->setTransform(QTransform::fromScale(ssf, ssf), true);

    graphicsWidget->setPos(QPointF(pPos.x() - graphicsWidget->boundingRect().width() / 2,
        pPos.y() - graphicsWidget->boundingRect().height() / 2));

    if (graphicsWidget->canBeContent())
    {
        graphicsWidget->loadMainHtml();

        graphicsWidget->setSelected(true);
        if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
            WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, graphicsWidget);
            WBApplication::undoStack->push(uc);
        }

        setDocumentUpdated();
    }
    else
    {
        WBApplication::boardController->moveGraphicsWidgetToControlView(graphicsWidget);
    }

    WBApplication::boardController->controlView()->setFocus();
}



WBGraphicsW3CWidgetItem* WBGraphicsScene::addOEmbed(const QUrl& pContentUrl, const QPointF& pPos)
{
    QStringList widgetPaths = WBPersistenceManager::persistenceManager()->allWidgets(WBSettings::settings()->applicationApplicationsLibraryDirectory());

    WBGraphicsW3CWidgetItem *widget = 0;

    foreach(QString widgetPath, widgetPaths)
    {
        if (widgetPath.contains("VideoPicker"))
        {
            widget = addW3CWidget(QUrl::fromLocalFile(widgetPath), pPos);

            if (widget)
            {
                widget->setPreference("oembedUrl", pContentUrl.toString());
                setDocumentUpdated();
                break;
            }
        }
    }

    return widget;
}

WBGraphicsGroupContainerItem *WBGraphicsScene::createGroup(QList<QGraphicsItem *> items)
{
    WBGraphicsGroupContainerItem *groupItem = new WBGraphicsGroupContainerItem();

    addItem(groupItem);
    foreach (QGraphicsItem *item, items) {
        if (item->type() == WBGraphicsGroupContainerItem::Type) {
            QList<QGraphicsItem*> childItems = item->childItems();
            WBGraphicsGroupContainerItem *currentGroup = dynamic_cast<WBGraphicsGroupContainerItem*>(item);
            if (currentGroup) {
                currentGroup->destroy();
            }
            foreach (QGraphicsItem *chItem, childItems) {
                groupItem->addToGroup(chItem);
                mFastAccessItems.removeAll(chItem);
            }
        } else {
            groupItem->addToGroup(item);
            mFastAccessItems.removeAll(item);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemGroupUndoCommand* uc = new WBGraphicsItemGroupUndoCommand(this, groupItem);
        WBApplication::undoStack->push(uc);
    }

    setDocumentUpdated();

    return groupItem;
}

void WBGraphicsScene::addGroup(WBGraphicsGroupContainerItem *groupItem)
{
    addItem(groupItem);
    for (int i = 0; i < groupItem->childItems().count(); i++)
    {
        QGraphicsItem *it = qgraphicsitem_cast<QGraphicsItem *>(groupItem->childItems().at(i));
        if (it)
        {
             mFastAccessItems.removeAll(it);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (groupItem->uuid().isNull()) {
        groupItem->setUuid(QUuid::createUuid());
    }

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, groupItem);
        WBApplication::undoStack->push(uc);
    }

    setDocumentUpdated();
}

WBGraphicsSvgItem* WBGraphicsScene::addSvg(const QUrl& pSvgFileUrl, const QPointF& pPos, const QByteArray pData)
{
    QString path = pSvgFileUrl.toLocalFile();

    WBGraphicsSvgItem *svgItem;
    if (pData.isNull())
        svgItem = new WBGraphicsSvgItem(path);
    else
        svgItem = new WBGraphicsSvgItem(pData);

    svgItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    svgItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    qreal sscale = 1 / WBApplication::boardController->systemScaleFactor();
    svgItem->setTransform(QTransform::fromScale(sscale, sscale), true);

    QPointF half(svgItem->boundingRect().width() / 2, svgItem->boundingRect().height() / 2);
    svgItem->setPos(pPos - half);
    svgItem->show();

    addItem(svgItem);

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, svgItem);
        WBApplication::undoStack->push(uc);
    }

    setDocumentUpdated();

    QString documentPath = WBApplication::boardController->selectedDocument()->persistencePath();

    QString fileName = WBPersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";

    QString completePath = documentPath + "/" + fileName;

    if (!QFile::exists(completePath))
    {
        QDir dir;
        dir.mkdir(documentPath + "/" + WBPersistenceManager::imageDirectory);

        QFile file(completePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "cannot open file for writing embeded svg content " << completePath;
            return NULL;
        }

        file.write(svgItem->fileData());
        file.close();
    }

    return svgItem;
}

WBGraphicsTextItem* WBGraphicsScene::addText(const QString& pString, const QPointF& pTopLeft)
{
    return addTextWithFont(pString, pTopLeft, WBSettings::settings()->fontPixelSize()
            , WBSettings::settings()->fontFamily(), WBSettings::settings()->isBoldFont()
            , WBSettings::settings()->isItalicFont());
}

WBGraphicsTextItem* WBGraphicsScene::addTextWithFont(const QString& pString, const QPointF& pTopLeft
            , int pointSize, const QString& fontFamily, bool bold, bool italic)
{
    WBGraphicsTextItem *textItem = new WBGraphicsTextItem();
    textItem->setPlainText(pString);

    QFont font = textItem->font();

    if (fontFamily == "")
    {
        font = QFont(WBSettings::settings()->fontFamily());
    }
    else
    {
        font = QFont(fontFamily);
    }

    if (pointSize < 1)
    {
        font.setPixelSize(WBSettings::settings()->fontPixelSize());
    }
    else
    {
        font.setPointSize(pointSize);
    }

    font.setBold(bold);
    font.setItalic(italic);
    textItem->setFont(font);

    QFontMetrics fi(font);
    QRect br = fi.boundingRect(pString);

    textItem->setTextWidth(qMax((qreal)br.width() + 50, (qreal)200));
    textItem->setTextHeight(br.height());

    addItem(textItem);

    textItem->setPos(pTopLeft);

    textItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, textItem);
        WBApplication::undoStack->push(uc);
    }

    connect(textItem, SIGNAL(textUndoCommandAdded(WBGraphicsTextItem *)), this, SLOT(textUndoCommandAdded(WBGraphicsTextItem *)));

    textItem->setSelected(true);
    textItem->setFocus();

    setDocumentUpdated();

    return textItem;
}

WBGraphicsTextItem *WBGraphicsScene::addTextHtml(const QString &pString, const QPointF& pTopLeft)
{
    WBGraphicsTextItem *textItem = new WBGraphicsTextItem();
    textItem->setPlainText("");
    textItem->setHtml(WBTextTools::cleanHtml(pString));

    addItem(textItem);
    textItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(this, 0, textItem);
        WBApplication::undoStack->push(uc);
    }

    connect(textItem, SIGNAL(textUndoCommandAdded(WBGraphicsTextItem *)), this, SLOT(textUndoCommandAdded(WBGraphicsTextItem *)));

    textItem->setFocus();

    setDocumentUpdated();
    textItem->setPos(pTopLeft);

    return textItem;
}

void WBGraphicsScene::addItem(QGraphicsItem* item)
{
    WBCoreGraphicsScene::addItem(item);

    // the default z value is already set. This is the case when a svg file is read
    if(item->zValue() == DEFAULT_Z_VALUE
            || item->zValue() == WBZLayerController::errorNum()
            || !mZLayerController->zLevelAvailable(item->zValue()))
    {
        qreal zvalue = mZLayerController->generateZLevel(item);
        WBGraphicsItem::assignZValue(item, zvalue);
    }

    else
        notifyZChanged(item, item->zValue());

    if (!mTools.contains(item))
      ++mItemCount;

    mFastAccessItems << item;
}

void WBGraphicsScene::addItems(const QSet<QGraphicsItem*>& items)
{
    foreach(QGraphicsItem* item, items) {
        WBCoreGraphicsScene::addItem(item);
        WBGraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));
    }

    mItemCount += items.size();

    mFastAccessItems += items.toList();
}

void WBGraphicsScene::removeItem(QGraphicsItem* item)
{
    item->setSelected(false);
    WBCoreGraphicsScene::removeItem(item);
    WBApplication::boardController->freezeW3CWidget(item, true);

    if (!mTools.contains(item))
      --mItemCount;

    mFastAccessItems.removeAll(item);
    /* delete the item if it is cache to allow its reinstanciation, because Cache implements design pattern Singleton. */
    if (dynamic_cast<WBGraphicsCache*>(item))
        WBCoreGraphicsScene::deleteItem(item);
}

void WBGraphicsScene::removeItems(const QSet<QGraphicsItem*>& items)
{
    foreach(QGraphicsItem* item, items)
        WBCoreGraphicsScene::removeItem(item);

    mItemCount -= items.size();

    foreach(QGraphicsItem* item, items)
        mFastAccessItems.removeAll(item);
}

void WBGraphicsScene::deselectAllItems()
{
    foreach(QGraphicsItem *gi, selectedItems ())
    {
        gi->clearFocus();
        gi->setSelected(false);
        // Hide selection frame
        if (mSelectionFrame) {
            mSelectionFrame->setEnclosedItems(QList<QGraphicsItem*>());
        }
        WBGraphicsTextItem* textItem = dynamic_cast<WBGraphicsTextItem*>(gi);
        if(textItem)
            textItem->activateTextEditor(false);
    }
}

void WBGraphicsScene::deselectAllItemsExcept(QGraphicsItem* item)
{
    foreach(QGraphicsItem* eachItem,selectedItems()){
        if(eachItem != item){
            eachItem->setSelected(false);

            WBGraphicsTextItem* textItem = dynamic_cast<WBGraphicsTextItem*>(eachItem);
            if(textItem)
                textItem->activateTextEditor(false);
        }
    }
}

/**
 * Return the bounding rectangle of all items on the page except for tools (ruler, compass,...)
 */
QRectF WBGraphicsScene::annotationsBoundingRect() const
{
    QRectF boundingRect;

    foreach (QGraphicsItem *item, items()) {
        if (!mTools.contains(rootItem(item)))
            boundingRect |= item->sceneBoundingRect();
    }

    return boundingRect;
}

bool WBGraphicsScene::isEmpty() const
{
    return mItemCount == 0;
}

QGraphicsItem* WBGraphicsScene::setAsBackgroundObject(QGraphicsItem* item, bool pAdaptTransformation, bool pExpand)
{
    if (mBackgroundObject)
    {
        removeItem(mBackgroundObject);
        mBackgroundObject = 0;
    }

    if (item)
    {
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->setAcceptedMouseButtons(Qt::NoButton);
        item->setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::FixedBackground);

        if (pAdaptTransformation)
        {
            item = scaleToFitDocumentSize(item, true, 0, pExpand);
        }

        if (item->scene() != this)
            addItem(item);

        mZLayerController->setLayerType(item, itemLayerType::BackgroundItem);
        WBGraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));

        mBackgroundObject = item;

    }

    return item;
}

void WBGraphicsScene::unsetBackgroundObject()
{
    if (!mBackgroundObject)
        return;

    mBackgroundObject->setFlag(QGraphicsItem::ItemIsSelectable, true);
    mBackgroundObject->setFlag(QGraphicsItem::ItemIsMovable, true);
    mBackgroundObject->setAcceptedMouseButtons(Qt::LeftButton);

    // Item zLayer and Layer Type should be set by the caller of this function, as
    // it may depend on the object type, where it was before, etc.

    mBackgroundObject = 0;
}

QRectF WBGraphicsScene::normalizedSceneRect(qreal ratio)
{

    QRectF normalizedRect(nominalSize().width() / -2, nominalSize().height() / -2,
        nominalSize().width(), nominalSize().height());

    foreach(QGraphicsItem* gi, mFastAccessItems)
    {
        if(gi && gi->isVisible() && !mTools.contains(gi))
        {
            normalizedRect = normalizedRect.united(gi->sceneBoundingRect());
        }
    }

    if (ratio > 0.0)
    {
        qreal normalizedRectRatio = normalizedRect.width() / normalizedRect.height();

        if (normalizedRectRatio > ratio)
        {
            //the normalized rect is too wide, we increase height
            qreal newHeight = normalizedRect.width() / ratio;
            qreal offset = (newHeight - normalizedRect.height()) / 2;
            normalizedRect.setY(normalizedRect.y() - offset);
            normalizedRect.setHeight(newHeight);
        }
        else if (normalizedRectRatio < ratio)
        {
            //the normalized rect is too high, we increase the width
            qreal newWidth = normalizedRect.height() * ratio;
            qreal offset = (newWidth - normalizedRect.width()) / 2;
            normalizedRect.setX(normalizedRect.x() - offset);
            normalizedRect.setWidth(newWidth);
        }
    }

    return normalizedRect;
}

QGraphicsItem *WBGraphicsScene::itemForUuid(QUuid uuid)
{
    QGraphicsItem *result = 0;
    QString ui = uuid.toString();

    //simple search before implementing container for fast access
    foreach (QGraphicsItem *item, items()) {
        if (WBGraphicsScene::getPersonalUuid(item) == uuid && !uuid.isNull()) {
            result = item;
        }
    }

    return result;
}

void WBGraphicsScene::setDocument(WBDocumentProxy* pDocument)
{
    if (pDocument != mDocument)
    {
        if (mDocument)
        {
            setModified(true);
        }

        mDocument = pDocument;
        setParent(pDocument);
    }
}

QGraphicsItem* WBGraphicsScene::scaleToFitDocumentSize(QGraphicsItem* item, bool center, int margin, bool expand)
{
    int maxWidth = mNominalSize.width() - (margin * 2);
    int maxHeight = mNominalSize.height() - (margin * 2);

    QRectF size = item->sceneBoundingRect();

    if (expand || size.width() > maxWidth || size.height() > maxHeight)
    {
        qreal ratio = qMin(maxWidth / size.width(), maxHeight / size.height());

        item->setTransform(QTransform::fromScale(ratio, ratio), true);

        if(center)
        {
            item->setPos(item->sceneBoundingRect().width() / -2.0,
                item->sceneBoundingRect().height() / -2.0);
        }
    }

    return item;
}

void WBGraphicsScene::addRuler(QPointF center)
{
    WBGraphicsRuler* ruler = new WBGraphicsRuler(); // mem : owned and destroyed by the scene
    mTools << ruler;
    QRectF rect = ruler->rect();
    ruler->setRect(center.x() - rect.width()/2, center.y() - rect.height()/2, rect.width(), rect.height());

    ruler->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));

    addItem(ruler);

    ruler->setVisible(true);
}

void WBGraphicsScene::addProtractor(QPointF center)
{
    // Protractor

    WBGraphicsProtractor* protractor = new WBGraphicsProtractor(); // mem : owned and destroyed by the scene
    mTools << protractor;

    protractor->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));

    addItem(protractor);

    QPointF itemSceneCenter = protractor->sceneBoundingRect().center();
    protractor->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    protractor->setVisible(true);
}

void WBGraphicsScene::addTriangle(QPointF center)
{
    // Triangle

    WBGraphicsTriangle* triangle = new WBGraphicsTriangle(); // mem : owned and destroyed by the scene
    mTools << triangle;

    triangle->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));

    addItem(triangle);

    QPointF itemSceneCenter = triangle->sceneBoundingRect().center();
    triangle->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    triangle->setVisible(true);
}

void WBGraphicsScene::addMagnifier(WBMagnifierParams params)
{
    // can have only one magnifier at one time
    if(magniferControlViewWidget) return;

    QWidget *cContainer = (QWidget*)(WBApplication::boardController->controlContainer());
    QGraphicsView *cView = (QGraphicsView*)WBApplication::boardController->controlView();
    QGraphicsView *dView = (QGraphicsView*)WBApplication::boardController->displayView();

    QPoint dvZeroPoint = dView->mapToGlobal(QPoint(0,0));

    int cvW = cView->width();
    int dvW = dView->width();
    qreal wCoeff = (qreal)dvW / (qreal)cvW;

    int cvH = cView->height();
    int dvH = dView->height();
    qreal hCoeff = (qreal)dvH / (qreal)cvH;

    QPoint ccPoint(params.x,params.y);
    QPoint globalPoint = cContainer->mapToGlobal(ccPoint);
    QPoint cvPoint = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint( cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magniferControlViewWidget = new WBMagnifier((QWidget*)(WBApplication::boardController->controlContainer()), true);
    magniferControlViewWidget->setGrabView((QGraphicsView*)WBApplication::boardController->controlView());
    magniferControlViewWidget->setMoveView((QWidget*)(WBApplication::boardController->controlContainer()));
    magniferControlViewWidget->setSize(params.sizePercentFromScene);
    magniferControlViewWidget->setZoom(params.zoom);

    magniferDisplayViewWidget = new WBMagnifier((QWidget*)(WBApplication::boardController->displayView()), false);
    magniferDisplayViewWidget->setGrabView((QGraphicsView*)WBApplication::boardController->controlView());
    magniferDisplayViewWidget->setMoveView((QGraphicsView*)WBApplication::boardController->displayView());
    magniferDisplayViewWidget->setSize(params.sizePercentFromScene);
    magniferDisplayViewWidget->setZoom(params.zoom);

    magniferControlViewWidget->grabNMove(globalPoint, globalPoint, true);
    magniferDisplayViewWidget->grabNMove(globalPoint, dvPoint, true);
    magniferControlViewWidget->show();
    magniferDisplayViewWidget->show();

    connect(magniferControlViewWidget, SIGNAL(magnifierMoved_Signal(QPoint)), this, SLOT(moveMagnifier(QPoint)));
    connect(magniferControlViewWidget, SIGNAL(magnifierClose_Signal()), this, SLOT(closeMagnifier()));
    connect(magniferControlViewWidget, SIGNAL(magnifierZoomIn_Signal()), this, SLOT(zoomInMagnifier()));
    connect(magniferControlViewWidget, SIGNAL(magnifierZoomOut_Signal()), this, SLOT(zoomOutMagnifier()));
    connect(magniferControlViewWidget, SIGNAL(magnifierDrawingModeChange_Signal(int)), this, SLOT(changeMagnifierMode(int)));
    connect(magniferControlViewWidget, SIGNAL(magnifierResized_Signal(qreal)), this, SLOT(resizedMagnifier(qreal)));

    setModified(true);
}

void WBGraphicsScene::moveMagnifier()
{
   if (magniferControlViewWidget)
   {
       QPoint magnifierPos = QPoint(magniferControlViewWidget->pos().x() + magniferControlViewWidget->size().width() / 2, magniferControlViewWidget->pos().y() + magniferControlViewWidget->size().height() / 2 );
       moveMagnifier(magnifierPos, true);
       setModified(true);
   }
}

void WBGraphicsScene::moveMagnifier(QPoint newPos, bool forceGrab)
{
    QWidget *cContainer = (QWidget*)(WBApplication::boardController->controlContainer());
    QGraphicsView *cView = (QGraphicsView*)WBApplication::boardController->controlView();
    QGraphicsView *dView = (QGraphicsView*)WBApplication::boardController->displayView();

    QPoint dvZeroPoint = dView->mapToGlobal(QPoint(0,0));

    int cvW = cView->width();
    int dvW = dView->width();
    qreal wCoeff = (qreal)dvW / (qreal)cvW;

    int cvH = cView->height();
    int dvH = dView->height();
    qreal hCoeff = (qreal)dvH / (qreal)cvH;

    QPoint globalPoint = cContainer->mapToGlobal(newPos);
    QPoint cvPoint = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint( cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magniferControlViewWidget->grabNMove(globalPoint, globalPoint, forceGrab, false);
    magniferDisplayViewWidget->grabNMove(globalPoint, dvPoint, forceGrab, true);

    setModified(true);
}

void WBGraphicsScene::closeMagnifier()
{
    DisposeMagnifierQWidgets();
    setModified(true);
}

void WBGraphicsScene::zoomInMagnifier()
{
    if(magniferControlViewWidget->params.zoom < 8)
    {
        magniferControlViewWidget->setZoom(magniferControlViewWidget->params.zoom + 0.5);
        magniferDisplayViewWidget->setZoom(magniferDisplayViewWidget->params.zoom + 0.5);
    }
}

void WBGraphicsScene::zoomOutMagnifier()
{
    if(magniferControlViewWidget->params.zoom > 1)
    {
        magniferControlViewWidget->setZoom(magniferControlViewWidget->params.zoom - 0.5);
        magniferDisplayViewWidget->setZoom(magniferDisplayViewWidget->params.zoom - 0.5);
        setModified(true);
    }
}

void WBGraphicsScene::changeMagnifierMode(int mode)
{
    if(magniferControlViewWidget)
        magniferControlViewWidget->setDrawingMode(mode);
}

void WBGraphicsScene::resizedMagnifier(qreal newPercent)
{
    if(newPercent > 18 && newPercent < 50)
    {
        magniferControlViewWidget->setSize(newPercent);
        magniferControlViewWidget->grabPoint();
        magniferDisplayViewWidget->setSize(newPercent);
        magniferDisplayViewWidget->grabPoint();
        setModified(true);
    }
}

void WBGraphicsScene::addCompass(QPointF center)
{
    WBGraphicsCompass* compass = new WBGraphicsCompass(); // mem : owned and destroyed by the scene
    mTools << compass;
    addItem(compass);

    QRectF rect = compass->rect();
    compass->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(), rect.height());

    compass->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));

    compass->setVisible(true);
}

void WBGraphicsScene::addCache()
{
    WBGraphicsCache* cache = WBGraphicsCache::instance(this);
    if (!items().contains(cache)) {
        addItem(cache);

        cache->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));

        cache->setVisible(true);
        cache->setSelected(true);
        WBApplication::boardController->notifyCache(true);
        WBApplication::boardController->notifyPageChanged();
    }
}

void WBGraphicsScene::addMask(const QPointF &center)
{
    WBGraphicsCurtainItem* curtain = new WBGraphicsCurtainItem(); // mem : owned and destroyed by the scene
    mTools << curtain;

    addItem(curtain);

    QRectF rect = WBApplication::boardController->activeScene()->normalizedSceneRect();
    rect.setRect(center.x() - rect.width()/4, center.y() - rect.height()/4, rect.width()/2 , rect.height()/2);
    curtain->setRect(rect);
    curtain->setVisible(true);
    curtain->setSelected(true);
}

void WBGraphicsScene::setRenderingQuality(WBItem::RenderingQuality pRenderingQuality)
{
    QListIterator<QGraphicsItem*> itItems(mFastAccessItems);

    while (itItems.hasNext())
    {
        QGraphicsItem *gItem =  itItems.next();

        WBItem *ubItem = dynamic_cast<WBItem*>(gItem);

        if (ubItem)
        {
            ubItem->setRenderingQuality(pRenderingQuality);
        }
    }
}

QList<QUrl> WBGraphicsScene::relativeDependencies() const
{
    QList<QUrl> relativePathes;

    QListIterator<QGraphicsItem*> itItems(mFastAccessItems);

    while (itItems.hasNext())
    {
        QGraphicsItem* item = itItems.next();

        WBGraphicsVideoItem *videoItem = qgraphicsitem_cast<WBGraphicsVideoItem*> (item);
        if (videoItem){
            QString completeFileName = QFileInfo(videoItem->mediaFileUrl().toLocalFile()).fileName();
            QString path = WBPersistenceManager::videoDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        WBGraphicsAudioItem *audioItem = qgraphicsitem_cast<WBGraphicsAudioItem*> (item);
        if (audioItem){
            QString completeFileName = QFileInfo(audioItem->mediaFileUrl().toLocalFile()).fileName();
            QString path = WBPersistenceManager::audioDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        WBGraphicsWidgetItem* widget = qgraphicsitem_cast<WBGraphicsWidgetItem*>(item);
        if(widget){
            QString widgetPath = WBPersistenceManager::widgetDirectory + "/" + widget->uuid().toString() + ".wgt";
            QString screenshotPath = WBPersistenceManager::widgetDirectory + "/" + widget->uuid().toString().remove("{").remove("}") + ".png";
            relativePathes << QUrl(widgetPath);
            relativePathes << QUrl(screenshotPath);
            continue;
        }

        WBGraphicsPixmapItem* pixmapItem = qgraphicsitem_cast<WBGraphicsPixmapItem*>(item);
        if(pixmapItem){
            relativePathes << QUrl(WBPersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png");
            continue;
        }

        WBGraphicsSvgItem* svgItem = qgraphicsitem_cast<WBGraphicsSvgItem*>(item);
        if(svgItem){
            relativePathes << QUrl(WBPersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg");
            continue;
        }
    }

    return relativePathes;
}

QSize WBGraphicsScene::nominalSize()
{
    if (mDocument && !mNominalSize.isValid())
    {
        mNominalSize = mDocument->defaultDocumentSize();
    }

    return mNominalSize;
}

/**
 * @brief Return the scene's boundary size, including any background item
 *
 * If no background item is present, this returns nominalSize()
 */
QSize WBGraphicsScene::sceneSize()
{
    WBGraphicsPDFItem *pdfItem = qgraphicsitem_cast<WBGraphicsPDFItem*>(backgroundObject());

    if (pdfItem) {
        QRectF targetRect = pdfItem->sceneBoundingRect();
        return targetRect.size().toSize();
    }

    else
        return nominalSize();
}

void WBGraphicsScene::setNominalSize(const QSize& pSize)
{
    if (nominalSize() != pSize)
    {
        mNominalSize = pSize;

        if(mDocument)
            mDocument->setDefaultDocumentSize(pSize);
    }
}

void WBGraphicsScene::setNominalSize(int pWidth, int pHeight)
{
     setNominalSize(QSize(pWidth, pHeight));
}

void WBGraphicsScene::setSelectedZLevel(QGraphicsItem * item)
{
    item->setZValue(mZLayerController->generateZLevel(itemLayerType::SelectedItem));
}

void WBGraphicsScene::setOwnZlevel(QGraphicsItem *item)
{
    item->setZValue(item->data(WBGraphicsItemData::ItemOwnZValue).toReal());
}

QUuid WBGraphicsScene::getPersonalUuid(QGraphicsItem *item)
{
    QString idCandidate = item->data(WBGraphicsItemData::ItemUuid).toString();
    return idCandidate == QUuid().toString() ? QUuid() : QUuid(idCandidate);
}

qreal WBGraphicsScene::changeZLevelTo(QGraphicsItem *item, WBZLayerController::moveDestination dest, bool addUndo)
{
    qreal previousZVal = item->data(WBGraphicsItemData::ItemOwnZValue).toReal();
    qreal res = mZLayerController->changeZLevelTo(item, dest);

    if(addUndo){
        WBGraphicsItemZLevelUndoCommand* uc = new WBGraphicsItemZLevelUndoCommand(this, item, previousZVal, dest);
        WBApplication::undoStack->push(uc);
    }

    return res;
}

QGraphicsItem* WBGraphicsScene::rootItem(QGraphicsItem* item) const
{
    QGraphicsItem* root = item;

    while (root->parentItem())
    {
        root = root->parentItem();
    }

    return root;
}

void WBGraphicsScene::drawItems (QPainter * painter, int numItems,
        QGraphicsItem * items[], const QStyleOptionGraphicsItem options[], QWidget * widget)
{
    if (mRenderingContext == NonScreen || mRenderingContext == PdfExport)
    {
        int count = 0;

        QGraphicsItem** itemsFiltered = new QGraphicsItem*[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++)
        {
            if (!mTools.contains(rootItem(items[i])))
            {
                bool isPdfItem =  qgraphicsitem_cast<WBGraphicsPDFItem*> (items[i]) != NULL;
                if(!isPdfItem || mRenderingContext == NonScreen)
                {
                    itemsFiltered[count] = items[i];
                    optionsFiltered[count] = options[i];
                    count++;
                }
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;

    }
    else if (mRenderingContext == Podcast)
    {
        int count = 0;

        QGraphicsItem** itemsFiltered = new QGraphicsItem*[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++)
        {
            bool ok;
            int itemLayerType = items[i]->data(WBGraphicsItemData::ItemLayerType).toInt(&ok);
            if (ok && (itemLayerType >= WBItemLayerType::FixedBackground && itemLayerType <= WBItemLayerType::Tool))
            {
                itemsFiltered[count] = items[i];
                optionsFiltered[count] = options[i];
                count++;
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;

    }
    else
    {
        QGraphicsScene::drawItems(painter, numItems, items, options, widget);
    }
}

void WBGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (mIsDesktopMode)
    {
        QGraphicsScene::drawBackground (painter, rect);
        return;
    }
    bool darkBackground = isDarkBackground ();

    if (darkBackground)
    {
      painter->fillRect (rect, QBrush (QColor (Qt::black)));
    }
    else
    {
      painter->fillRect (rect, QBrush (QColor (Qt::white)));
    }

    if (mZoomFactor > 0.5)
    {
        QColor bgCrossColor;

        if (darkBackground)
            bgCrossColor = QColor(WBSettings::settings()->boardCrossColorDarkBackground->get().toString());
        else
            bgCrossColor = QColor(WBSettings::settings()->boardCrossColorLightBackground->get().toString());
        if (mZoomFactor < 0.7)
        {
            int alpha = 255 * mZoomFactor / 2;
            bgCrossColor.setAlpha (alpha); // fade the crossing on small zooms
        }

        painter->setPen (bgCrossColor);

        if (mPageBackground == WBPageBackground::crossed)
        {
            qreal firstY = ((int) (rect.y () / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y () + rect.height (); yPos += backgroundGridSize())
            {
                painter->drawLine (rect.x (), yPos, rect.x () + rect.width (), yPos);
            }

            qreal firstX = ((int) (rect.x () / backgroundGridSize())) * backgroundGridSize();

            for (qreal xPos = firstX; xPos < rect.x () + rect.width (); xPos += backgroundGridSize())
            {
                painter->drawLine (xPos, rect.y (), xPos, rect.y () + rect.height ());
            }
        }

        else if (mPageBackground == WBPageBackground::ruled)
        {
            qreal firstY = ((int) (rect.y () / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y () + rect.height (); yPos += backgroundGridSize())
            {
                painter->drawLine (rect.x (), yPos, rect.x () + rect.width (), yPos);
            }
        }
    }
}

void WBGraphicsScene::keyReleaseEvent(QKeyEvent * keyEvent)
{

    QList<QGraphicsItem*> si = selectedItems();

    if(keyEvent->matches(QKeySequence::SelectAll)){
        QListIterator<QGraphicsItem*> itItems(this->mFastAccessItems);

        while (itItems.hasNext())
            itItems.next()->setSelected(true);

        keyEvent->accept();
        return;
    }

    if ((si.size() > 0) && (keyEvent->isAccepted()))
    {
#ifdef Q_OS_MAC
        if (keyEvent->key() == Qt::Key_Backspace)
#else
        if (keyEvent->matches(QKeySequence::Delete))
#endif
        {
            QVector<WBGraphicsItem*> ubItemsToRemove;
            QVector<QGraphicsItem*> itemToRemove;

            bool bRemoveOk = true;

            foreach(QGraphicsItem* item, si)
            {
                switch (item->type())
                {
                case WBGraphicsWidgetItem::Type:
                    {
                        WBGraphicsW3CWidgetItem *wc3_widget = dynamic_cast<WBGraphicsW3CWidgetItem*>(item);
                        if (0 != wc3_widget)
                        if (!wc3_widget->hasFocus())
                            ubItemsToRemove << wc3_widget;
                        break;
                    }
                case WBGraphicsTextItem::Type:
                    {
                        WBGraphicsTextItem *text_item = dynamic_cast<WBGraphicsTextItem*>(item);
                        if (0 != text_item){
                            if (!text_item->hasFocus())
                                ubItemsToRemove << text_item;
                            else
                                bRemoveOk = false;
                        }
                        break;
                    }

                case WBGraphicsGroupContainerItem::Type:
                {
                    WBGraphicsGroupContainerItem* group_item = dynamic_cast<WBGraphicsGroupContainerItem*>(item);
                    if(NULL != group_item){
                        if(!hasTextItemWithFocus(group_item))
                            ubItemsToRemove << group_item;
                        else
                            bRemoveOk = false;
                    }
                    break;
                }

                default:
                    {
                        WBGraphicsItem *ubgi = dynamic_cast<WBGraphicsItem*>(item);
                        if (0 != ubgi)
                            ubItemsToRemove << ubgi;
                        else
                            itemToRemove << item;
                    }
                }
            }

            if(bRemoveOk){
                foreach(WBGraphicsItem* pUBItem, ubItemsToRemove){
                    pUBItem->remove();
                }
                foreach(QGraphicsItem* pItem, itemToRemove){
                    WBCoreGraphicsScene::removeItem(pItem);
                }
            }
        }

        keyEvent->accept();
    }

    QGraphicsScene::keyReleaseEvent(keyEvent);
}

bool WBGraphicsScene::hasTextItemWithFocus(WBGraphicsGroupContainerItem *item){
    bool bHasFocus = false;

    foreach(QGraphicsItem* pItem, item->childItems()){
        WBGraphicsTextItem *text_item = dynamic_cast<WBGraphicsTextItem*>(pItem);
        if (NULL != text_item){
            if(text_item->hasFocus()){
                bHasFocus = true;
                break;
            }
        }
    }

    return bHasFocus;
}


void WBGraphicsScene::simplifyCurrentStroke()
{
    if (!mCurrentStroke)
        return;

    WBGraphicsStroke* simplerStroke = mCurrentStroke->simplify();
    if (!simplerStroke)
        return;

    foreach(WBGraphicsPolygonItem* poly, mCurrentStroke->polygons()){
        mPreviousPolygonItems.removeAll(poly);
        removeItem(poly);
    }

    mCurrentStroke = simplerStroke;

    foreach(WBGraphicsPolygonItem* poly, mCurrentStroke->polygons()) {
        addItem(poly);
        mPreviousPolygonItems.append(poly);
    }

}

void WBGraphicsScene::setDocumentUpdated()
{
    if (document())
        document()->setMetaData(WBSettings::documentUpdatedAt
                , WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
}

void WBGraphicsScene::createEraiser()
{
    if (WBSettings::settings()->showEraserPreviewCircle->get().toBool()) {
        mEraser = new QGraphicsEllipseItem(); // mem : owned and destroyed by the scene
        mEraser->setRect(QRect(0, 0, 0, 0));
        mEraser->setVisible(false);

        updateEraserColor();

        mEraser->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));
        mEraser->setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser)); //Necessary to set if we want z value to be assigned correctly

        mTools << mEraser;
        addItem(mEraser);
    }
}

void WBGraphicsScene::createPointer()
{
    mPointer = new QGraphicsEllipseItem();  // mem : owned and destroyed by the scene
    mPointer->setRect(QRect(0, 0, 20, 20));
    mPointer->setVisible(false);

    mPointer->setPen(Qt::NoPen);
    mPointer->setBrush(QBrush(QColor(255, 0, 0, 186)));

    mPointer->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Tool));
    mPointer->setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Pointer)); //Necessary to set if we want z value to be assigned correctly

    mTools << mPointer;
    addItem(mPointer);
}

void WBGraphicsScene::createMarkerCircle()
{
    if (WBSettings::settings()->showMarkerPreviewCircle->get().toBool()) {
        mMarkerCircle = new QGraphicsEllipseItem();

        mMarkerCircle->setRect(QRect(0, 0, 0, 0));
        mMarkerCircle->setVisible(false);

        mMarkerCircle->setPen(Qt::DotLine);
        updateMarkerCircleColor();

        mMarkerCircle->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));
        mMarkerCircle->setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser));

        mTools << mMarkerCircle;
        addItem(mMarkerCircle);
    }
}

void WBGraphicsScene::createPenCircle()
{
    if (WBSettings::settings()->showPenPreviewCircle->get().toBool()) {
        mPenCircle = new QGraphicsEllipseItem();

        mPenCircle->setRect(QRect(0, 0, 0, 0));
        mPenCircle->setVisible(false);

        mPenCircle->setPen(Qt::DotLine);
        updatePenCircleColor();

        mPenCircle->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));
        mPenCircle->setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser));

        mTools << mPenCircle;
        addItem(mPenCircle);
    }
}

void WBGraphicsScene::updateEraserColor()
{
    if (!mEraser)
        return;

    if (mDarkBackground) {
        mEraser->setBrush(WBSettings::eraserBrushDarkBackground);
        mEraser->setPen(WBSettings::eraserPenDarkBackground);
    }

    else {
        mEraser->setBrush(WBSettings::eraserBrushLightBackground);
        mEraser->setPen(WBSettings::eraserPenLightBackground);
    }
}

void WBGraphicsScene::updateMarkerCircleColor()
{
    if (!mMarkerCircle)
        return;

    QPen mcPen = mMarkerCircle->pen();

    if (mDarkBackground) {
        mcPen.setColor(WBSettings::markerCirclePenColorDarkBackground);
        mMarkerCircle->setBrush(WBSettings::markerCircleBrushColorDarkBackground);
    }

    else {
        mcPen.setColor(WBSettings::markerCirclePenColorLightBackground);
        mMarkerCircle->setBrush(WBSettings::markerCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mMarkerCircle->setPen(mcPen);
}

void WBGraphicsScene::updatePenCircleColor()
{
    if (!mPenCircle)
        return;

    QPen mcPen = mPenCircle->pen();

    if (mDarkBackground) {
        mcPen.setColor(WBSettings::penCirclePenColorDarkBackground);
        mPenCircle->setBrush(WBSettings::penCircleBrushColorDarkBackground);
    }

    else {
        mcPen.setColor(WBSettings::penCirclePenColorLightBackground);
        mPenCircle->setBrush(WBSettings::penCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mPenCircle->setPen(mcPen);
}

void WBGraphicsScene::setToolCursor(int tool)
{
    if (tool == (int)WBStylusTool::Selector ||
            tool == (int)WBStylusTool::Text ||
            tool == (int)WBStylusTool::Play) {
        deselectAllItems();
        hideMarkerCircle();
        hidePenCircle();
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty()){
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

}

void WBGraphicsScene::initStroke()
{
    mCurrentStroke = new WBGraphicsStroke(this);
}
