#include "WBApplication.h"

#include <QtWidgets>
#include <QtWebEngine>
#include <QtXml>
#include <QFontDatabase>
#include <QStyleFactory>

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBStringUtils.h"

#include "WBSettings.h"
#include "WBSetting.h"
#include "WBPersistenceManager.h"
#include "WBDocumentManager.h"
#include "WBPreferencesController.h"
#include "WBIdleTimer.h"
#include "WBApplicationController.h"

#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"
#include "board/WBBoardView.h"
#include "board/WBBoardPaletteManager.h"
#include "web/WBWebController.h"

#include "document/WBDocumentController.h"
#include "document/WBDocumentProxy.h"

#include "gui/WBMainWindow.h"
#include "gui/WBResources.h"
#include "gui/WBThumbnailWidget.h"

#include "ui_mainWindow.h"

#include "frameworks/WBCryptoUtils.h"
#include "tools/WBToolsManager.h"

#include "WBDisplayManager.h"
#include "core/memcheck.h"

QPointer<QUndoStack> WBApplication::undoStack;

WBApplicationController* WBApplication::applicationController = 0;
WBBoardController* WBApplication::boardController = 0;
WBWebController* WBApplication::webController = 0;
WBDocumentController* WBApplication::documentController = 0;

WBMainWindow* WBApplication::mainWindow = 0;

const QString WBApplication::mimeTypeUniboardDocument = QString("application/vnd.mnemis-uniboard-document");
const QString WBApplication::mimeTypeUniboardPage = QString("application/vnd.mnemis-uniboard-page");
const QString WBApplication::mimeTypeUniboardPageItem =  QString("application/vnd.mnemis-uniboard-page-item");
const QString WBApplication::mimeTypeUniboardPageThumbnail = QString("application/vnd.mnemis-uniboard-thumbnail");

#if defined(Q_OS_OSX) || defined(Q_OS_LINUX)
bool bIsMinimized = false;
#endif

QObject* WBApplication::staticMemoryCleaner = 0;


WBApplication::WBApplication(const QString &id, int &argc, char **argv) : QtSingleApplication(id, argc, argv)
  , mPreferencesController(NULL)
  , mApplicationTranslator(NULL)
  , mQtGuiTranslator(NULL)
{

    staticMemoryCleaner = new QObject(0); // deleted in WBApplication destructor

    setOrganizationName("Open Education Foundation");
    setOrganizationDomain("oe-f.org");
    setApplicationName("WBoard");

	QString version = "V1.1.0";//UBVERSION;
    if(version.endsWith("."))
        version = version.left(version.length()-1);
    setApplicationVersion(version);

    QStringList args = arguments();

    mIsVerbose = args.contains("-v")
        || args.contains("-verbose")
        || args.contains("verbose")
        || args.contains("-log")
        || args.contains("log");


    setupTranslators(args);

    WBResources::resources();

    if (!undoStack)
        undoStack = new QUndoStack(staticMemoryCleaner);

    WBPlatformUtils::init();

    WBSettings *settings = WBSettings::settings();

    connect(settings->appToolBarPositionedAtTop, SIGNAL(changed(QVariant)), this, SLOT(toolBarPositionChanged(QVariant)));
    connect(settings->appToolBarDisplayText, SIGNAL(changed(QVariant)), this, SLOT(toolBarDisplayTextChanged(QVariant)));
    updateProtoActionsState();

#ifndef Q_OS_OSX
    setWindowIcon(QIcon(":/images/WBoard.png"));
#endif

    setStyle("fusion");

	QString css = WBFileSystemUtils::readTextFile(WBPlatformUtils::applicationResourcesDirectory() + "/etc/"+ qApp->applicationName()+".css");
    if (css.length() > 0)
        setStyleSheet(css);

    QApplication::setStartDragDistance(8); // default is 4, and is a bit small for tablets

    installEventFilter(this);

}


WBApplication::~WBApplication()
{
    WBPlatformUtils::destroy();

    WBFileSystemUtils::deleteAllTempDirCreatedDuringSession();

    delete mainWindow;
    mainWindow = 0;

    WBPersistenceManager::destroy();

    WBDownloadManager::destroy();

    WBDrawingController::destroy();

    WBSettings::destroy();

    WBCryptoUtils::destroy();

    WBToolsManager::destroy();

    if(mApplicationTranslator != NULL){
        delete mApplicationTranslator;
        mApplicationTranslator = NULL;
    }
    if(mQtGuiTranslator!=NULL){
        delete mQtGuiTranslator;
        mQtGuiTranslator = NULL;
    }

    delete staticMemoryCleaner;
    staticMemoryCleaner = 0;
}

QString WBApplication::checkLanguageAvailabilityForSankore(QString &language)
{
    QStringList availableTranslations = WBPlatformUtils::availableTranslations();

    if(availableTranslations.contains(language,Qt::CaseInsensitive))
        return language;
    else{
        if(language.length() > 2){
            QString shortLanguageCode = language.left(2);

            foreach (const QString &str, availableTranslations) {
                       if (str.contains(shortLanguageCode))
                           return shortLanguageCode;
                   }

        }
    }
    return QString("");
}

void WBApplication::setupTranslators(QStringList args)
{
    QString forcedLanguage("");
    if(args.contains("-lang"))
        forcedLanguage=args.at(args.indexOf("-lang") + 1);
// TODO claudio: this has been commented because some of the translation seem to be loaded at this time
//               especially tools name. This is a workaround and we have to be able to load settings without
//               impacting the translations
//    else{
//        QString setLanguage = WBSettings::settings()->appPreferredLanguage->get().toString();
//        if(!setLanguage.isEmpty())
//            forcedLanguage = setLanguage;
//    }

    QString language("");

    if(!forcedLanguage.isEmpty())
        language = checkLanguageAvailabilityForSankore(forcedLanguage);

    if(language.isEmpty()){
        QString systemLanguage = WBPlatformUtils::systemLanguage();
        language = checkLanguageAvailabilityForSankore(systemLanguage);
    }

    if(language.isEmpty()){
        language = "zh_CN";
        //fallback if no translation are available
    }
    else{
        mApplicationTranslator = new QTranslator(this);
        mQtGuiTranslator = new QTranslator(this);
        mApplicationTranslator->load(WBPlatformUtils::translationPath(QString("WBoard_"),language));
        installTranslator(mApplicationTranslator);

        QString qtGuiTranslationPath = WBPlatformUtils::translationPath("qt_", language);


        if(!QFile(qtGuiTranslationPath).exists()){
            qtGuiTranslationPath = WBPlatformUtils::translationPath("qt_", language.left(2));
            if(!QFile(qtGuiTranslationPath).exists())
                qtGuiTranslationPath = "";
        }

        if(!qtGuiTranslationPath.isEmpty()){
            mQtGuiTranslator->load(qtGuiTranslationPath);
            installTranslator(mQtGuiTranslator);
        }
        else
            qDebug() << "Qt gui translation in " << language << " is not available";
    }

    QLocale::setDefault(QLocale(language));
    qDebug() << "Running application in:" << language;
}

int WBApplication::exec(const QString& pFileToImport)
{
    QPixmapCache::setCacheLimit(1024 * 100);

    QString webDbPath = WBSettings::userDataDirectory() + "/web-databases";
    QDir webDbDir(webDbPath);
    if (!webDbDir.exists(webDbPath))
        webDbDir.mkpath(webDbPath);

	//QWebEngineSettings::setIconDatabasePath(webDbPath);
 //   QWebEngineSettings::setOfflineStoragePath (webDbPath);

    QWebEngineSettings *gs = QWebEngineSettings::globalSettings();
    gs->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    gs->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    gs->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    gs->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    gs->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, true);
    gs->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    gs->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);


    mainWindow = new WBMainWindow(0, Qt::FramelessWindowHint); 
    mainWindow->setAttribute(Qt::WA_NativeWindow, true);

    mainWindow->actionCopy->setShortcuts(QKeySequence::Copy);
    mainWindow->actionPaste->setShortcuts(QKeySequence::Paste);
    mainWindow->actionCut->setShortcuts(QKeySequence::Cut);

    WBThumbnailUI::_private::initCatalog();

    connect(mainWindow->actionBoard, SIGNAL(triggered()), this, SLOT(showBoard()));
    connect(mainWindow->actionWeb, SIGNAL(triggered()), this, SLOT(showInternet()));
    connect(mainWindow->actionWeb, SIGNAL(triggered()), this, SLOT(stopScript()));
    connect(mainWindow->actionDocument, SIGNAL(triggered()), this, SLOT(showDocument()));
    connect(mainWindow->actionDocument, SIGNAL(triggered()), this, SLOT(stopScript()));
    connect(mainWindow->actionQuit, SIGNAL(triggered()), this, SLOT(closing()));
    connect(mainWindow, SIGNAL(closeEvent_Signal(QCloseEvent*)), this, SLOT(closeEvent(QCloseEvent*)));

    boardController = new WBBoardController(mainWindow);
    boardController->init();

    webController = new WBWebController(mainWindow);
    documentController = new WBDocumentController(mainWindow);

    WBDrawingController::drawingController()->setStylusTool((int)WBStylusTool::Pen);

    applicationController = new WBApplicationController(boardController->controlView(),
                                                        boardController->displayView(),
                                                        mainWindow,
                                                        staticMemoryCleaner,
                                                        boardController->paletteManager()->rightPalette());


    connect(applicationController, SIGNAL(mainModeChanged(WBApplicationController::MainMode)),
            boardController->paletteManager(), SLOT(slot_changeMainMode(WBApplicationController::MainMode)));

    connect(applicationController, SIGNAL(desktopMode(bool)),
            boardController->paletteManager(), SLOT(slot_changeDesktopMode(bool)));

    connect(applicationController, SIGNAL(mainModeChanged(WBApplicationController::MainMode))
          , boardController,       SLOT(appMainModeChanged(WBApplicationController::MainMode)));

    connect(mainWindow->actionDesktop, SIGNAL(triggered(bool)), applicationController, SLOT(showDesktop(bool)));
    connect(mainWindow->actionDesktop, SIGNAL(triggered(bool)), this, SLOT(stopScript()));
#if defined(Q_OS_OSX) || defined(Q_OS_LINUX)
    connect(mainWindow->actionHideApplication, SIGNAL(triggered()), this, SLOT(showMinimized()));
#else
    connect(mainWindow->actionHideApplication, SIGNAL(triggered()), mainWindow, SLOT(showMinimized()));
#endif

    mPreferencesController = new WBPreferencesController(mainWindow);

    connect(mainWindow->actionPreferences, SIGNAL(triggered()), mPreferencesController, SLOT(show()));
    connect(mainWindow->actionCheckUpdate, SIGNAL(triggered()), applicationController, SLOT(checkUpdateRequest()));


    toolBarPositionChanged(WBSettings::settings()->appToolBarPositionedAtTop->get());

    bool bUseMultiScreen = WBSettings::settings()->appUseMultiscreen->get().toBool();
    mainWindow->actionMultiScreen->setChecked(bUseMultiScreen);
    connect(mainWindow->actionMultiScreen, SIGNAL(triggered(bool)), applicationController, SLOT(useMultiScreen(bool)));
    connect(mainWindow->actionWidePageSize, SIGNAL(triggered(bool)), boardController, SLOT(setWidePageSize(bool)));
    connect(mainWindow->actionRegularPageSize, SIGNAL(triggered(bool)), boardController, SLOT(setRegularPageSize(bool)));

    connect(mainWindow->actionCut, SIGNAL(triggered()), applicationController, SLOT(actionCut()));
    connect(mainWindow->actionCopy, SIGNAL(triggered()), applicationController, SLOT(actionCopy()));
    connect(mainWindow->actionPaste, SIGNAL(triggered()), applicationController, SLOT(actionPaste()));

    applicationController->initScreenLayout(bUseMultiScreen);
    boardController->setupLayout();

    if (pFileToImport.length() > 0)
        WBApplication::applicationController->importFile(pFileToImport);

    if (WBSettings::settings()->appStartMode->get().toInt())
        applicationController->showDesktop();
    else
        applicationController->showBoard();

    emit WBDrawingController::drawingController()->colorPaletteChanged();

    onScreenCountChanged(1);
    connect(desktop(), SIGNAL(screenCountChanged(int)), this, SLOT(onScreenCountChanged(int)));
    return QApplication::exec();
}

void WBApplication::onScreenCountChanged(int newCount)
{
    Q_UNUSED(newCount);
    WBDisplayManager displayManager;
    mainWindow->actionMultiScreen->setEnabled(displayManager.numScreens() > 1);
}

void WBApplication::showMinimized()
{
#ifdef Q_OS_OSX
    mainWindow->hide();
    bIsMinimized = true;
#elif defined(Q_OS_LINUX)
    mainWindow->showMinimized();
    bIsMinimized = true;
#endif

}


void WBApplication::startScript()
{
    this->boardController->freezeW3CWidgets(false);
}

void WBApplication::stopScript()
{
    this->boardController->freezeW3CWidgets(true);
}

void WBApplication::showBoard()
{
    applicationController->showBoard();
}

void WBApplication::showInternet()
{
    applicationController->showInternet();
    webController->showTabAtTop(true);
}

void WBApplication::showDocument()
{
    applicationController->showDocument();
}

int WBApplication::toolBarHeight()
{
    return mainWindow->boardToolBar->rect().height();
}


void WBApplication::toolBarPositionChanged(QVariant topOrBottom)
{
    Qt::ToolBarArea area;

    if (topOrBottom.toBool())
        area = Qt::TopToolBarArea;
    else
        area = Qt::BottomToolBarArea;

    mainWindow->addToolBar(area, mainWindow->boardToolBar);
    mainWindow->addToolBar(area, mainWindow->webToolBar);
    mainWindow->addToolBar(area, mainWindow->documentToolBar);

    webController->showTabAtTop(topOrBottom.toBool());

}


void WBApplication::toolBarDisplayTextChanged(QVariant display)
{
    Qt::ToolButtonStyle toolButtonStyle = display.toBool() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;
    mainWindow->boardToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->webToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->documentToolBar->setToolButtonStyle(toolButtonStyle);
}


void WBApplication::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    closing();
}

void WBApplication::closing()
{
    if (WBSettings::settings()->emptyTrashForOlderDocuments->get().toBool())
    {
        WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        documentController->deleteDocumentsInFolderOlderThan(docModel->trashIndex(), WBSettings::settings()->emptyTrashDaysValue->get().toInt());
        if (docModel->hasChildren(docModel->trashIndex()))
            documentController->deleteEmptyFolders(docModel->trashIndex());
    }

    if (boardController)
        boardController->closing();

    if (applicationController)
        applicationController->closing();

    if (webController)
        webController->closing();

    WBSettings::settings()->closing();

    WBSettings::settings()->appToolBarPositionedAtTop->set(mainWindow->toolBarArea(mainWindow->boardToolBar) == Qt::TopToolBarArea);

    quit();
}


void WBApplication::showMessage(const QString& message, bool showSpinningWheel)
{
    if (applicationController)
        applicationController->showMessage(message, showSpinningWheel);
}


void WBApplication::setDisabled(bool disable)
{
    boardController->setDisabled(disable);
}


void WBApplication::decorateActionMenu(QAction* action)
{
    foreach(QWidget* menuWidget,  action->associatedWidgets())
    {
        QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

        if (tb && !tb->menu())
        {
            tb->setObjectName("ubButtonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu* menu = new QMenu(mainWindow);

            QActionGroup* pageSizeGroup = new QActionGroup(mainWindow);
            pageSizeGroup->addAction(mainWindow->actionWidePageSize);
            pageSizeGroup->addAction(mainWindow->actionRegularPageSize);
            pageSizeGroup->addAction(mainWindow->actionCustomPageSize);

            QMenu* documentSizeMenu = menu->addMenu(QIcon(":/images/toolbar/pageSize.png"),tr("Page Size"));
            documentSizeMenu->addAction(mainWindow->actionWidePageSize);
            documentSizeMenu->addAction(mainWindow->actionRegularPageSize);
            documentSizeMenu->addAction(mainWindow->actionCustomPageSize);
            menu->addAction(mainWindow->actionCut);
            menu->addAction(mainWindow->actionCopy);
            menu->addAction(mainWindow->actionPaste);
            menu->addAction(mainWindow->actionHideApplication);
            menu->addAction(mainWindow->actionSleep);

            menu->addSeparator();
            menu->addAction(mainWindow->actionPreferences);
            menu->addAction(mainWindow->actionMultiScreen);
            if (!WBSettings::settings()->appHideCheckForSoftwareUpdate->get().toBool())
                menu->addAction(mainWindow->actionCheckUpdate);
            menu->addSeparator();

            menu->addAction(mainWindow->actionPodcast);
            mainWindow->actionPodcast->setText(tr("Podcast"));

            menu->addSeparator();
            menu->addAction(mainWindow->actionQuit);

            tb->setMenu(menu);
        }
    }
}


void WBApplication::updateProtoActionsState()
{
    if (mainWindow)
    {
        mainWindow->actionMultiScreen->setVisible(true);
    }

    foreach(QMenu* protoMenu, mProtoMenus)
        protoMenu->setVisible(true);

}


void WBApplication::insertSpaceToToolbarBeforeAction(QToolBar* toolbar, QAction* action, int width)
{
    QWidget* spacer = new QWidget();

    if (width >= 0){
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addSpacing(width);
        spacer->setLayout(layout);
    }
    else
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    toolbar->insertWidget(action, spacer);
}


bool WBApplication::eventFilter(QObject *obj, QEvent *event)
{
    bool result = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *fileToOpenEvent = static_cast<QFileOpenEvent *>(event);

        WBPlatformUtils::setFrontProcess();

        applicationController->importFile(fileToOpenEvent->file());
    }

    if (event->type() == QEvent::TabletLeaveProximity)
    {
        if (boardController && boardController->controlView())
            boardController->controlView()->forcedTabletRelease();
    }


    if (event->type() == QEvent::ApplicationActivate)
    {
        boardController->controlView()->setMultiselection(false);

#if defined(Q_OS_OSX)
        if (bIsMinimized) {
            if (mainWindow->isHidden())
                mainWindow->show();
            bIsMinimized = false;
        }
#elif defined(Q_OS_LINUX)
        if (bIsMinimized) {
            bIsMinimized = false;
            WBPlatformUtils::showFullScreen(mainWindow);
        }
#endif
    }

    return result;
}


bool WBApplication::handleOpenMessage(const QString& pMessage)
{
    qDebug() << "received message" << pMessage;

    if (pMessage == WBSettings::appPingMessage)
    {
        qDebug() << "received ping";
        return true;
    }

    qDebug() << "importing file" << pMessage;

    WBApplication::applicationController->importFile(pMessage);

    return true;
}

void WBApplication::cleanup()
{
    if (applicationController) delete applicationController;
    if (boardController) delete boardController;
    if (webController) delete webController;
    if (documentController) delete documentController;

    applicationController = NULL;
    boardController = NULL;
    webController = NULL;
    documentController = NULL;
}

QString WBApplication::urlFromHtml(QString html)
{
    QString _html;
    QRegExp comments("\\<![ \r\n\t]*(--([^\\-]|[\r\n]|-[^\\-])*--[ \r\n\t]*)\\>");
    QString url;
    QDomDocument domDoc;

    //    We remove all the comments & CRLF of this html
    _html = html.remove(comments);
    domDoc.setContent(_html.remove(QRegExp("[\\0]")));
    QDomElement rootElem = domDoc.documentElement();

    //  QUICKFIX: Here we have to check rootElem. Sometimes it can be a <meta> tag
    //  In such a case we will not be able to retrieve the src value
    if(rootElem.tagName().toLower().contains("meta")){
        qDebug() << rootElem.firstChildElement().tagName();
        //  In that case we get the next element
        url = rootElem.firstChildElement().attribute("src");
    }else{
        url = rootElem.attribute("src");
    }

    return url;
}

bool WBApplication::isFromWeb(QString url)
{
    bool res = true;

    if( url.startsWith("wboardtool://") ||
        url.startsWith("file://") ||
        url.startsWith("/")){
        res = false;
    }

    return res;
}

QScreen* WBApplication::controlScreen()
{
    QList<QScreen*> screenList = screens();
    if (screenList.size() == 1)
        return screenList.first();

    return screenList[controlScreenIndex()];
}


int WBApplication::controlScreenIndex()
{
    return applicationController->displayManager()->controleScreenIndex();
}
