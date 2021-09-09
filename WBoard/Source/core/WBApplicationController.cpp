#include "WBApplicationController.h"

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBVersion.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBDocumentManager.h"
#include "core/WBDisplayManager.h"
#include "core/WBOpenSankoreImporter.h"


#include "board/WBBoardView.h"
#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"
#include "board/WBDrawingController.h"

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentController.h"

#include "domain/WBGraphicsWidgetItem.h"

#include "desktop/WBDesktopPalette.h"
#include "desktop/WBDesktopAnnotationController.h"

#include "web/WBWebController.h"

#include "gui/WBScreenMirror.h"
#include "gui/WBMainWindow.h"

#include "domain/WBGraphicsPixmapItem.h"

#include "podcast/WBPodcastController.h"

#include "network/WBNetworkAccessManager.h"

#include "ui_mainWindow.h"



#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

#include "core/memcheck.h"

WBApplicationController::WBApplicationController(WBBoardView *pControlView,
                                                 WBBoardView *pDisplayView,
                                                 WBMainWindow* pMainWindow,
                                                 QObject* parent,
                                                 WBRightPalette* rightPalette)
    : QObject(parent)
    , mMainWindow(pMainWindow)
    , mControlView(pControlView)
    , mDisplayView(pDisplayView)
    , mMirror(0)
    , mMainMode(Board)
    , mDisplayManager(0)
    , mAutomaticCheckForUpdates(false)
    , mCheckingForUpdates(false)
    , mIsShowingDesktop(false)
{
    mDisplayManager = new WBDisplayManager(this);

    mUninoteController = new WBDesktopAnnotationController(this, rightPalette);

    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), this, SLOT(screenLayoutChanged()));
    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), mUninoteController, SLOT(screenLayoutChanged()));
    connect(mDisplayManager, SIGNAL(screenLayoutChanged()), WBApplication::webController, SLOT(screenLayoutChanged()));
    connect(mDisplayManager, SIGNAL(adjustDisplayViewsRequired()), WBApplication::boardController, SLOT(adjustDisplayViews()));
    connect(mUninoteController, SIGNAL(imageCaptured(const QPixmap &, bool)), this, SLOT(addCapturedPixmap(const QPixmap &, bool)));
    connect(mUninoteController, SIGNAL(restoreUniboard()), this, SLOT(hideDesktop()));

    for(int i = 0; i < mDisplayManager->numPreviousViews(); i++)
    {
        WBBoardView *previousView = new WBBoardView(WBApplication::boardController, WBItemLayerType::FixedBackground, WBItemLayerType::Tool, 0);
        previousView->setInteractive(false);
        mPreviousViews.append(previousView);
    }

    mBlackScene = new WBGraphicsScene(0); // deleted by WBApplicationController::destructor
    mBlackScene->setBackground(true, WBPageBackground::plain);

    if (mDisplayManager->numScreens() >= 2 && mDisplayManager->useMultiScreen())
    {
        mMirror = new WBScreenMirror();
    }

    connect(WBApplication::webController, SIGNAL(imageCaptured(const QPixmap &, bool, const QUrl&))
            , this, SLOT(addCapturedPixmap(const QPixmap &, bool, const QUrl&)));

    mNetworkAccessManager = new QNetworkAccessManager (this);
    QTimer::singleShot (1000, this, SLOT (checkAtLaunch()));
}


WBApplicationController::~WBApplicationController()
{
    foreach(WBBoardView* view, mPreviousViews)
    {
        delete view;
    }

    delete mBlackScene;
    delete mMirror;

    delete(mOpenSankoreImporter);
    mOpenSankoreImporter = NULL;
}


void WBApplicationController::initViewState(int horizontalPosition, int verticalPostition)
{
    mInitialHScroll = horizontalPosition;
    mInitialVScroll = verticalPostition;
}


void WBApplicationController::initScreenLayout(bool useMultiscreen)
{
    mDisplayManager->setControlWidget(mMainWindow);
    mDisplayManager->setDisplayWidget(mDisplayView);

    mDisplayManager->setPreviousDisplaysWidgets(mPreviousViews);
    mDisplayManager->setDesktopWidget(mUninoteController->drawingView());

    mDisplayManager->setUseMultiScreen(useMultiscreen);
    mDisplayManager->adjustScreens(-1);
}


void WBApplicationController::screenLayoutChanged()
{
    initViewState(mControlView->horizontalScrollBar()->value(),
            mControlView->verticalScrollBar()->value());

    adaptToolBar();

    adjustDisplayView();

    if (mDisplayManager->hasDisplay())
    {
        WBApplication::boardController->setBoxing(mDisplayView->geometry());
    }
    else
    {
       WBApplication::boardController->setBoxing(QRect());
    }

    adjustPreviousViews(0, 0);
}


void WBApplicationController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->actionClearPage->setVisible(Board == mMainMode && highResolution);
    mMainWindow->actionBoard->setVisible(Board != mMainMode || highResolution);
    mMainWindow->actionDocument->setVisible(Document != mMainMode || highResolution);
    mMainWindow->actionWeb->setVisible(Internet != mMainMode || highResolution);
    mMainWindow->boardToolBar->setIconSize(QSize(highResolution ? 48 : 42, mMainWindow->boardToolBar->iconSize().height()));

    mMainWindow->actionBoard->setEnabled(mMainMode != Board);
    mMainWindow->actionWeb->setEnabled(mMainMode != Internet);
    mMainWindow->actionDocument->setEnabled(mMainMode != Document);

    if (Document == mMainMode)
    {
        connect(WBApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), WBApplication::documentController, SLOT(focusChanged(QWidget *, QWidget *)));
    }
    else
    {
        disconnect(WBApplication::instance(), SIGNAL(focusChanged(QWidget *, QWidget *)), WBApplication::documentController, SLOT(focusChanged(QWidget *, QWidget *)));
        if (Board == mMainMode)
            mMainWindow->actionDuplicate->setEnabled(true);
    }

    WBApplication::boardController->setToolbarTexts();

    WBApplication::webController->adaptToolBar();

}


void WBApplicationController::adjustDisplayView()
{
    if (mDisplayView)
    {
        qreal systemDisplayViewScaleFactor = 1.0;

        QSize pageSize = WBApplication::boardController->activeScene()->nominalSize();
        QSize displaySize = mDisplayView->size();

        qreal hFactor = ((qreal)displaySize.width()) / ((qreal)pageSize.width());
        qreal vFactor = ((qreal)displaySize.height()) / ((qreal)pageSize.height());

        systemDisplayViewScaleFactor = qMin(hFactor, vFactor);

        QTransform tr;
        qreal scaleFactor = systemDisplayViewScaleFactor * WBApplication::boardController->currentZoom();

        tr.scale(scaleFactor, scaleFactor);

        QRect rect = mControlView->rect();
        QPoint center(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);

        QTransform recentTransform = mDisplayView->transform();

        if (recentTransform != tr)
            mDisplayView->setTransform(tr);

        mDisplayView->centerOn(mControlView->mapToScene(center));
    }
}


void WBApplicationController::adjustPreviousViews(int pActiveSceneIndex, WBDocumentProxy *pActiveDocument)
{
    int viewIndex = pActiveSceneIndex;

    foreach(WBBoardView* previousView, mPreviousViews)
    {
        if (viewIndex > 0)
        {
            viewIndex--;

            WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(pActiveDocument, viewIndex);

            if (scene)
            {
                previousView->setScene(scene);

                qreal ratio = ((qreal)previousView->geometry().width()) / ((qreal)previousView->geometry().height());
                QRectF sceneRect = scene->normalizedSceneRect(ratio);
                qreal scaleRatio = previousView->geometry().width() / sceneRect.width();

                previousView->resetTransform();

                previousView->scale(scaleRatio, scaleRatio);

                previousView->centerOn(sceneRect.center());
            }
        }
        else
        {
            previousView->setScene(mBlackScene);
        }
    }
}


void WBApplicationController::blackout()
{
    mDisplayManager->blackout();
}


void WBApplicationController::addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl& sourceUrl)
{
    if (!pPixmap.isNull())
    {
        qreal sf = WBApplication::boardController->systemScaleFactor();
        qreal scaledWidth = ((qreal)pPixmap.width()) / sf;
        qreal scaledHeight = ((qreal)pPixmap.height()) / sf;

        QSize pageNominalSize = WBApplication::boardController->activeScene()->nominalSize();

        int newWidth = qMin((int)scaledWidth, pageNominalSize.width());
        int newHeight = qMin((int)scaledHeight, pageNominalSize.height());

        if (pageMode)
        {
            newHeight = pPixmap.height();
        }

        QSizeF scaledSize(scaledWidth, scaledHeight);
        scaledSize.scale(newWidth, newHeight, Qt::KeepAspectRatio);

        qreal scaleFactor = qMin(scaledSize.width() / (qreal)pPixmap.width(), scaledSize.height() / (qreal)pPixmap.height());

        QPointF pos(0.0, 0.0);

        if (pageMode)
        {
            pos.setY(pageNominalSize.height() / -2  + scaledSize.height() / 2);
        }

        WBApplication::boardController->paletteManager()->addItem(pPixmap, pos, scaleFactor, sourceUrl);
    }
}


void WBApplicationController::addCapturedEmbedCode(const QString& embedCode)
{
    if (!embedCode.isEmpty())
    {
        showBoard();

        const QString userWidgetPath = WBSettings::settings()->userInteractiveDirectory() + "/" + tr("Web");
        QDir userWidgetDir(userWidgetPath);

        int width = 300;
        int height = 150;

        QString widgetPath = WBGraphicsW3CWidgetItem::createHtmlWrapperInDir(embedCode, userWidgetDir,
                QSize(width, height), WBStringUtils::toCanonicalUuid(QUuid::createUuid()));

        if (widgetPath.length() > 0)
            WBApplication::boardController->downloadURL(QUrl::fromLocalFile(widgetPath));
    }
}


void WBApplicationController::showBoard()
{
    mMainWindow->webToolBar->hide();
    mMainWindow->documentToolBar->hide();
    mMainWindow->boardToolBar->show();

    if (mMainMode == Document)
    {
        int selectedSceneIndex = WBApplication::documentController->getSelectedItemIndex();
        if (selectedSceneIndex != -1)
        {
            WBApplication::boardController->setActiveDocumentScene(WBApplication::documentController->selectedDocument(), selectedSceneIndex, true);
        }
    }

    mMainMode = Board;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToBoardWidget();

    if (WBApplication::boardController)
        WBApplication::boardController->show();

    mIsShowingDesktop = false;
    WBPlatformUtils::setDesktopMode(false);

    mUninoteController->hideWindow();

    WBPlatformUtils::showFullScreen(mMainWindow);

    emit mainModeChanged(Board);

    WBApplication::boardController->freezeW3CWidgets(false);
}


void WBApplicationController::showInternet()
{

    if (WBApplication::boardController)
    {
        WBApplication::boardController->persistCurrentScene();
        WBApplication::boardController->hide();
    }

    if (WBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        showDesktop(true);
        WBApplication::webController->show();
    }
    else
    {
        mMainWindow->boardToolBar->hide();
        mMainWindow->documentToolBar->hide();
        mMainWindow->webToolBar->show();

        mMainMode = Internet;

        adaptToolBar();

        mMainWindow->show();
        mUninoteController->hideWindow();

        WBApplication::webController->show();

        emit mainModeChanged(Internet);
    }
}


void WBApplicationController::showDocument()
{
    mMainWindow->webToolBar->hide();
    mMainWindow->boardToolBar->hide();
    mMainWindow->documentToolBar->show();

    mMainMode = Document;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToDocumentsWidget();

    if (WBApplication::boardController)
    {
        if (WBApplication::boardController->activeScene()->isModified())
            WBApplication::boardController->persistCurrentScene();
        WBApplication::boardController->hide();
    }

    if (WBApplication::documentController)
    {
        emit WBApplication::documentController->reorderDocumentsRequested();
        WBApplication::documentController->show();
    }

    mMainWindow->show();

    mUninoteController->hideWindow();

    emit mainModeChanged(Document);
}

void WBApplicationController::showDesktop(bool dontSwitchFrontProcess)
{
    int desktopWidgetIndex = qApp->desktop()->screenNumber(mMainWindow);

    if (WBApplication::boardController)
        WBApplication::boardController->hide();

    mMainWindow->hide();
    mUninoteController->showWindow();

    if (mMirror)
    {
        QRect rect = qApp->desktop()->screenGeometry(desktopWidgetIndex);
        mMirror->setSourceRect(rect);
    }

    mIsShowingDesktop = true;
    emit desktopMode(true);

    if (!dontSwitchFrontProcess) {
        WBPlatformUtils::bringPreviousProcessToFront();
    }

    WBDrawingController::drawingController()->setInDestopMode(true);
    WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
}

void WBApplicationController::checkUpdate(const QUrl& url)
{
    QUrl jsonUrl = url;
    if (url.isEmpty())
        jsonUrl = WBSettings::settings()->appSoftwareUpdateURL->get().toUrl();

    qDebug() << "Checking for update at url: " << jsonUrl.toString();

    connect(mNetworkAccessManager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(updateRequestFinished(QNetworkReply*)));

    mNetworkAccessManager->get(QNetworkRequest(jsonUrl));

}

void WBApplicationController::updateRequestFinished(QNetworkReply * reply)
{
    if (reply->error()) {
        qWarning() << "Error downloading update file: " << reply->errorString();
        return;
    }

    // Check if we are being redirected. If so, call checkUpdate again

    QVariant redirect_target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirect_target.isNull()) {
        // The returned URL might be relative. resolved() creates an absolute url from it
        QUrl redirect_url(reply->url().resolved(redirect_target.toUrl()));

        checkUpdate(redirect_url);
        return;
    }

    // No error and no redirect => we read the whole response

    QString responseString = QString(reply->readAll());

    if (!responseString.isEmpty() &&
            responseString.contains("version") &&
            responseString.contains("url")) {

        reply->close();
        reply->deleteLater();

        downloadJsonFinished(responseString);
    }
}


void WBApplicationController::downloadJsonFinished(QString currentJson)
{
    QJSValue scriptValue;
    QJSEngine scriptEngine;
    scriptValue = scriptEngine.evaluate ("(" + currentJson + ")");

    WBVersion installedVersion (qApp->applicationVersion());
    WBVersion jsonVersion (scriptValue.property("version").toString());

    qDebug() << "json version: " << jsonVersion.toUInt();
    qDebug() << "installed version: " << installedVersion.toUInt();

    if (jsonVersion > installedVersion) {
        if (WBApplication::mainWindow->yesNoQuestion(tr("Update available"), tr ("New update available, would you go to the web page ?"))){
            QUrl url(scriptValue.property("url").toString());
            QDesktopServices::openUrl(url);
        }
    }
    else if (isNoUpdateDisplayed) {
        mMainWindow->information(tr("Update"), tr("No update available"));
    }
}

void WBApplicationController::checkAtLaunch()
{
    mOpenSankoreImporter = new WBOpenSankoreImporter(mMainWindow->centralWidget());

    if(WBSettings::settings()->appEnableAutomaticSoftwareUpdates->get().toBool()){
        isNoUpdateDisplayed = false;
        checkUpdate();
    }
}

void WBApplicationController::checkUpdateRequest()
{
    isNoUpdateDisplayed = true;
    checkUpdate();
}

void WBApplicationController::hideDesktop()
{
    if (mMainMode == Board)
    {
        showBoard();
    }
    else if (mMainMode == Internet)
    {
        showInternet();
    }
    else if (mMainMode == Document)
    {
        showDocument();
    }

    mIsShowingDesktop = false;

    mDisplayManager->adjustScreens(-1);

    emit desktopMode(false);
}

void WBApplicationController::setMirrorSourceWidget(QWidget* pWidget)
{
    if (mMirror)
    {
        mMirror->setSourceWidget(pWidget);
    }
}

void WBApplicationController::mirroringEnabled(bool enabled)
{
    if (mMirror)
    {
        if (enabled)
        {
            mMirror->start();
            mDisplayManager->setDisplayWidget(mMirror);

        }
        else
        {
            mDisplayManager->setDisplayWidget(mDisplayView);
            mMirror->stop();
        }

        mMirror->setVisible(enabled && mDisplayManager->numScreens() > 1);
        mUninoteController->updateShowHideState(enabled);
        WBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(enabled);
    }
    else
    {
        mDisplayManager->setDisplayWidget(mDisplayView);
    }
}

void WBApplicationController::closing()
{
    if (mMirror)
        mMirror->stop();

    if (mUninoteController)
    {
        mUninoteController->hideWindow();
        mUninoteController->close();
    }

    /*

    if (WBApplication::documentController)
        WBApplication::documentController->closing();

    */

    WBPersistenceManager::persistenceManager()->closing(); // ALTI/AOU - 20140616 : to update the file "documents/folders.xml"
}


void WBApplicationController::showMessage(const QString& message, bool showSpinningWheel)
{
    if (!WBApplication::closingDown())
    {
        if (mMainMode == Document)
        {
            WBApplication::boardController->hideMessage();
            WBApplication::documentController->showMessage(message, showSpinningWheel);
        }
        else
        {
            WBApplication::documentController->hideMessage();
            WBApplication::boardController->showMessage(message, showSpinningWheel);
        }
    }
}

void WBApplicationController::importFile(const QString& pFilePath)
{
    const QFile fileToOpen(pFilePath);

    if (!fileToOpen.exists())
        return;

    WBDocumentProxy* document = 0;

    bool success = false;

    document = WBDocumentManager::documentManager()->importFile(fileToOpen, "");

    success = (document != 0);

    if (success && document)
    {
        if (mMainMode == Board || mMainMode == Internet)
        {
            if (WBApplication::boardController)
            {
                WBApplication::boardController->setActiveDocumentScene(document, 0);
                showBoard();
            }
        }
        else if (mMainMode == Document)
        {
            if (WBApplication::documentController)
                WBApplication::documentController->selectDocument(document);
        }
    }
}

void WBApplicationController::useMultiScreen(bool use)
{
    if (use && !mMirror)
        mMirror = new WBScreenMirror();
    if (!use && mMirror) {
        delete mMirror;
        mMirror = NULL;
    }

    mDisplayManager->setUseMultiScreen(use);
    mDisplayManager->adjustScreens(0);
    WBSettings::settings()->appUseMultiscreen->set(use);

}

QStringList WBApplicationController::widgetInlineJavaScripts()
{
    QString scriptDirPath = WBPlatformUtils::applicationResourcesDirectory() + "/widget-inline-js";
    QDir scriptDir(scriptDirPath);

    QStringList scripts;

    if (scriptDir.exists())
    {
        QStringList files = scriptDir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

        foreach(QString file, files)
        {
            QFile scriptFile(scriptDirPath + "/" + file);
            if (file.endsWith(".js") && scriptFile.open(QIODevice::ReadOnly))
            {
                QString s = QString::fromUtf8(scriptFile.readAll());

                if (s.length() > 0)
                    scripts << s;

            }
        }
    }

    qSort(scripts);

    return scripts;
}


void WBApplicationController::actionCut()
{
    if (!WBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            WBApplication::boardController->cut();
        }
        else if(mMainMode == Document)
        {
            WBApplication::documentController->cut();
        }
        else if(mMainMode == Internet)
        {
            WBApplication::webController->cut();
        }
    }
}


void WBApplicationController::actionCopy()
{
    if (!WBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            WBApplication::boardController->copy();
        }
        else if(mMainMode == Document)
        {
            WBApplication::documentController->copy();
        }
        else if(mMainMode == Internet)
        {
            WBApplication::webController->copy();
        }
    }
}


void WBApplicationController::actionPaste()
{
    if (!WBApplication::closingDown())
    {
        if (mMainMode == Board)
        {
            WBApplication::boardController->paste();
        }
        else if (mMainMode == Document)
        {
            WBApplication::documentController->paste();
        }
        else if(mMainMode == Internet)
        {
            WBApplication::webController->paste();
        }
    }
}
