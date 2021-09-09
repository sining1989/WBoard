#include "WBBrowserWindow.h"

#include <QtWidgets>
#include <QtWebEngine>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QDesktopWidget>

#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBApplication.h"

#include "network/WBNetworkAccessManager.h"
#include "network/WBAutoSaver.h"

#include "gui/WBMainWindow.h"

//#include "pdf/UBWebPluginPDFWidget.h"


#include "WBChaseWidget.h"
#include "WBDownloadManager_.h"
#include "WBHistory.h"
#include "WBTabWidget.h"
#include "WBToolBarSearch.h"
#include "WBWebView.h"

#include "core/memcheck.h"

WBDownloadManager_ *WBBrowserWindow::sDownloadManager = 0;
WBHistoryManager *WBBrowserWindow::sHistoryManager = 0;


WBBrowserWindow::WBBrowserWindow(QWidget *parent, Ui::MainWindow* uniboardMainWindow)
        : QWidget(parent)
        , mWebToolBar(0)
        //, mSearchToolBar(0)
        , mTabWidget(new WBTabWidget(this))
        //, mSearchAction(0)
        , mUniboardMainWindow(uniboardMainWindow)
{	
    QWebEngineSettings *defaultSettings = QWebEngineSettings::globalSettings();
    defaultSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    defaultSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    setupMenu();
    setupToolBar();


    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);

    layout->addWidget(mTabWidget);

    this->setLayout(layout);

    connect(mTabWidget, SIGNAL(loadPage(const QString &)), this, SLOT(loadPage(const QString &)));


    connect(mTabWidget, SIGNAL(setCurrentTitle(const QString &)), this, SLOT(slotUpdateWindowTitle(const QString &)));

    connect(mTabWidget, SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));


    connect(mTabWidget, SIGNAL(loadFinished(bool)), this, SIGNAL(activeViewPageChanged()));

    connect(mTabWidget, SIGNAL(geometryChangeRequested(const QRect &)), this, SLOT(geometryChangeRequested(const QRect &)));

    slotUpdateWindowTitle();


    mTabWidget->newTab();

    connect(mTabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabCurrentChanged(int)));

}


WBBrowserWindow::~WBBrowserWindow()
{
    if(mTabWidget){
        delete mTabWidget;
        mTabWidget = NULL;
    }
}

UBCookieJar *WBBrowserWindow::cookieJar()
{
    return (UBCookieJar*)WBNetworkAccessManager::defaultAccessManager()->cookieJar();
}

WBDownloadManager_ *WBBrowserWindow::downloadManager()
{
    if (!sDownloadManager) {
        sDownloadManager = new WBDownloadManager_(WBApplication::mainWindow);
    }
    return sDownloadManager;
}

//WBHistoryManager *WBBrowserWindow::historyManager()
//{
//    if (!sHistoryManager) {
//        sHistoryManager = new WBHistoryManager();
//        //QWebHistoryInterface::setDefaultInterface(sHistoryManager);
//    }
//    return sHistoryManager;
//}

QSize WBBrowserWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry(WBApplication::controlScreenIndex());
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void WBBrowserWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(slotSwapFocus()));
}

void WBBrowserWindow::setupToolBar()
{
    mWebToolBar = mUniboardMainWindow->webToolBar;

    mTabWidget->addWebAction(mUniboardMainWindow->actionWebBack, QWebEnginePage::Back);
    mTabWidget->addWebAction(mUniboardMainWindow->actionWebForward, QWebEnginePage::Forward);
    mTabWidget->addWebAction(mUniboardMainWindow->actionWebReload, QWebEnginePage::Reload);
    mTabWidget->addWebAction(mUniboardMainWindow->actionStopLoading, QWebEnginePage::Stop);

    mHistoryBackMenu = new QMenu(this);
    connect(mHistoryBackMenu, SIGNAL(aboutToShow()),this, SLOT(aboutToShowBackMenu()));
    connect(mHistoryBackMenu, SIGNAL(triggered(QAction *)), this, SLOT(openActionUrl(QAction *)));

    //foreach (QWidget* menuWidget,  mUniboardMainWindow->actionWebBack->associatedWidgets())
    //{
    //    QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

    //    if (tb && !tb->menu())
    //    {
    //        tb->setMenu(mHistoryBackMenu);
    //       // tb->setStyleSheet("QToolButton::menu-indicator { subcontrol-position: bottom left; padding-left: 8px;}");
    //    }
    //}

    mHistoryForwardMenu = new QMenu(this);
    connect(mHistoryForwardMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowForwardMenu()));
    connect(mHistoryForwardMenu, SIGNAL(triggered(QAction *)), this, SLOT(openActionUrl(QAction *)));

    //foreach (QWidget* menuWidget,  mUniboardMainWindow->actionWebForward->associatedWidgets())
    //{
    //    QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

    //    if (tb && !tb->menu())
    //    {
    //        tb->setMenu(mHistoryForwardMenu);
    //        //tb->setStyleSheet("QToolButton { padding-right: 8px; }");
    //    }
    //}

    mWebToolBar->insertWidget(mUniboardMainWindow->actionWebBigger, mTabWidget->lineEditStack());

    //mSearchToolBar = new WBToolbarSearch(mWebToolBar);

    //mSearchAction = mWebToolBar->insertWidget(mUniboardMainWindow->actionWebBigger, mSearchToolBar);

    //connect(mSearchToolBar, SIGNAL(search(const QUrl&)), SLOT(loadUrl(const QUrl&)));

    mChaseWidget = new WBChaseWidget(this);
    mWebToolBar->insertWidget(mUniboardMainWindow->actionWebBigger, mChaseWidget);
    mWebToolBar->insertSeparator(mUniboardMainWindow->actionWebBigger);

    connect(mUniboardMainWindow->actionHome, SIGNAL(triggered()), this , SLOT(slotHome()));

    connect(mUniboardMainWindow->actionBookmarks, SIGNAL(triggered()), this , SLOT(bookmarks()));
    connect(mUniboardMainWindow->actionAddBookmark, SIGNAL(triggered()), this , SLOT(addBookmark()));
    connect(mUniboardMainWindow->actionWebBigger, SIGNAL(triggered()), this , SLOT(slotViewZoomIn()));
    connect(mUniboardMainWindow->actionWebSmaller, SIGNAL(triggered()), this , SLOT(slotViewZoomOut()));

    mWebToolBar->show();
}


//void WBBrowserWindow::adaptToolBar(bool wideRes)
//{
//    if (mSearchAction)
//    {
//        mSearchAction->setVisible(wideRes);
//    }
//}

QUrl WBBrowserWindow::guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // Check if it looks like a qualified URL. Try parsing it and see.
    bool hasSchema = test.exactMatch(urlStr);
    if (hasSchema)
    {
        int dotCount = urlStr.count(".");

        if (dotCount == 0 && !urlStr.contains(".com"))
        {
            urlStr += ".com";
        }

        QUrl url = QUrl::fromEncoded(urlStr.toUtf8(), QUrl::TolerantMode);
        if (url.isValid())
        {
            return url;
        }
    }

    // Might be a file.
    if (QFile::exists(urlStr))
    {
        QFileInfo info(urlStr);
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }

    // Might be a shorturl - try to detect the schema.
    if (!hasSchema)
    {
        QString schema = "http";

        QString guessed = schema + "://" + urlStr;

        int dotCount = guessed.count(".");

        if (dotCount == 0 && !urlStr.contains(".com"))
        {
            guessed += ".com";
        }

        QUrl url = QUrl::fromEncoded(guessed.toUtf8(), QUrl::TolerantMode);

        if (url.isValid())
            return url;
    }

    // Fall back to QUrl's own tolerant parser.
    QUrl url = QUrl::fromEncoded(string.toUtf8(), QUrl::TolerantMode);

    // finally for cases where the user just types in a hostname add http
    if (url.scheme().isEmpty())
        url = QUrl::fromEncoded("http://" + string.toUtf8(), QUrl::TolerantMode);

    return url;
}

void WBBrowserWindow::loadUrl(const QUrl &url)
{
    if (!currentTabWebView() || !url.isValid())
        return;

    mTabWidget->currentLineEdit()->setText(url.toString());
    mTabWidget->loadUrlInCurrentTab(url);
}

void WBBrowserWindow::loadUrlInNewTab(const QUrl &url)
{
    if (!url.isValid())
        return;

    mTabWidget->newTab();
    loadUrl(url);
}

WBWebView* WBBrowserWindow::createNewTab()
{
    return mTabWidget->newTab();
}

void WBBrowserWindow::slotSelectLineEdit()
{
    mTabWidget->currentLineEdit()->selectAll();
    mTabWidget->currentLineEdit()->setFocus();
}

void WBBrowserWindow::slotFileSaveAs()
{
    downloadManager()->download(currentTabWebView()->url(), true);
}

void WBBrowserWindow::slotUpdateStatusbar(const QString &string)
{
    Q_UNUSED(string);
}

void WBBrowserWindow::slotUpdateWindowTitle(const QString &title)
{
    Q_UNUSED(title);
}

void WBBrowserWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    deleteLater();
}

void WBBrowserWindow::slotViewZoomIn()
{
    WBWebView *currentWebView = currentTabWebView();

    if (!currentWebView)
        return;

    QWidget *view = currentWebView->page()->view();

    //UBWebPluginPDFWidget *pdfPluginWidget = view ? view->findChild<UBWebPluginPDFWidget *>("PDFWebPluginWidget") : 0;
    //if (pdfPluginWidget)
    //{
    //    pdfPluginWidget->zoomIn();
    //}
    //else
    {
        currentWebView->setZoomFactor(currentWebView->zoomFactor() + 0.1);
    }
}

void WBBrowserWindow::slotViewZoomOut()
{
    WBWebView *currentWebView = currentTabWebView();

    if (!currentWebView)
        return;

    QWidget *view = currentWebView->page()->view();

    //UBWebPluginPDFWidget *pdfPluginWidget = view ? view->findChild<UBWebPluginPDFWidget *>("PDFWebPluginWidget") : 0;

    //if (pdfPluginWidget)
    //{
    //    pdfPluginWidget->zoomOut();
    //}
    //else
    {
        currentWebView->setZoomFactor(currentWebView->zoomFactor() - 0.1);
    }
}

void WBBrowserWindow::slotViewResetZoom()
{
    if (!currentTabWebView())
        return;
    currentTabWebView()->setZoomFactor(1.0);
}

void WBBrowserWindow::slotViewZoomTextOnly(bool enable)
{
    if (!currentTabWebView())
        return;
    currentTabWebView()->page()->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, enable); //QWebSettings::ZoomTextOnly
}

void WBBrowserWindow::slotHome()
{
    loadPage(WBSettings::settings()->webHomePage->get().toString());
}

void WBBrowserWindow::slotWebSearch()
{
    //mSearchToolBar->lineEdit()->selectAll();
    //mSearchToolBar->lineEdit()->setFocus();
}

void WBBrowserWindow::slotToggleInspector(bool enable)
{
    Q_UNUSED(enable);
}

void WBBrowserWindow::slotSwapFocus()
{
    if (currentTabWebView()->hasFocus())
        mTabWidget->currentLineEdit()->setFocus();
    else
        currentTabWebView()->setFocus();
}

void WBBrowserWindow::loadPage(const QString &page)
{
    QUrl url = guessUrlFromString(page);
    loadUrl(url);
}

WBTabWidget *WBBrowserWindow::tabWidget() const
{
    return mTabWidget;
}

WBWebView *WBBrowserWindow::currentTabWebView() const
{
    return mTabWidget->currentWebView();
}

void WBBrowserWindow::slotLoadProgress(int progress)
{
    if (mChaseWidget)
        mChaseWidget->setAnimated(progress > 0 && progress < 100);
}

void WBBrowserWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

void WBBrowserWindow::tabCurrentChanged(int index)
{
    QWidget* current = mTabWidget->widget(index);

    emit activeViewChange(current);
    emit activeViewPageChanged();
}

void WBBrowserWindow::bookmarks()
{
    loadPage(WBSettings::settings()->webBookmarksPage->get().toString());
}

void WBBrowserWindow::addBookmark()
{
    loadPage(WBSettings::settings()->webAddBookmarkUrl->get().toString() + currentTabWebView()->url().toString());
}

WBWebView* WBBrowserWindow::paintWidget()
{
    return mTabWidget->currentWebView();
}

void WBBrowserWindow::showTabAtTop(bool attop)
{
    if (attop)
        mTabWidget->setTabPosition(QTabWidget::North);
    else
        mTabWidget->setTabPosition(QTabWidget::South);
}

void WBBrowserWindow::aboutToShowBackMenu()
{
    mHistoryBackMenu->clear();
    if (!currentTabWebView())
        return;
    QWebEngineHistory *history = currentTabWebView()->history();

    int historyCount = history->count();
    int historyLimit = history->backItems(historyCount).count() - WBSettings::settings()->historyLimit->get().toReal();
    if (historyLimit < 0)
        historyLimit = 0;

    for (int i = history->backItems(historyCount).count() - 1; i >= historyLimit; --i)
    {
		QWebEngineHistoryItem item = history->backItems(historyCount).at(i);

        QAction *action = new QAction(this);
        action->setData(-1*(historyCount-i-1));

        /*if (!QWebEngineSettings::iconForUrl(item.originalUrl()).isNull())
			action->setIcon(item.icon());*/
        action->setText(item.title().isEmpty() ? item.url().toString() : item.title());
        mHistoryBackMenu->addAction(action);
    }

    mHistoryBackMenu->addSeparator();

    QAction *action = new QAction(this);
    action->setData("clear");
    action->setText("Clear history");
    mHistoryBackMenu->addAction(action);

}

void WBBrowserWindow::aboutToShowForwardMenu()
{
    mHistoryForwardMenu->clear();
    if (!currentTabWebView())
        return;
    QWebEngineHistory *history = currentTabWebView()->history();
    int historyCount = history->count();

    int historyLimit = history->forwardItems(historyCount).count();
    if (historyLimit > WBSettings::settings()->historyLimit->get().toReal())
        historyLimit = WBSettings::settings()->historyLimit->get().toReal();

    for (int i = 0; i < historyLimit; ++i)
    {
		QWebEngineHistoryItem item = history->forwardItems(historyCount).at(i);

        QAction *action = new QAction(this);
        action->setData(historyCount-i);

		/*if (!QWebEngineSettings::iconForUrl(item.originalUrl()).isNull())
				 action->setIcon(item.icon());*/
        action->setText(item.title().isEmpty() ? item.url().toString() : item.title());
        mHistoryForwardMenu->addAction(action);
    }

    mHistoryForwardMenu->addSeparator();

    QAction *action = new QAction(this);
    action->setData("clear");
    action->setText("Clear history");
    mHistoryForwardMenu->addAction(action);
}

void WBBrowserWindow::openActionUrl(QAction *action)
{
    QWebEngineHistory *history = currentTabWebView()->history();

    if (action->data() == "clear")
    {
        history->clear();
        return;
    }

    int offset = action->data().toInt();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first());
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back());
 }
