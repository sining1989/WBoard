#include "WBBoardController.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBPersistenceManager.h"
#include "core/WBApplicationController.h"
#include "core/WBDocumentManager.h"
#include "core/WBMimeData.h"
#include "core/WBDownloadManager.h"

#include "network/WBHttpGet.h"

#include "gui/WBMessageWindow.h"
#include "gui/WBResources.h"
#include "gui/WBToolbarButtonGroup.h"
#include "gui/WBMainWindow.h"
#include "gui/WBToolWidget.h"
#include "gui/WBKeyboardPalette.h"
#include "gui/WBMagnifer.h"
#include "gui/WBDockPaletteWidget.h"

#include "domain/WBGraphicsPixmapItem.h"
#include "domain/WBGraphicsItemUndoCommand.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsMediaItem.h"
#include "domain/WBGraphicsPDFItem.h"
#include "domain/WBGraphicsTextItem.h"
#include "domain/WBPageSizeUndoCommand.h"
#include "domain/WBGraphicsGroupContainerItem.h"
#include "domain/WBGraphicsStrokesGroup.h"
#include "domain/WBItem.h"
#include "board/WBFeaturesController.h"

#include "gui/WBFeaturesWidget.h"

#include "tools/WBToolsManager.h"

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentController.h"

#include "board/WBDrawingController.h"
#include "board/WBBoardView.h"

#include "podcast/WBPodcastController.h"

#include "adaptors/WBMetadataDcSubsetAdaptor.h"
#include "adaptors/WBSvgSubsetAdaptor.h"

#include "WBBoardPaletteManager.h"

#include "core/WBSettings.h"

#include "core/memcheck.h"

WBBoardController::WBBoardController(WBMainWindow* mainWindow)
    : WBDocumentContainer(mainWindow->centralWidget())
    , mMainWindow(mainWindow)
    , mActiveScene(0)
    , mActiveSceneIndex(-1)
    , mPaletteManager(0)
    , mSoftwareUpdateDialog(0)
    , mMessageWindow(0)
    , mControlView(0)
    , mDisplayView(0)
    , mControlContainer(0)
    , mControlLayout(0)
    , mZoomFactor(1.0)
    , mIsClosing(false)
    , mSystemScaleFactor(1.0)
    , mCleanupDone(false)
    , mCacheWidgetIsEnabled(false)
    , mDeletingSceneIndex(-1)
    , mMovingSceneIndex(-1)
    , mActionGroupText(tr("Group"))
    , mActionUngroupText(tr("Ungroup"))
    , mAutosaveTimer(0)
{
    mZoomFactor = WBSettings::settings()->boardZoomFactor->get().toDouble();

    int penColorIndex = WBSettings::settings()->penColorIndex();
    int markerColorIndex = WBSettings::settings()->markerColorIndex();

    mPenColorOnDarkBackground = WBSettings::settings()->penColors(true).at(penColorIndex);
    mPenColorOnLightBackground = WBSettings::settings()->penColors(false).at(penColorIndex);
    mMarkerColorOnDarkBackground = WBSettings::settings()->markerColors(true).at(markerColorIndex);
    mMarkerColorOnLightBackground = WBSettings::settings()->markerColors(false).at(markerColorIndex);

}


void WBBoardController::init()
{
    setupViews();
    setupToolbar();

    connect(WBApplication::undoStack, SIGNAL(canUndoChanged(bool))
            , this, SLOT(undoRedoStateChange(bool)));

    connect(WBApplication::undoStack, SIGNAL(canRedoChanged (bool))
            , this, SLOT(undoRedoStateChange(bool)));

    connect(WBDrawingController::drawingController(), SIGNAL(stylusToolChanged(int))
            , this, SLOT(setToolCursor(int)));

    connect(WBDrawingController::drawingController(), SIGNAL(stylusToolChanged(int))
            , this, SLOT(stylusToolChanged(int)));

    connect(WBApplication::app(), SIGNAL(lastWindowClosed())
            , this, SLOT(lastWindowClosed()));

    connect(WBDownloadManager::downloadManager(), SIGNAL(downloadModalFinished()), this, SLOT(onDownloadModalFinished()));
    connect(WBDownloadManager::downloadManager(), SIGNAL(addDownloadedFileToBoard(bool,QUrl,QUrl,QString,QByteArray,QPointF,QSize,bool)), this, SLOT(downloadFinished(bool,QUrl,QUrl,QString,QByteArray,QPointF,QSize,bool)));

    WBDocumentProxy* doc = WBPersistenceManager::persistenceManager()->createNewDocument();

    setActiveDocumentScene(doc);

    initBackgroundGridSize();

    undoRedoStateChange(true);

}


WBBoardController::~WBBoardController()
{
    delete mDisplayView;
}

/**
 * @brief Set the default background grid size to appear as roughly 1cm on screen
 */
void WBBoardController::initBackgroundGridSize()
{
    // Besides adjusting for DPI, we also need to scale the grid size by the ratio of the control view size
    // to document size. However the control view isn't available as soon as the boardController is created,
    // so we approximate this ratio as (document resolution) / (screen resolution).
    // Later on, this is calculated by `updateSystemScaleFactor` and stored in `mSystemScaleFactor`.

    QDesktopWidget* desktop = WBApplication::desktop();
    qreal dpi = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2.;

    //qDebug() << "dpi: " << dpi;

    // The display manager isn't initialized yet so we have to just assume the control view is on the main display
    qreal screenY = desktop->screenGeometry(mControlView).height();
    qreal documentY = mActiveScene->nominalSize().height();
    qreal resolutionRatio = documentY / screenY;

    //qDebug() << "resolution ratio: " << resolutionRatio;

    int gridSize = (resolutionRatio * 10. * dpi) / WBGeometryUtils::inchSize;

    WBSettings::settings()->crossSize = gridSize;
    WBSettings::settings()->defaultCrossSize = gridSize;
    mActiveScene->setBackgroundGridSize(gridSize);

    //qDebug() << "grid size: " << gridSize;
}

int WBBoardController::currentPage()
{
    return mActiveSceneIndex + 1;
}

void WBBoardController::setupViews()
{
    mControlContainer = new QWidget(mMainWindow->centralWidget());

    mControlLayout = new QHBoxLayout(mControlContainer);
    mControlLayout->setContentsMargins(0, 0, 0, 0);

    mControlView = new WBBoardView(this, mControlContainer, true, false);
    mControlView->setObjectName(CONTROLVIEW_OBJ_NAME);
    mControlView->setInteractive(true);
    mControlView->setMouseTracking(true);

    mControlView->grabGesture(Qt::SwipeGesture);

    mControlView->setTransformationAnchor(QGraphicsView::NoAnchor);

    mControlLayout->addWidget(mControlView);
    mControlContainer->setObjectName("ubBoardControlContainer");
    mMainWindow->addBoardWidget(mControlContainer);

    connect(mControlView, SIGNAL(resized(QResizeEvent*)), this, SLOT(boardViewResized(QResizeEvent*)));

    mDisplayView = new WBBoardView(this, WBItemLayerType::FixedBackground, WBItemLayerType::Tool, 0);
    mDisplayView->setInteractive(false);
    mDisplayView->setTransformationAnchor(QGraphicsView::NoAnchor);

    mPaletteManager = new WBBoardPaletteManager(mControlContainer, this);

    mMessageWindow = new WBMessageWindow(mControlContainer);
    mMessageWindow->hide();

    connect(this, SIGNAL(activeSceneChanged()), mPaletteManager, SLOT(activeSceneChanged()));
}


void WBBoardController::setupLayout()
{
    if(mPaletteManager)
        mPaletteManager->setupLayout();
}


void WBBoardController::setBoxing(QRect displayRect)
{
    if (displayRect.isNull())
    {
        mControlLayout->setContentsMargins(0, 0, 0, 0);
        return;
    }

    qreal controlWidth = (qreal)mMainWindow->centralWidget()->width();
    qreal controlHeight = (qreal)mMainWindow->centralWidget()->height();
    qreal displayWidth = (qreal)displayRect.width();
    qreal displayHeight = (qreal)displayRect.height();

    qreal displayRatio = displayWidth / displayHeight;
    qreal controlRatio = controlWidth / controlHeight;

    if (displayRatio < controlRatio)
    {
        int boxWidth = (controlWidth - (displayWidth * (controlHeight / displayHeight))) / 2;
        mControlLayout->setContentsMargins(boxWidth, 0, boxWidth, 0);
    }
    else if (displayRatio > controlRatio)
    {
        int boxHeight = (controlHeight - (displayHeight * (controlWidth / displayWidth))) / 2;
        mControlLayout->setContentsMargins(0, boxHeight, 0, boxHeight);
    }
    else
    {
        mControlLayout->setContentsMargins(0, 0, 0, 0);
    }
}

QSize WBBoardController::displayViewport()
{
    return mDisplayView->geometry().size();
}

QSize WBBoardController::controlViewport()
{
    return mControlView->geometry().size();
}

QRectF WBBoardController::controlGeometry()
{
    return mControlView->geometry();
}

void WBBoardController::setupToolbar()
{
    WBSettings *settings = WBSettings::settings();
	//zhusizhi 20210625
    //// Setup color choice widget
    //QList<QAction *> colorActions;
    //colorActions.append(mMainWindow->actionColor0);
    //colorActions.append(mMainWindow->actionColor1);
    //colorActions.append(mMainWindow->actionColor2);
    //colorActions.append(mMainWindow->actionColor3);
    //colorActions.append(mMainWindow->actionColor4);

    //WBToolbarButtonGroup *colorChoice =
    //        new WBToolbarButtonGroup(mMainWindow->boardToolBar, colorActions);

    ////mMainWindow->boardToolBar->insertWidget(mMainWindow->actionBackgrounds, colorChoice);

    //connect(settings->appToolBarDisplayText, SIGNAL(changed(QVariant)), colorChoice, SLOT(displayText(QVariant)));
    //connect(colorChoice, SIGNAL(activated(int)), this, SLOT(setColorIndex(int)));
    //connect(WBDrawingController::drawingController(), SIGNAL(colorIndexChanged(int)), colorChoice, SLOT(setCurrentIndex(int)));
    //connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), colorChoice, SLOT(colorPaletteChanged()));
    //connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), this, SLOT(colorPaletteChanged()));

    //colorChoice->displayText(QVariant(settings->appToolBarDisplayText->get().toBool()));
    //colorChoice->colorPaletteChanged();

    //// Setup line width choice widget
    //QList<QAction *> lineWidthActions;
    //lineWidthActions.append(mMainWindow->actionLineSmall);
    //lineWidthActions.append(mMainWindow->actionLineMedium);
    //lineWidthActions.append(mMainWindow->actionLineLarge);

    //WBToolbarButtonGroup *lineWidthChoice =
    //        new WBToolbarButtonGroup(mMainWindow->boardToolBar, lineWidthActions);

    //connect(settings->appToolBarDisplayText, SIGNAL(changed(QVariant)), lineWidthChoice, SLOT(displayText(QVariant)));

    //connect(lineWidthChoice, SIGNAL(activated(int))
    //        , WBDrawingController::drawingController(), SLOT(setLineWidthIndex(int)));

    //connect(WBDrawingController::drawingController(), SIGNAL(lineWidthIndexChanged(int))
    //        , lineWidthChoice, SLOT(setCurrentIndex(int)));

    //lineWidthChoice->displayText(QVariant(settings->appToolBarDisplayText->get().toBool()));

    ////mMainWindow->boardToolBar->insertWidget(mMainWindow->actionBackgrounds, lineWidthChoice);

    //// Setup eraser width choice widget

    //QList<QAction *> eraserWidthActions;
    //eraserWidthActions.append(mMainWindow->actionEraserSmall);
    //eraserWidthActions.append(mMainWindow->actionEraserMedium);
    //eraserWidthActions.append(mMainWindow->actionEraserLarge);

    //WBToolbarButtonGroup *eraserWidthChoice =
    //        new WBToolbarButtonGroup(mMainWindow->boardToolBar, eraserWidthActions);

    //mMainWindow->boardToolBar->insertWidget(mMainWindow->actionBackgrounds, eraserWidthChoice);

    //connect(settings->appToolBarDisplayText, SIGNAL(changed(QVariant)), eraserWidthChoice, SLOT(displayText(QVariant)));
    //connect(eraserWidthChoice, SIGNAL(activated(int)), WBDrawingController::drawingController(), SLOT(setEraserWidthIndex(int)));

    //eraserWidthChoice->displayText(QVariant(settings->appToolBarDisplayText->get().toBool()));
    //eraserWidthChoice->setCurrentIndex(settings->eraserWidthIndex());

    //mMainWindow->boardToolBar->insertSeparator(mMainWindow->actionBackgrounds);


    WBApplication::app()->insertSpaceToToolbarBeforeAction(mMainWindow->boardToolBar, mMainWindow->actionBoard);

    WBApplication::app()->decorateActionMenu(mMainWindow->actionMenu);

    mMainWindow->actionBoard->setVisible(false);

    mMainWindow->webToolBar->hide();
    mMainWindow->documentToolBar->hide();

    connectToolbar();
    initToolbarTexts();

    WBApplication::app()->toolBarDisplayTextChanged(QVariant(settings->appToolBarDisplayText->get().toBool()));
}


void WBBoardController::setToolCursor(int tool)
{
    if (mActiveScene)
        mActiveScene->setToolCursor(tool);

    mControlView->setToolCursor(tool);
}


void WBBoardController::connectToolbar()
{
    connect(mMainWindow->actionAdd, SIGNAL(triggered()), this, SLOT(addItem()));
    connect(mMainWindow->actionNewPage, SIGNAL(triggered()), this, SLOT(addScene()));
    connect(mMainWindow->actionDuplicatePage, SIGNAL(triggered()), this, SLOT(duplicateScene()));

    connect(mMainWindow->actionClearPage, SIGNAL(triggered()), this, SLOT(clearScene()));
    connect(mMainWindow->actionEraseItems, SIGNAL(triggered()), this, SLOT(clearSceneItems()));
    connect(mMainWindow->actionEraseAnnotations, SIGNAL(triggered()), this, SLOT(clearSceneAnnotation()));
    connect(mMainWindow->actionEraseBackground,SIGNAL(triggered()),this,SLOT(clearSceneBackground()));

    connect(mMainWindow->actionUndo, SIGNAL(triggered()), WBApplication::undoStack, SLOT(undo()));
    connect(mMainWindow->actionRedo, SIGNAL(triggered()), WBApplication::undoStack, SLOT(redo()));
    connect(mMainWindow->actionRedo, SIGNAL(triggered()), this, SLOT(startScript()));
    connect(mMainWindow->actionBack, SIGNAL( triggered()), this, SLOT(previousScene()));
    connect(mMainWindow->actionForward, SIGNAL(triggered()), this, SLOT(nextScene()));
    connect(mMainWindow->actionSleep, SIGNAL(triggered()), this, SLOT(stopScript()));
    connect(mMainWindow->actionSleep, SIGNAL(triggered()), this, SLOT(blackout()));
    connect(mMainWindow->actionVirtualKeyboard, SIGNAL(triggered(bool)), this, SLOT(showKeyboard(bool)));
    connect(mMainWindow->actionImportPage, SIGNAL(triggered()), this, SLOT(importPage()));
}

void WBBoardController::startScript()
{
    freezeW3CWidgets(false);
}

void WBBoardController::stopScript()
{
    freezeW3CWidgets(true);
}

void WBBoardController::saveData(SaveFlags fls)
{
    bool verbose = fls | sf_showProgress;
    if (verbose) {
        WBApplication::showMessage(tr("Saving document..."));
    }
    if (mActiveScene && mActiveScene->isModified()) {
        persistCurrentScene(true);
    }
    if (verbose) {
        WBApplication::showMessage(tr("Document has just been saved..."));
    }
}

void WBBoardController::initToolbarTexts()
{
    QList<QAction*> allToolbarActions;

    allToolbarActions << mMainWindow->boardToolBar->actions();
    allToolbarActions << mMainWindow->webToolBar->actions();
    allToolbarActions << mMainWindow->documentToolBar->actions();

    foreach(QAction* action, allToolbarActions)
    {
        QString nominalText = action->text();
        QString shortText = truncate(nominalText, 48);
        QPair<QString, QString> texts(nominalText, shortText);

        mActionTexts.insert(action, texts);
    }
}


void WBBoardController::setToolbarTexts()
{
    bool highResolution = mMainWindow->width() > 1024;
    QSize iconSize;

    if (highResolution)
        iconSize = QSize(48, 32);
    else
        iconSize = QSize(32, 32);

    mMainWindow->boardToolBar->setIconSize(iconSize);
    mMainWindow->webToolBar->setIconSize(iconSize);
    mMainWindow->documentToolBar->setIconSize(iconSize);

    foreach(QAction* action, mActionTexts.keys())
    {
        QPair<QString, QString> texts = mActionTexts.value(action);

        if (highResolution)
            action->setText(texts.first);
        else
        {
            action->setText(texts.second);
        }

        action->setToolTip(texts.first);
    }
}


QString WBBoardController::truncate(QString text, int maxWidth)
{
    QFontMetricsF fontMetrics(mMainWindow->font());
    return fontMetrics.elidedText(text, Qt::ElideRight, maxWidth);
}


void WBBoardController::stylusToolDoubleClicked(int tool)
{
    if (tool == WBStylusTool::ZoomIn || tool == WBStylusTool::ZoomOut)
    {
        zoomRestore();
    }
    else if (tool == WBStylusTool::Hand)
    {
        centerRestore();
        mActiveScene->setLastCenter(QPointF(0,0));
    }
}



void WBBoardController::addScene()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    persistViewPositionOnCurrentScene();
    persistCurrentScene(false,true);

    WBDocumentContainer::addPage(mActiveSceneIndex + 1);

    selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    setActiveDocumentScene(mActiveSceneIndex + 1);
    QApplication::restoreOverrideCursor();
}

void WBBoardController::addScene(WBGraphicsScene* scene, bool replaceActiveIfEmpty)
{
    if (scene)
    {
        WBGraphicsScene* clone = scene->sceneDeepCopy();

        if (scene->document() && (scene->document() != selectedDocument()))
        {
            foreach(QUrl relativeFile, scene->relativeDependencies())
            {
                QString source = scene->document()->persistencePath() + "/" + relativeFile.path();
                QString destination = selectedDocument()->persistencePath() + "/" + relativeFile.path();

                WBFileSystemUtils::copy(source, destination, true);
            }
        }

        if (replaceActiveIfEmpty && mActiveScene->isEmpty())
        {
            WBPersistenceManager::persistenceManager()->insertDocumentSceneAt(selectedDocument(), clone, mActiveSceneIndex);
            emit addThumbnailRequired(this, mActiveSceneIndex);
            setActiveDocumentScene(mActiveSceneIndex);
            deleteScene(mActiveSceneIndex + 1);
        }
        else
        {
            persistCurrentScene(false,true);
            WBPersistenceManager::persistenceManager()->insertDocumentSceneAt(selectedDocument(), clone, mActiveSceneIndex + 1);
            emit addThumbnailRequired(this, mActiveSceneIndex + 1);
            setActiveDocumentScene(mActiveSceneIndex + 1);
        }

        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}


void WBBoardController::addScene(WBDocumentProxy* proxy, int sceneIndex, bool replaceActiveIfEmpty)
{
    WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(proxy, sceneIndex);

    if (scene)
    {
        addScene(scene, replaceActiveIfEmpty);
    }
}

void WBBoardController::duplicateScene(int nIndex)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    persistCurrentScene(false,true);

    QList<int> scIndexes;
    scIndexes << nIndex;
    duplicatePages(scIndexes);
    insertThumbPage(nIndex);
    emit documentThumbnailsUpdated(this);
    emit addThumbnailRequired(this, nIndex + 1);
    selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    setActiveDocumentScene(nIndex + 1);
    QApplication::restoreOverrideCursor();
}

void WBBoardController::duplicateScene()
{
    if (WBApplication::applicationController->displayMode() != WBApplicationController::Board)
        return;
    duplicateScene(mActiveSceneIndex);
}

WBGraphicsItem *WBBoardController::duplicateItem(WBItem *item)
{
    if (!item)
        return NULL;

    WBGraphicsItem *retItem = NULL;

    mLastCreatedItem = NULL;

    QUrl sourceUrl;
    QByteArray pData;

    //common parameters for any item
    QPointF itemPos;
    QSizeF itemSize;

    QGraphicsItem *commonItem = dynamic_cast<QGraphicsItem*>(item);
    if (commonItem)
    {
        qreal shifting = WBSettings::settings()->objectFrameWidth;
        itemPos = commonItem->pos() + QPointF(shifting,shifting);
        itemSize = commonItem->boundingRect().size();
        commonItem->setSelected(false);

    }

    WBMimeType::Enum itemMimeType;

    QString srcFile = item->sourceUrl().toLocalFile();
    if (srcFile.isEmpty())
        srcFile = item->sourceUrl().toString();

    QString contentTypeHeader;
    if (!srcFile.isEmpty())
        contentTypeHeader = WBFileSystemUtils::mimeTypeFromFileName(srcFile);

    if(NULL != qgraphicsitem_cast<WBGraphicsGroupContainerItem*>(commonItem))
        itemMimeType = WBMimeType::Group;
    else
        itemMimeType = WBFileSystemUtils::mimeTypeFromString(contentTypeHeader);

    switch(static_cast<int>(itemMimeType))
    {
    case WBMimeType::AppleWidget:
    case WBMimeType::W3CWidget:
        {
            WBGraphicsWidgetItem *witem = dynamic_cast<WBGraphicsWidgetItem*>(item);
            if (witem)
            {
                sourceUrl = witem->getOwnFolder();
            }
        }break;

    case WBMimeType::Video:
    case WBMimeType::Audio:
        {
            WBGraphicsMediaItem *mitem = dynamic_cast<WBGraphicsMediaItem*>(item);
            if (mitem)
            {
                sourceUrl = mitem->mediaFileUrl();
                downloadURL(sourceUrl, srcFile, itemPos, QSize(itemSize.width(), itemSize.height()), false, false);
                return NULL; // async operation
            }
        }break;

    case WBMimeType::VectorImage:
        {
            WBGraphicsSvgItem *viitem = dynamic_cast<WBGraphicsSvgItem*>(item);
            if (viitem)
            {
                pData = viitem->fileData();
                sourceUrl = item->sourceUrl();
            }
        }break;

    case WBMimeType::RasterImage:
        {
            WBGraphicsPixmapItem *pixitem = dynamic_cast<WBGraphicsPixmapItem*>(item);
            if (pixitem)
            {
                 QBuffer buffer(&pData);
                 buffer.open(QIODevice::WriteOnly);
                 QString format = WBFileSystemUtils::extension(item->sourceUrl().toString(QUrl::DecodeReserved));
                 pixitem->pixmap().save(&buffer, format.toLatin1());
            }
        }break;

    case WBMimeType::Group:
    {
        WBGraphicsGroupContainerItem* groupItem = dynamic_cast<WBGraphicsGroupContainerItem*>(item);
        WBGraphicsGroupContainerItem* duplicatedGroup = NULL;

        QList<QGraphicsItem*> duplicatedItems;
        QList<QGraphicsItem*> children = groupItem->childItems();

        mActiveScene->setURStackEnable(false);
        foreach(QGraphicsItem* pIt, children){
            WBItem* pItem = dynamic_cast<WBItem*>(pIt);
            if(pItem)
            {
                QGraphicsItem * itemToGroup = dynamic_cast<QGraphicsItem *>(duplicateItem(pItem));
                if (itemToGroup)
                {
                    itemToGroup->setZValue(pIt->zValue());
                    itemToGroup->setData(WBGraphicsItemData::ItemOwnZValue, pIt->data(WBGraphicsItemData::ItemOwnZValue).toReal());
                    duplicatedItems.append(itemToGroup);
                }
            }
        }
        duplicatedGroup = mActiveScene->createGroup(duplicatedItems);
        duplicatedGroup->setTransform(groupItem->transform());
        groupItem->setSelected(false);

        retItem = dynamic_cast<WBGraphicsItem *>(duplicatedGroup);

        QGraphicsItem * itemToAdd = dynamic_cast<QGraphicsItem *>(retItem);
        if (itemToAdd)
        {
            mActiveScene->addItem(itemToAdd);
            itemToAdd->setSelected(true);
        }
        mActiveScene->setURStackEnable(true);
    }break;

    case WBMimeType::UNKNOWN:
        {
            QGraphicsItem *gitem = dynamic_cast<QGraphicsItem*>(item->deepCopy());
            if (gitem)
            {
                mActiveScene->addItem(gitem);

                gitem->setPos(itemPos);

                mLastCreatedItem = gitem;
                gitem->setSelected(true);
            }
            retItem = dynamic_cast<WBGraphicsItem *>(gitem);
        }break;
    }

    if (retItem)
    {
        QGraphicsItem *graphicsRetItem = dynamic_cast<QGraphicsItem *>(retItem);
        if (mActiveScene->isURStackIsEnabled()) { //should be deleted after scene own undo stack implemented
             WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(mActiveScene, 0, graphicsRetItem);
             WBApplication::undoStack->push(uc);
        }
        return retItem;
    }

    WBItem *createdItem = downloadFinished(true, sourceUrl, QUrl::fromLocalFile(srcFile), contentTypeHeader, pData, itemPos, QSize(itemSize.width(), itemSize.height()), false);
    if (createdItem)
    {
        createdItem->setSourceUrl(item->sourceUrl());
        item->copyItemParameters(createdItem);

        QGraphicsItem *createdGitem = dynamic_cast<QGraphicsItem*>(createdItem);
        if (createdGitem)
            createdGitem->setPos(itemPos);
        mLastCreatedItem = dynamic_cast<QGraphicsItem*>(createdItem);
        mLastCreatedItem->setSelected(true);

        retItem = dynamic_cast<WBGraphicsItem *>(createdItem);
    }

    return retItem;
}

void WBBoardController::deleteScene(int nIndex)
{
    if (selectedDocument()->pageCount()>=2)
    {
        mDeletingSceneIndex = nIndex;
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistCurrentScene();
        showMessage(tr("Deleting page %1").arg(nIndex+1), true);

        QList<int> scIndexes;
        scIndexes << nIndex;
        deletePages(scIndexes);
        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        if (nIndex >= pageCount())
            nIndex = pageCount()-1;
        setActiveDocumentScene(nIndex);
        showMessage(tr("Page %1 deleted").arg(nIndex+1));
        QApplication::restoreOverrideCursor();
        mDeletingSceneIndex = -1;
    }
}


void WBBoardController::clearScene()
{
    if (mActiveScene)
    {
        freezeW3CWidgets(true);
        mActiveScene->clearContent(WBGraphicsScene::clearItemsAndAnnotations);
        mActiveScene->setLastCenter(QPointF(0,0));
        mControlView->centerOn(mActiveScene->lastCenter());
        updateActionStates();
    }
}


void WBBoardController::clearSceneItems()
{
    if (mActiveScene)
    {
        freezeW3CWidgets(true);
        mActiveScene->clearContent(WBGraphicsScene::clearItems);
        updateActionStates();
    }
}


void WBBoardController::clearSceneAnnotation()
{
    if (mActiveScene)
    {
        mActiveScene->clearContent(WBGraphicsScene::clearAnnotations);
        updateActionStates();
    }
}

void WBBoardController::clearSceneBackground()
{
    if (mActiveScene)
    {
        mActiveScene->clearContent(WBGraphicsScene::clearBackground);
        updateActionStates();
    }
}

void WBBoardController::showDocumentsDialog()
{
    if (selectedDocument())
        persistCurrentScene();

    WBApplication::mainWindow->actionLibrary->setChecked(false);

}

void WBBoardController::libraryDialogClosed(int ret)
{
    Q_UNUSED(ret);

    mMainWindow->actionLibrary->setChecked(false);
}


void WBBoardController::blackout()
{
    WBApplication::applicationController->blackout();
}

void WBBoardController::showKeyboard(bool show)
{
    if(show)
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);

    if(WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool())
        WBPlatformUtils::showOSK(show);
    else
        mPaletteManager->showVirtualKeyboard(show);

}


void WBBoardController::zoomIn(QPointF scenePoint)
{
    if (mControlView->transform().m11() > WB_MAX_ZOOM)
    {
        qApp->beep();
        return;
    }
    zoom(mZoomFactor, scenePoint);
}


void WBBoardController::zoomOut(QPointF scenePoint)
{
    if ((mControlView->horizontalScrollBar()->maximum() == 0) && (mControlView->verticalScrollBar()->maximum() == 0))
    {
        // Do not zoom out if we reached the maximum
        qApp->beep();
        return;
    }

    qreal newZoomFactor = 1 / mZoomFactor;

    zoom(newZoomFactor, scenePoint);
}


void WBBoardController::zoomRestore()
{
    QTransform tr;

    tr.scale(mSystemScaleFactor, mSystemScaleFactor);
    mControlView->setTransform(tr);

    centerRestore();

    foreach(QGraphicsItem *gi, mActiveScene->selectedItems ())
    {
        //force item to redraw the frame (for the anti scale calculation)
        gi->setSelected(false);
        gi->setSelected(true);
    }

    emit zoomChanged(1.0);
}


void WBBoardController::centerRestore()
{
    centerOn(QPointF(0,0));
}


void WBBoardController::centerOn(QPointF scenePoint)
{
    mControlView->centerOn(scenePoint);
    WBApplication::applicationController->adjustDisplayView();
}


void WBBoardController::zoom(const qreal ratio, QPointF scenePoint)
{
    QPointF viewCenter = mControlView->mapToScene(QRect(0, 0, mControlView->width(), mControlView->height()).center());
    QPointF offset = scenePoint - viewCenter;
    QPointF scalledOffset = offset / ratio;

    qreal currentZoom = ratio * mControlView->viewportTransform().m11() / mSystemScaleFactor;

    qreal usedRatio = ratio;
    if (currentZoom > WB_MAX_ZOOM)
    {
        currentZoom = WB_MAX_ZOOM;
        usedRatio = currentZoom * mSystemScaleFactor / mControlView->viewportTransform().m11();
    }

    mControlView->scale(usedRatio, usedRatio);

    QPointF newCenter = scenePoint - scalledOffset;

    mControlView->centerOn(newCenter);

    emit zoomChanged(currentZoom);
    WBApplication::applicationController->adjustDisplayView();

    emit controlViewportChanged();
    mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());
}


void WBBoardController::handScroll(qreal dx, qreal dy)
{
    qreal antiScaleRatio = 1/(mSystemScaleFactor * currentZoom());
    mControlView->translate(dx*antiScaleRatio, dy*antiScaleRatio);

    WBApplication::applicationController->adjustDisplayView();

    emit controlViewportChanged();
}

void WBBoardController::persistViewPositionOnCurrentScene()
{
    QRect rect = mControlView->rect();
    QPoint center(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QPointF viewRelativeCenter = mControlView->mapToScene(center);
    mActiveScene->setLastCenter(viewRelativeCenter);
}

void WBBoardController::previousScene()
{
    if (mActiveSceneIndex > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(mActiveSceneIndex - 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}


void WBBoardController::nextScene()
{
    if (mActiveSceneIndex < selectedDocument()->pageCount() - 1)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(mActiveSceneIndex + 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}


void WBBoardController::firstScene()
{
    if (mActiveSceneIndex > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(0);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}


void WBBoardController::lastScene()
{
    if (mActiveSceneIndex < selectedDocument()->pageCount() - 1)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(selectedDocument()->pageCount() - 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}

void WBBoardController::downloadURL(const QUrl& url, QString contentSourceUrl, const QPointF& pPos, const QSize& pSize, bool isBackground, bool internalData)
{
    qDebug() << "something has been dropped on the board! Url is: " << url.toString();
    QString sUrl = url.toString();

    QGraphicsItem *oldBackgroundObject = NULL;
    if (isBackground)
        oldBackgroundObject = mActiveScene->backgroundObject();

    if(sUrl.startsWith("wboardtool://"))
    {
        downloadFinished(true, url, QUrl(), "application/wboard-tool", QByteArray(), pPos, pSize, isBackground);
    }
    else if (sUrl.startsWith("file://") || sUrl.startsWith("/"))
    {
        QUrl formedUrl = sUrl.startsWith("file://") ? url : QUrl::fromLocalFile(sUrl);
        QString fileName = formedUrl.toLocalFile();
        QString contentType = WBFileSystemUtils::mimeTypeFromFileName(fileName);

        bool shouldLoadFileData =
                contentType.startsWith("image")
                || contentType.startsWith("application/widget")
                || contentType.startsWith("application/vnd.apple-widget");

       if (shouldLoadFileData)
       {
            QFile file(fileName);
            file.open(QIODevice::ReadOnly);
            downloadFinished(true, formedUrl, QUrl(), contentType, file.readAll(), pPos, pSize, isBackground, internalData);
            file.close();
       }
       else
       {
           // media items should be copyed in separate thread

           sDownloadFileDesc desc;
           desc.modal = false;
           desc.srcUrl = sUrl;
           desc.originalSrcUrl = contentSourceUrl;
           desc.currentSize = 0;
           desc.name = QFileInfo(url.toString()).fileName();
           desc.totalSize = 0; // The total size will be retrieved during the download
           desc.pos = pPos;
           desc.size = pSize;
           desc.isBackground = isBackground;

           WBDownloadManager::downloadManager()->addFileToDownload(desc);
       }
    }
    else
    {
        QString urlString = url.toString();
        int parametersStringPosition = urlString.indexOf("?");
        if(parametersStringPosition != -1)
            urlString = urlString.left(parametersStringPosition);

        // When we fall there, it means that we are dropping something from the web to the board
        sDownloadFileDesc desc;
        desc.modal = true;
        desc.srcUrl = urlString;
        desc.currentSize = 0;
        desc.name = QFileInfo(urlString).fileName();
        desc.totalSize = 0; // The total size will be retrieved during the download
        desc.pos = pPos;
        desc.size = pSize;
        desc.isBackground = isBackground;

        WBDownloadManager::downloadManager()->addFileToDownload(desc);
    }

    if (isBackground && oldBackgroundObject != mActiveScene->backgroundObject())
    {
        if (mActiveScene->isURStackIsEnabled()) { //should be deleted after scene own undo stack implemented
            WBGraphicsItemUndoCommand* uc = new WBGraphicsItemUndoCommand(mActiveScene, oldBackgroundObject, mActiveScene->backgroundObject());
            WBApplication::undoStack->push(uc);
        }
    }


}


WBItem *WBBoardController::downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader,
                                            QByteArray pData, QPointF pPos, QSize pSize,
                                            bool isBackground, bool internalData)
{
    QString mimeType = pContentTypeHeader;

    // In some cases "image/jpeg;charset=" is retourned by the drag-n-drop. That is
    // why we will check if an ; exists and take the first part (the standard allows this kind of mimetype)
    if(mimeType.isEmpty())
      mimeType = WBFileSystemUtils::mimeTypeFromFileName(sourceUrl.toString());

    int position=mimeType.indexOf(";");
    if(position != -1)
        mimeType=mimeType.left(position);

    WBMimeType::Enum itemMimeType = WBFileSystemUtils::mimeTypeFromString(mimeType);

    if (!pSuccess)
    {
        showMessage(tr("Downloading content %1 failed").arg(sourceUrl.toString()));
        return NULL;
    }


    mActiveScene->deselectAllItems();

    if (!sourceUrl.toString().startsWith("file://") && !sourceUrl.toString().startsWith("wboardtool://"))
        showMessage(tr("Download finished"));

    if (WBMimeType::RasterImage == itemMimeType)
    {

        qDebug() << "accepting mime type" << mimeType << "as raster image";


        QPixmap pix;
        if(pData.length() == 0){
            pix.load(sourceUrl.toLocalFile());
        }
        else{
            QImage img;
            img.loadFromData(pData);
            pix = QPixmap::fromImage(img);
        }

        WBGraphicsPixmapItem* pixItem = mActiveScene->addPixmap(pix, NULL, pPos, 1.);
        pixItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(pixItem, true);
        }
        else
        {
            mActiveScene->scaleToFitDocumentSize(pixItem, true, WBSettings::objectInControlViewMargin);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }

        return pixItem;
    }
    else if (WBMimeType::VectorImage == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as vecto image";

        WBGraphicsSvgItem* svgItem = mActiveScene->addSvg(sourceUrl, pPos, pData);
        svgItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(svgItem);
        }
        else
        {
            mActiveScene->scaleToFitDocumentSize(svgItem, true, WBSettings::objectInControlViewMargin);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }

        return svgItem;
    }
    else if (WBMimeType::AppleWidget == itemMimeType) //mime type invented by us :-(
    {
        qDebug() << "accepting mime type" << mimeType << "as Apple widget";

        QUrl widgetUrl = sourceUrl;

        if (pData.length() > 0)
        {
            widgetUrl = expandWidgetToTempDir(pData, "wdgt");
        }

        WBGraphicsWidgetItem* appleWidgetItem = mActiveScene->addAppleWidget(widgetUrl, pPos);

        appleWidgetItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(appleWidgetItem);
        }
        else
        {
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }

        return appleWidgetItem;
    }
    else if (WBMimeType::W3CWidget == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as W3C widget";
        QUrl widgetUrl = sourceUrl;

        if (pData.length() > 0)
        {
            widgetUrl = expandWidgetToTempDir(pData);
        }

        WBGraphicsWidgetItem *w3cWidgetItem = addW3cWidget(widgetUrl, pPos);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(w3cWidgetItem);
        }
        else
        {
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }

        return w3cWidgetItem;
    }
    else if (WBMimeType::Video == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as video";

        WBGraphicsMediaItem *mediaVideoItem = 0;
        QUuid uuid = QUuid::createUuid();
        if (pData.length() > 0)
        {
            QString destFile;
            bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(selectedDocument(),
                sourceUrl.toString(),
                WBPersistenceManager::videoDirectory,
                uuid,
                destFile,
                &pData);
            if (!b)
            {
                showMessage(tr("Add file operation failed: file copying error"));
                return NULL;
            }

            QUrl url = QUrl::fromLocalFile(destFile);

            mediaVideoItem = mActiveScene->addMedia(url, false, pPos);
        }
        else
        {
            qDebug() << sourceUrl.toString();
            mediaVideoItem = addVideo(sourceUrl, false, pPos, true);
        }

        if(mediaVideoItem){
            if (contentUrl.isEmpty())
                mediaVideoItem->setSourceUrl(sourceUrl);
            else
                mediaVideoItem->setSourceUrl(contentUrl);
            mediaVideoItem->setUuid(uuid);
            connect(this, SIGNAL(activeSceneChanged()), mediaVideoItem, SLOT(activeSceneChanged()));
        }

        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);

        return mediaVideoItem;
    }
    else if (WBMimeType::Audio == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as audio";

        WBGraphicsMediaItem *audioMediaItem = 0;

        QUuid uuid = QUuid::createUuid();
        if (pData.length() > 0)
        {
            QString destFile;
            bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(selectedDocument(),
                sourceUrl.toString(),
                WBPersistenceManager::audioDirectory,
                uuid,
                destFile,
                &pData);
            if (!b)
            {
                showMessage(tr("Add file operation failed: file copying error"));
                return NULL;
            }

            QUrl url = QUrl::fromLocalFile(destFile);

            audioMediaItem = mActiveScene->addMedia(url, false, pPos);
        }
        else
        {
            audioMediaItem = addAudio(sourceUrl, false, pPos, true);
        }

        if(audioMediaItem){
            if (contentUrl.isEmpty())
                audioMediaItem->setSourceUrl(sourceUrl);
            else
                audioMediaItem->setSourceUrl(contentUrl);
            audioMediaItem->setUuid(uuid);
            connect(this, SIGNAL(activeSceneChanged()), audioMediaItem, SLOT(activeSceneChanged()));
        }

        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);

        return audioMediaItem;
    }	  
    else if (WBMimeType::PDF == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as PDF";
        qDebug() << "pdf data length: " << pData.size();
        qDebug() << "sourceurl : " + sourceUrl.toString();
        int result = 0;
        if(!sourceUrl.isEmpty()){
            QStringList fileNames;
            fileNames << sourceUrl.toLocalFile();
            result = WBDocumentManager::documentManager()->addFilesToDocument(selectedDocument(), fileNames);
        }
        else if(pData.size()){
            QTemporaryFile pdfFile("XXXXXX.pdf");
            if (pdfFile.open())
            {
                pdfFile.write(pData);
                QStringList fileNames;
                fileNames << pdfFile.fileName();
                result = WBDocumentManager::documentManager()->addFilesToDocument(selectedDocument(), fileNames);
                pdfFile.close();
            }
        }

        if (result){
            selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        }
    }
    else if (WBMimeType::WboardTool == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "WBoard Tool";

        if (sourceUrl.toString() == WBToolsManager::manager()->compass.id)
        {
            mActiveScene->addCompass(pPos);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->ruler.id)
        {
            mActiveScene->addRuler(pPos);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->protractor.id)
        {
            mActiveScene->addProtractor(pPos);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->triangle.id)
        {
            mActiveScene->addTriangle(pPos);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->cache.id)
        {
            mActiveScene->addCache();
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->magnifier.id)
        {
            WBMagnifierParams params;
            params.x = controlContainer()->geometry().width() / 2;
            params.y = controlContainer()->geometry().height() / 2;
            params.zoom = 2;
            params.sizePercentFromScene = 20;
            mActiveScene->addMagnifier(params);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else if (sourceUrl.toString() == WBToolsManager::manager()->mask.id)
        {
            mActiveScene->addMask(pPos);
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
        else
        {
            showMessage(tr("Unknown tool type %1").arg(sourceUrl.toString()));
        }
    }
    else if (sourceUrl.toString().contains("edumedia-sciences.com"))
    {
        qDebug() << "accepting url " << sourceUrl.toString() << "as eduMedia content";

        QTemporaryFile eduMediaZipFile("XXXXXX.edumedia");
        if (eduMediaZipFile.open())
        {
            eduMediaZipFile.write(pData);
            eduMediaZipFile.close();

            QString tempDir = WBFileSystemUtils::createTempDir("uniboard-edumedia");

            WBFileSystemUtils::expandZipToDir(eduMediaZipFile, tempDir);

            QDir appDir(tempDir);

            foreach(QString subDirName, appDir.entryList(QDir::AllDirs))
            {
                QDir subDir(tempDir + "/" + subDirName + "/contents");

                foreach(QString fileName, subDir.entryList(QDir::Files))
                {
                    if (fileName.toLower().endsWith(".swf"))
                    {
                        QString swfFile = tempDir + "/" + subDirName + "/contents/" + fileName;

                        QSize size;

                        if (pSize.height() > 0 && pSize.width() > 0)
                            size = pSize;
                        else
                            size = mActiveScene->nominalSize() * .8;

                        QString widgetUrl = WBGraphicsW3CWidgetItem::createNPAPIWrapper(swfFile, "application/x-shockwave-flash", size);

                        if (widgetUrl.length() > 0)
                        {
                            WBGraphicsWidgetItem *widgetItem = mActiveScene->addW3CWidget(QUrl::fromLocalFile(widgetUrl), pPos);

                            widgetItem->setSourceUrl(sourceUrl);

                            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);

                            return widgetItem;
                        }
                    }
                }
            }
        }
    }
    else
    {
        showMessage(tr("Unknown content type %1").arg(pContentTypeHeader));
        qWarning() << "ignoring mime type" << pContentTypeHeader ;
    }

    return NULL;
}

void WBBoardController::setActiveDocumentScene(int pSceneIndex)
{
    setActiveDocumentScene(selectedDocument(), pSceneIndex);
}

void WBBoardController::setActiveDocumentScene(WBDocumentProxy* pDocumentProxy, const int pSceneIndex, bool forceReload, bool onImport)
{
    saveViewState();

    bool documentChange = selectedDocument() != pDocumentProxy;

    int index = pSceneIndex;
    int sceneCount = pDocumentProxy->pageCount();
    if (index >= sceneCount && sceneCount > 0)
        index = sceneCount - 1;

    WBGraphicsScene* targetScene = WBPersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, index);

    bool sceneChange = targetScene != mActiveScene;

    if (targetScene)
    {
        if (mActiveScene && !onImport)
        {
            persistCurrentScene();
            freezeW3CWidgets(true);
            ClearUndoStack();
        }else
        {
            WBApplication::undoStack->clear();
        }

        mActiveScene = targetScene;
        mActiveSceneIndex = index;

        setDocument(pDocumentProxy, forceReload);

        updateSystemScaleFactor();

        mControlView->setScene(mActiveScene);
        disconnect(mControlView, SIGNAL(mouseReleased()), mActiveScene, SLOT(updateSelectionFrame()));
        connect(mControlView, SIGNAL(mouseReleased()), mActiveScene, SLOT(updateSelectionFrame()));

        mDisplayView->setScene(mActiveScene);
        mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());
        pDocumentProxy->setDefaultDocumentSize(mActiveScene->nominalSize());
        updatePageSizeState();

        adjustDisplayViews();

        WBSettings::settings()->setDarkBackground(mActiveScene->isDarkBackground());
        WBSettings::settings()->setPageBackground(mActiveScene->pageBackground());

        freezeW3CWidgets(false);
    }

    selectionChanged();

    updateBackgroundActionsState(mActiveScene->isDarkBackground(), mActiveScene->pageBackground());

    if(documentChange)
    {
        WBGraphicsTextItem::lastUsedTextColor = QColor(Qt::black);
    }

    if (sceneChange)
    {
        emit activeSceneChanged();
    }
}


void WBBoardController::moveSceneToIndex(int source, int target)
{
    if (selectedDocument())
    {
        persistCurrentScene(false,true);

        WBDocumentContainer::movePageToIndex(source, target);

        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        WBPersistenceManager::persistenceManager()->persistDocumentMetadata(selectedDocument());
        mMovingSceneIndex = source;
        mActiveSceneIndex = target;
        setActiveDocumentScene(target);
        mMovingSceneIndex = -1;

        emit activeSceneChanged();
        emit updateThumbnailsRequired();
    }
}

void WBBoardController::findUniquesItems(const QUndoCommand *parent, QSet<QGraphicsItem*> &itms)
{
    if (parent->childCount()) {
        for (int i = 0; i < parent->childCount(); i++) {
            findUniquesItems(parent->child(i), itms);
        }
    }

    // Undo command transaction macros. Process separatedly
    if (parent->text() == WBSettings::undoCommandTransactionName) {
        return;
    }

    const WBUndoCommand *undoCmd = static_cast<const WBUndoCommand*>(parent);
    if(undoCmd->getType() != WBUndoType::undotype_GRAPHICITEM)
        return;

    const WBGraphicsItemUndoCommand *cmd = dynamic_cast<const WBGraphicsItemUndoCommand*>(parent);

    // go through all added and removed objects, for create list of unique objects
    // grouped items will be deleted by groups, so we don't need do delete that items.
    QSetIterator<QGraphicsItem*> itAdded(cmd->GetAddedList());
    while (itAdded.hasNext())
    {
        QGraphicsItem* item = itAdded.next();
        if( !itms.contains(item) && !(item->parentItem() && WBGraphicsGroupContainerItem::Type == item->parentItem()->type()))
            itms.insert(item);
    }

    QSetIterator<QGraphicsItem*> itRemoved(cmd->GetRemovedList());
    while (itRemoved.hasNext())
    {
        QGraphicsItem* item = itRemoved.next();
        if( !itms.contains(item) && !(item->parentItem() && WBGraphicsGroupContainerItem::Type == item->parentItem()->type()))
            itms.insert(item);
    }
}

void WBBoardController::ClearUndoStack()
{
    QSet<QGraphicsItem*> uniqueItems;
    // go through all stack command
    for (int i = 0; i < WBApplication::undoStack->count(); i++) {
        findUniquesItems(WBApplication::undoStack->command(i), uniqueItems);
    }

    // Get items from clipboard in order not to delete an item that was cut
    // (using source URL of graphics items as a surrogate for equality testing)
    // This ensures that we can cut and paste a media item, widget, etc. from one page to the next.
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    QList<QUrl> sourceURLs;

    if (data && data->hasFormat(WBApplication::mimeTypeUniboardPageItem)) {
        const WBMimeDataGraphicsItem* mimeDataGI = qobject_cast <const WBMimeDataGraphicsItem*>(data);

        if (mimeDataGI) {
            foreach (WBItem* sourceItem, mimeDataGI->items()) {
                sourceURLs << sourceItem->sourceUrl();
            }
        }
    }

    // go through all unique items, and check, if they are on scene, or not.
    // if not on scene, than item can be deleted
    QSetIterator<QGraphicsItem*> itUniq(uniqueItems);
    while (itUniq.hasNext())
    {
        QGraphicsItem* item = itUniq.next();
        WBGraphicsScene *scene = NULL;
        if (item->scene()) {
            scene = dynamic_cast<WBGraphicsScene*>(item->scene());
        }

        bool inClipboard = false;
        WBItem* ubi = dynamic_cast<WBItem*>(item);
        if (ubi && sourceURLs.contains(ubi->sourceUrl()))
            inClipboard = true;

        if(!scene && !inClipboard)
        {
            if (!mActiveScene->deleteItem(item)){
                delete item;
                item = 0;
            }
        }
    }

    // clear stack, and command list
    WBApplication::undoStack->clear();
}

void WBBoardController::adjustDisplayViews()
{
    if (WBApplication::applicationController)
    {
        WBApplication::applicationController->adjustDisplayView();
        WBApplication::applicationController->adjustPreviousViews(mActiveSceneIndex, selectedDocument());
    }
}


int WBBoardController::autosaveTimeoutFromSettings()
{
    int value = WBSettings::settings()->autoSaveInterval->get().toInt();
    int minute = 60 * 1000;

    return value * minute;
}

void WBBoardController::changeBackground(bool isDark, WBPageBackground pageBackground)
{
    bool currentIsDark = mActiveScene->isDarkBackground();
    WBPageBackground currentBackgroundType = mActiveScene->pageBackground();

    if ((isDark != currentIsDark) || (currentBackgroundType != pageBackground))
    {
        WBSettings::settings()->setDarkBackground(isDark);
        WBSettings::settings()->setPageBackground(pageBackground);

        mActiveScene->setBackground(isDark, pageBackground);

        emit backgroundChanged();
    }
}

void WBBoardController::boardViewResized(QResizeEvent* event)
{
    Q_UNUSED(event);

    int innerMargin = WBSettings::boardMargin;
    int userHeight = mControlContainer->height() - (2 * innerMargin);

    mMessageWindow->move(innerMargin, innerMargin + userHeight - mMessageWindow->height());
    mMessageWindow->adjustSizeAndPosition();

    WBApplication::applicationController->initViewState(
                mControlView->horizontalScrollBar()->value(),
                mControlView->verticalScrollBar()->value());

    updateSystemScaleFactor();

    mControlView->centerOn(0,0);

    if (mDisplayView) {
        WBApplication::applicationController->adjustDisplayView();
        mDisplayView->centerOn(0,0);
    }

    mPaletteManager->containerResized();

    WBApplication::boardController->controlView()->scene()->moveMagnifier();

}


void WBBoardController::documentWillBeDeleted(WBDocumentProxy* pProxy)
{
    if (selectedDocument() == pProxy)
    {
        if (!mIsClosing)
            setActiveDocumentScene(WBPersistenceManager::persistenceManager()->createDocument());
    }
}


void WBBoardController::showMessage(const QString& message, bool showSpinningWheel)
{
    mMessageWindow->showMessage(message, showSpinningWheel);
}


void WBBoardController::hideMessage()
{
    mMessageWindow->hideMessage();
}


void WBBoardController::setDisabled(bool disable)
{
    mMainWindow->boardToolBar->setDisabled(disable);
    mControlView->setDisabled(disable);
}


void WBBoardController::selectionChanged()
{
    updateActionStates();
    emit pageSelectionChanged(activeSceneIndex());
    emit updateThumbnailsRequired();
}


void WBBoardController::undoRedoStateChange(bool canUndo)
{
    Q_UNUSED(canUndo);

    mMainWindow->actionUndo->setEnabled(WBApplication::undoStack->canUndo());
    mMainWindow->actionRedo->setEnabled(WBApplication::undoStack->canRedo());

    updateActionStates();
}


void WBBoardController::updateActionStates()
{
    mMainWindow->actionBack->setEnabled(selectedDocument() && (mActiveSceneIndex > 0));
    mMainWindow->actionForward->setEnabled(selectedDocument() && (mActiveSceneIndex < selectedDocument()->pageCount() - 1));
    mMainWindow->actionErase->setEnabled(mActiveScene && !mActiveScene->isEmpty());
}


WBGraphicsScene* WBBoardController::activeScene() const
{
    return mActiveScene;
}


int WBBoardController::activeSceneIndex() const
{
    return mActiveSceneIndex;
}

void WBBoardController::setActiveSceneIndex(int i)
{
    mActiveSceneIndex = i;
}

void WBBoardController::documentSceneChanged(WBDocumentProxy* pDocumentProxy, int pIndex)
{
    Q_UNUSED(pIndex);

    if(selectedDocument() == pDocumentProxy)
    {
        setActiveDocumentScene(mActiveSceneIndex);
        updatePage(pIndex);
    }
}

void WBBoardController::autosaveTimeout()
{
    if (WBApplication::applicationController->displayMode() != WBApplicationController::Board) {
        //perform autosave only in board mode
        return;
    }

    saveData(sf_showProgress);
    WBSettings::settings()->save();
}

void WBBoardController::appMainModeChanged(WBApplicationController::MainMode md)
{
    int autoSaveInterval = autosaveTimeoutFromSettings();
    if (!autoSaveInterval) {
        return;
    }

    if (!mAutosaveTimer) {
        mAutosaveTimer = new QTimer(this);
        connect(mAutosaveTimer, SIGNAL(timeout()), this, SLOT(autosaveTimeout()));
    }

    if (md == WBApplicationController::Board) {
        mAutosaveTimer->start(autoSaveInterval);
    } else if (mAutosaveTimer->isActive()) {
        mAutosaveTimer->stop();
    }
}

void WBBoardController::closing()
{
    mIsClosing = true;
    lastWindowClosed();
    ClearUndoStack();
    showKeyboard(false);
}

void WBBoardController::lastWindowClosed()
{
    if (!mCleanupDone)
    {
        if (selectedDocument()->pageCount() == 1 && (!mActiveScene || mActiveScene->isEmpty()))
        {
            WBPersistenceManager::persistenceManager()->deleteDocument(selectedDocument());
        }
        else
        {
            persistCurrentScene();
        }

        WBPersistenceManager::persistenceManager()->purgeEmptyDocuments();

        mCleanupDone = true;
    }
}



void WBBoardController::setColorIndex(int pColorIndex)
{
    WBDrawingController::drawingController()->setColorIndex(pColorIndex);

    if (WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Marker &&
            WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Line &&
            WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Text &&
            WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Selector)
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Pen);
    }

    if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Pen ||
            WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Line ||
            WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Text ||
            WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Selector)
    {
        mPenColorOnDarkBackground = WBSettings::settings()->penColors(true).at(pColorIndex);
        mPenColorOnLightBackground = WBSettings::settings()->penColors(false).at(pColorIndex);

        if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Selector)
        {
            // If we are in mode board, then do that
            if(WBApplication::applicationController->displayMode() == WBApplicationController::Board)
            {
                WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Pen);
                mMainWindow->actionPen->setChecked(true);
            }
        }

        emit penColorChanged();
    }
    else if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Marker)
    {
        mMarkerColorOnDarkBackground = WBSettings::settings()->markerColors(true).at(pColorIndex);
        mMarkerColorOnLightBackground = WBSettings::settings()->markerColors(false).at(pColorIndex);
    }
}

void WBBoardController::colorPaletteChanged()
{
    mPenColorOnDarkBackground = WBSettings::settings()->penColor(true);
    mPenColorOnLightBackground = WBSettings::settings()->penColor(false);
    mMarkerColorOnDarkBackground = WBSettings::settings()->markerColor(true);
    mMarkerColorOnLightBackground = WBSettings::settings()->markerColor(false);
}


qreal WBBoardController::currentZoom()
{
    if (mControlView)
        return mControlView->viewportTransform().m11() / mSystemScaleFactor;
    else
        return 1.0;
}

void WBBoardController::removeTool(WBToolWidget* toolWidget)
{
    toolWidget->hide();

    delete toolWidget;
}

void WBBoardController::hide()
{
    WBApplication::mainWindow->actionLibrary->setChecked(false);
}

void WBBoardController::show()
{
    WBApplication::mainWindow->actionLibrary->setChecked(false);
}

void WBBoardController::persistCurrentScene(bool isAnAutomaticBackup, bool forceImmediateSave)
{
    if(WBPersistenceManager::persistenceManager()
            && selectedDocument() && mActiveScene && mActiveSceneIndex != mDeletingSceneIndex
            && (mActiveSceneIndex >= 0) && mActiveSceneIndex != mMovingSceneIndex
            && (mActiveScene->isModified()))
    {
        WBPersistenceManager::persistenceManager()->persistDocumentScene(selectedDocument(), mActiveScene, mActiveSceneIndex);
        updatePage(mActiveSceneIndex);
    }
}

void WBBoardController::updateSystemScaleFactor()
{
    qreal newScaleFactor = 1.0;

    if (mActiveScene)
    {
        QSize pageNominalSize = mActiveScene->nominalSize();
        //we're going to keep scale factor untouched if the size is custom
        QMap<DocumentSizeRatio::Enum, QSize> sizesMap = WBSettings::settings()->documentSizes;
      //  if(pageNominalSize == sizesMap.value(DocumentSizeRatio::Ratio16_9) || pageNominalSize == sizesMap.value(DocumentSizeRatio::Ratio4_3))
        {
            QSize controlSize = controlViewport();

            qreal hFactor = ((qreal)controlSize.width()) / ((qreal)pageNominalSize.width());
            qreal vFactor = ((qreal)controlSize.height()) / ((qreal)pageNominalSize.height());

            newScaleFactor = qMin(hFactor, vFactor);
        }
    }

    if (mSystemScaleFactor != newScaleFactor)
        mSystemScaleFactor = newScaleFactor;

    WBGraphicsScene::SceneViewState viewState = mActiveScene->viewState();

    QTransform scalingTransform;

    qreal scaleFactor = viewState.zoomFactor * mSystemScaleFactor;
    scalingTransform.scale(scaleFactor, scaleFactor);

    mControlView->setTransform(scalingTransform);
    mControlView->horizontalScrollBar()->setValue(viewState.horizontalPosition);
    mControlView->verticalScrollBar()->setValue(viewState.verticalPostition);
    mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());}


void WBBoardController::setWidePageSize(bool checked)
{
    Q_UNUSED(checked);
    QSize newSize = WBSettings::settings()->documentSizes.value(DocumentSizeRatio::Ratio16_9);

    if (mActiveScene->nominalSize() != newSize)
    {
        WBPageSizeUndoCommand* uc = new WBPageSizeUndoCommand(mActiveScene, mActiveScene->nominalSize(), newSize);
        WBApplication::undoStack->push(uc);

        setPageSize(newSize);
    }
}


void WBBoardController::setRegularPageSize(bool checked)
{
    Q_UNUSED(checked);
    QSize newSize = WBSettings::settings()->documentSizes.value(DocumentSizeRatio::Ratio4_3);

    if (mActiveScene->nominalSize() != newSize)
    {
        WBPageSizeUndoCommand* uc = new WBPageSizeUndoCommand(mActiveScene, mActiveScene->nominalSize(), newSize);
        WBApplication::undoStack->push(uc);

        setPageSize(newSize);
    }
}


void WBBoardController::setPageSize(QSize newSize)
{
    if (mActiveScene->nominalSize() != newSize)
    {
        mActiveScene->setNominalSize(newSize);

        saveViewState();

        updateSystemScaleFactor();
        updatePageSizeState();
        adjustDisplayViews();
        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        WBSettings::settings()->pageSize->set(newSize);
    }
}

void WBBoardController::notifyCache(bool visible)
{
    if(visible)
        emit cacheEnabled();

    mCacheWidgetIsEnabled = visible;
}

void WBBoardController::updatePageSizeState()
{
    if (mActiveScene->nominalSize() == WBSettings::settings()->documentSizes.value(DocumentSizeRatio::Ratio16_9))
    {
        mMainWindow->actionWidePageSize->setChecked(true);
    }
    else if(mActiveScene->nominalSize() == WBSettings::settings()->documentSizes.value(DocumentSizeRatio::Ratio4_3))
    {
        mMainWindow->actionRegularPageSize->setChecked(true);
    }
    else
    {
        mMainWindow->actionCustomPageSize->setChecked(true);
    }
}


void WBBoardController::saveViewState()
{
    if (mActiveScene)
    {
        mActiveScene->setViewState(WBGraphicsScene::SceneViewState(currentZoom(),
                                                                   mControlView->horizontalScrollBar()->value(),
                                                                   mControlView->verticalScrollBar()->value(),
                                                                   mActiveScene->lastCenter()));
    }
}

void WBBoardController::stylusToolChanged(int tool)
{
    if (WBPlatformUtils::hasVirtualKeyboard() && mPaletteManager->mKeyboardPalette)
    {
        WBStylusTool::Enum eTool = (WBStylusTool::Enum)tool;
        if(eTool != WBStylusTool::Selector && eTool != WBStylusTool::Text)
        {
            if(mPaletteManager->mKeyboardPalette->m_isVisible)
                WBApplication::mainWindow->actionVirtualKeyboard->activate(QAction::Trigger);
        }
    }

}


QUrl WBBoardController::expandWidgetToTempDir(const QByteArray& pZipedData, const QString& ext)
{
    QUrl widgetUrl;
    QTemporaryFile tmp;

    if (tmp.open())
    {
        tmp.write(pZipedData);
        tmp.flush();
        tmp.close();

        QString tmpDir = WBFileSystemUtils::createTempDir() + "." + ext;

        if (WBFileSystemUtils::expandZipToDir(tmp, tmpDir))
        {
            widgetUrl = QUrl::fromLocalFile(tmpDir);
        }
    }

    return widgetUrl;
}


void WBBoardController::grabScene(const QRectF& pSceneRect)
{
    if (mActiveScene)
    {
        QImage image(pSceneRect.width(), pSceneRect.height(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QRectF targetRect(0, 0, pSceneRect.width(), pSceneRect.height());
        QPainter painter(&image);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        mActiveScene->setRenderingContext(WBGraphicsScene::NonScreen);
        mActiveScene->setRenderingQuality(WBItem::RenderingQualityHigh);

        mActiveScene->render(&painter, targetRect, pSceneRect);

        mActiveScene->setRenderingContext(WBGraphicsScene::Screen);
//        mActiveScene->setRenderingQuality(WBItem::RenderingQualityNormal);
        mActiveScene->setRenderingQuality(WBItem::RenderingQualityHigh);


        mPaletteManager->addItem(QPixmap::fromImage(image));
        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}

WBGraphicsMediaItem* WBBoardController::addVideo(const QUrl& pSourceUrl, bool startPlay, const QPointF& pos, bool bUseSource)
{
    QUuid uuid = QUuid::createUuid();
    QUrl concreteUrl = pSourceUrl;

    // media file is not in document folder yet
    if (!bUseSource)
    {
        QString destFile;
        bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(selectedDocument(),
                    pSourceUrl.toLocalFile(),
                    WBPersistenceManager::videoDirectory,
                    uuid,
                    destFile);
        if (!b)
        {
            showMessage(tr("Add file operation failed: file copying error"));
            return NULL;
        }
        concreteUrl = QUrl::fromLocalFile(destFile);
    }// else we just use source Url.


    WBGraphicsMediaItem* vi = mActiveScene->addMedia(concreteUrl, startPlay, pos);
    selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    if (vi) {
        vi->setUuid(uuid);
        vi->setSourceUrl(pSourceUrl);
    }

    return vi;

}

WBGraphicsMediaItem* WBBoardController::addAudio(const QUrl& pSourceUrl, bool startPlay, const QPointF& pos, bool bUseSource)
{
    QUuid uuid = QUuid::createUuid();
    QUrl concreteUrl = pSourceUrl;

    // media file is not in document folder yet
    if (!bUseSource)
    {
        QString destFile;
        bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(selectedDocument(),
            pSourceUrl.toLocalFile(),
            WBPersistenceManager::audioDirectory,
            uuid,
            destFile);
        if (!b)
        {
            showMessage(tr("Add file operation failed: file copying error"));
            return NULL;
        }
        concreteUrl = QUrl::fromLocalFile(destFile);
    }// else we just use source Url.

    WBGraphicsMediaItem* ai = mActiveScene->addMedia(concreteUrl, startPlay, pos);
    selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    if (ai){
        ai->setUuid(uuid);
        ai->setSourceUrl(pSourceUrl);
    }

    return ai;

}

WBGraphicsWidgetItem *WBBoardController::addW3cWidget(const QUrl &pUrl, const QPointF &pos)
{
    WBGraphicsWidgetItem* w3cWidgetItem = 0;

    QUuid uuid = QUuid::createUuid();

    QString destPath;
    if (!WBPersistenceManager::persistenceManager()->addGraphicsWidgetToDocument(selectedDocument(), pUrl.toLocalFile(), uuid, destPath))
        return NULL;
    QUrl newUrl = QUrl::fromLocalFile(destPath);

    w3cWidgetItem = mActiveScene->addW3CWidget(newUrl, pos);

    if (w3cWidgetItem) {
        w3cWidgetItem->setUuid(uuid);
        w3cWidgetItem->setOwnFolder(newUrl);
        w3cWidgetItem->setSourceUrl(pUrl);

        QString struuid = WBStringUtils::toCanonicalUuid(uuid);
        QString snapshotPath = selectedDocument()->persistencePath() +  "/" + WBPersistenceManager::widgetDirectory + "/" + struuid + ".png";
        w3cWidgetItem->setSnapshotPath(QUrl::fromLocalFile(snapshotPath));
        WBGraphicsWidgetItem *tmpItem = dynamic_cast<WBGraphicsWidgetItem*>(w3cWidgetItem);
        if (tmpItem && tmpItem->scene())
           tmpItem->takeSnapshot().save(snapshotPath, "PNG");

    }

    return w3cWidgetItem;
}

void WBBoardController::cut()
{
    //---------------------------------------------------------//

    QList<QGraphicsItem*> selectedItems;
    foreach(QGraphicsItem* gi, mActiveScene->selectedItems())
        selectedItems << gi;

    //---------------------------------------------------------//

    QList<WBItem*> selected;
    foreach(QGraphicsItem* gi, selectedItems)
    {
        gi->setSelected(false);

        WBItem* ubItem = dynamic_cast<WBItem*>(gi);
        WBGraphicsItem *ubGi =  dynamic_cast<WBGraphicsItem*>(gi);

        if (ubItem && ubGi && !mActiveScene->tools().contains(gi))
        {
            selected << ubItem->deepCopy();
            ubGi->remove();
        }
    }

    //---------------------------------------------------------//

    if (selected.size() > 0)
    {
        QClipboard *clipboard = QApplication::clipboard();

        WBMimeDataGraphicsItem*  mimeGi = new WBMimeDataGraphicsItem(selected);

        mimeGi->setData(WBApplication::mimeTypeUniboardPageItem, QByteArray());
        clipboard->setMimeData(mimeGi);

        selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }

    //---------------------------------------------------------//
}


void WBBoardController::copy()
{
    QList<WBItem*> selected;

    foreach(QGraphicsItem* gi, mActiveScene->selectedItems())
    {
        WBItem* ubItem = dynamic_cast<WBItem*>(gi);

        if (ubItem && !mActiveScene->tools().contains(gi))
            selected << ubItem;
    }

    if (selected.size() > 0)
    {
        QClipboard *clipboard = QApplication::clipboard();

        WBMimeDataGraphicsItem*  mimeGi = new WBMimeDataGraphicsItem(selected);

        mimeGi->setData(WBApplication::mimeTypeUniboardPageItem, QByteArray());
        clipboard->setMimeData(mimeGi);

    }
}


void WBBoardController::paste()
{
    QClipboard *clipboard = QApplication::clipboard();
    qreal xPosition = ((qreal)qrand()/(qreal)RAND_MAX) * 400;
    qreal yPosition = ((qreal)qrand()/(qreal)RAND_MAX) * 200;
    QPointF pos(xPosition -200 , yPosition - 100);
    processMimeData(clipboard->mimeData(), pos);

    selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
}


bool zLevelLessThan( WBItem* s1, WBItem* s2)
{
    qreal s1Zvalue = dynamic_cast<QGraphicsItem*>(s1)->data(WBGraphicsItemData::ItemOwnZValue).toReal();
    qreal s2Zvalue = dynamic_cast<QGraphicsItem*>(s2)->data(WBGraphicsItemData::ItemOwnZValue).toReal();
    return s1Zvalue < s2Zvalue;
}

void WBBoardController::processMimeData(const QMimeData* pMimeData, const QPointF& pPos)
{
    if (pMimeData->hasFormat(WBApplication::mimeTypeUniboardPage))
    {
        const WBMimeData* mimeData = qobject_cast <const WBMimeData*>(pMimeData);

        if (mimeData)
        {
            int previousActiveSceneIndex = activeSceneIndex();
            int previousPageCount = selectedDocument()->pageCount();

            foreach (WBMimeDataItem sourceItem, mimeData->items())
                addScene(sourceItem.documentProxy(), sourceItem.sceneIndex(), true);

            if (selectedDocument()->pageCount() < previousPageCount + mimeData->items().count())
                setActiveDocumentScene(previousActiveSceneIndex);
            else
                setActiveDocumentScene(previousActiveSceneIndex + 1);

            return;
        }
    }

    if (pMimeData->hasFormat(WBApplication::mimeTypeUniboardPageItem))
    {
        const WBMimeDataGraphicsItem* mimeData = qobject_cast <const WBMimeDataGraphicsItem*>(pMimeData);

        if (mimeData)
        {
            QList<WBItem*> items = mimeData->items();
            qStableSort(items.begin(),items.end(),zLevelLessThan);
            foreach(WBItem* item, items)
            {
                QGraphicsItem* pItem = dynamic_cast<QGraphicsItem*>(item);
                if(NULL != pItem){
                    duplicateItem(item);
                }
            }

            return;
        }
    }

    if(pMimeData->hasHtml())
    {
        QString qsHtml = pMimeData->html();
        QString url = WBApplication::urlFromHtml(qsHtml);

        if("" != url)
        {
            downloadURL(url, QString(), pPos);
            return;
        }
    }

    if (pMimeData->hasUrls())
    {
        QList<QUrl> urls = pMimeData->urls();

        int index = 0;

        const WBFeaturesMimeData *internalMimeData = qobject_cast<const WBFeaturesMimeData*>(pMimeData);
        bool internalData = false;
        if (internalMimeData) {
            internalData = true;
        }

        foreach(const QUrl url, urls){
            QPointF pos(pPos + QPointF(index * 15, index * 15));

            downloadURL(url, QString(), pos, QSize(), false,  internalData);
            index++;
        }

        return;
    }

    if (pMimeData->hasImage())
    {
        QImage img = qvariant_cast<QImage> (pMimeData->imageData());
        QPixmap pix = QPixmap::fromImage(img);

        // validate that the image is really an image, webkit does not fill properly the image mime data
        if (pix.width() != 0 && pix.height() != 0)
        {
            mActiveScene->addPixmap(pix, NULL, pPos, 1.);
            return;
        }
    }

    if (pMimeData->hasText())
    {
        if("" != pMimeData->text()){
            // Sometimes, it is possible to have an URL as text. we check here if it is the case
            QString qsTmp = pMimeData->text().remove(QRegExp("[\\0]"));
            if(qsTmp.startsWith("http"))
                downloadURL(QUrl(qsTmp), QString(), pPos);
            else{
                if(mActiveScene->selectedItems().count() && mActiveScene->selectedItems().at(0)->type() == WBGraphicsItemType::TextItemType)
                    dynamic_cast<WBGraphicsTextItem*>(mActiveScene->selectedItems().at(0))->setHtml(pMimeData->text());
                else
                    mActiveScene->addTextHtml("", pPos)->setHtml(pMimeData->text());
            }
        }
        else{
#ifdef Q_OS_OSX
                //  With Safari, in 95% of the drops, the mime datas are hidden in Apple Web Archive pasteboard type.
                //  This is due to the way Safari is working so we have to dig into the pasteboard in order to retrieve
                //  the data.
                QString qsUrl = WBPlatformUtils::urlFromClipboard();
                if("" != qsUrl){
                    // We finally got the url of the dropped ressource! Let's import it!
                    downloadURL(qsUrl, qsUrl, pPos);
                    return;
                }
#endif
        }
    }
}


void WBBoardController::togglePodcast(bool checked)
{
    if (WBPodcastController::instance())
        WBPodcastController::instance()->toggleRecordingPalette(checked);
}

void WBBoardController::moveGraphicsWidgetToControlView(WBGraphicsWidgetItem* graphicsWidget)
{
    mActiveScene->setURStackEnable(false);
    WBGraphicsItem *toolW3C = duplicateItem(dynamic_cast<WBItem *>(graphicsWidget));
    WBGraphicsWidgetItem *copyedGraphicsWidget = NULL;

    if (toolW3C)
    {
        if (WBGraphicsWidgetItem::Type == toolW3C->type())
            copyedGraphicsWidget = static_cast<WBGraphicsWidgetItem *>(toolW3C);

        WBToolWidget *toolWidget = new WBToolWidget(copyedGraphicsWidget, mControlView);

        graphicsWidget->remove(false);
        mActiveScene->addItemToDeletion(graphicsWidget);

        mActiveScene->setURStackEnable(true);

        QPoint controlViewPos = mControlView->mapFromScene(graphicsWidget->sceneBoundingRect().center());
        toolWidget->centerOn(mControlView->mapTo(mControlContainer, controlViewPos));
        toolWidget->show();
    }
}


void WBBoardController::moveToolWidgetToScene(WBToolWidget* toolWidget)
{
    WBGraphicsWidgetItem *widgetToScene = toolWidget->toolWidget();

    widgetToScene->resetTransform();

    QPoint mainWindowCenter = toolWidget->mapTo(mMainWindow, QPoint(toolWidget->width(), toolWidget->height()) / 2);
    QPoint controlViewCenter = mControlView->mapFrom(mMainWindow, mainWindowCenter);
    QPointF scenePos = mControlView->mapToScene(controlViewCenter);

    mActiveScene->addGraphicsWidget(widgetToScene, scenePos);

    toolWidget->remove();
}


void WBBoardController::updateBackgroundActionsState(bool isDark, WBPageBackground pageBackground)
{
    switch (pageBackground) {

        case WBPageBackground::crossed:
            if (isDark)
                mMainWindow->actionCrossedDarkBackground->setChecked(true);
            else
                mMainWindow->actionCrossedLightBackground->setChecked(true);
        break;

        case WBPageBackground::ruled :
            if (isDark)
                mMainWindow->actionRuledDarkBackground->setChecked(true);
            else
                mMainWindow->actionRuledLightBackground->setChecked(true);
        break;

        default:
            if (isDark)
                mMainWindow->actionPlainDarkBackground->setChecked(true);
            else
                mMainWindow->actionPlainLightBackground->setChecked(true);
        break;
    }
}


void WBBoardController::addItem()
{
    QString defaultPath = WBSettings::settings()->lastImportToLibraryPath->get().toString();

    QString extensions;
    foreach(QString ext, WBSettings::imageFileExtensions)
    {
        extensions += " *.";
        extensions += ext;
    }

    QString filename = QFileDialog::getOpenFileName(mControlContainer, tr("Add Item"),
                                                    defaultPath,
                                                    tr("All Supported (%1)").arg(extensions), NULL, QFileDialog::DontUseNativeDialog);

    if (filename.length() > 0)
    {
        mPaletteManager->addItem(QUrl::fromLocalFile(filename));
        QFileInfo source(filename);
        WBSettings::settings()->lastImportToLibraryPath->set(QVariant(source.absolutePath()));
    }
}

void WBBoardController::importPage()
{
    int pageCount = selectedDocument()->pageCount();
    if (WBApplication::documentController->addFileToDocument(selectedDocument()))
    {
        setActiveDocumentScene(selectedDocument(), pageCount, true);
    }
}

void WBBoardController::notifyPageChanged()
{
    emit activeSceneChanged();
}

void WBBoardController::onDownloadModalFinished()
{

}

void WBBoardController::displayMetaData(QMap<QString, QString> metadatas)
{
    emit displayMetadata(metadatas);
}

void WBBoardController::freezeW3CWidgets(bool freeze)
{
    if (mActiveSceneIndex >= 0)
    {
        QList<QGraphicsItem *> list = WBApplication::boardController->activeScene()->getFastAccessItems();
        foreach(QGraphicsItem *item, list)
        {
            freezeW3CWidget(item, freeze);
        }
    }
}

void WBBoardController::freezeW3CWidget(QGraphicsItem *item, bool freeze)
{
    if(item->type() == WBGraphicsW3CWidgetItem::Type)
    {
        WBGraphicsW3CWidgetItem* item_casted = dynamic_cast<WBGraphicsW3CWidgetItem*>(item);
        if (0 == item_casted)
            return;

        //if (freeze) {
        //    item_casted->load(QUrl(WBGraphicsW3CWidgetItem::freezedWidgetFilePath()));
        //} else
            item_casted->loadMainHtml();
    }
}
