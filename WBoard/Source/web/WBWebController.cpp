#include <QtWidgets>
#include <QDomDocument>
#include <QXmlQuery>
#include <QWebEnginePage>

#include "frameworks/WBPlatformUtils.h"

#include "WBWebController.h"
#include "WBOEmbedParser.h"

#include "web/browser/WBBrowserWindow.h"
#include "web/browser/WBWebView.h"
#include "web/browser/WBDownloadManager_.h"
#include "web/browser/WBTabWidget.h"

//#include "network/WBServerXMLHttpRequest.h"
#include "network/WBNetworkAccessManager.h"

#include "gui/WBWidgetMirror.h"
#include "gui/WBMainWindow.h"
#include "gui/WBWebToolsPalette.h"
#include "gui/WBKeyboardPalette.h"

#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"

#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"

#include "domain/WBGraphicsScene.h"

#include "desktop/WBCustomCaptureWindow.h"
#include "board/WBBoardPaletteManager.h"

#include "core/memcheck.h"

WBWebController::WBWebController(WBMainWindow* mainWindow)
    : QObject(mainWindow->centralWidget())
    , mMainWindow(mainWindow)
    , mCurrentWebBrowser(0)
    , mBrowserWidget(0)
    , mToolsCurrentPalette(0)
    , mToolsPalettePositionned(false)
    , mDownloadViewIsVisible(false)
{
    connect(&mOEmbedParser, SIGNAL(oembedParsed(QVector<sOEmbedContent>)), this, SLOT(onOEmbedParsed(QVector<sOEmbedContent>)));

    initialiazemOEmbedProviders();

 }


WBWebController::~WBWebController()
{
    // NOOP
}

void WBWebController::initialiazemOEmbedProviders()
{
    mOEmbedProviders << "baidu.com";
}

void WBWebController::webBrowserInstance()
{
    QString webHomePage = WBSettings::settings()->webHomePage->get().toString();
    QUrl currentUrl = WBBrowserWindow::guessUrlFromString(webHomePage);

    if (WBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(currentUrl);
    }
    else
    {
        if (!mCurrentWebBrowser)
        {
            mCurrentWebBrowser = new WBBrowserWindow(mMainWindow->centralWidget(), mMainWindow);

            mMainWindow->addWebWidget(mCurrentWebBrowser);

            connect(mCurrentWebBrowser, SIGNAL(activeViewChange(QWidget*)), this, SLOT(setSourceWidget(QWidget*)));

            WBBrowserWindow::downloadManager()->setParent(mCurrentWebBrowser, Qt::Tool);

            WBApplication::app()->insertSpaceToToolbarBeforeAction(mMainWindow->webToolBar, mMainWindow->actionBoard, 32);
            WBApplication::app()->decorateActionMenu(mMainWindow->actionMenu);

            bool showAddBookmarkButtons = WBSettings::settings()->webShowAddBookmarkButton->get().toBool();
            mMainWindow->actionBookmarks->setVisible(showAddBookmarkButtons);
            mMainWindow->actionAddBookmark->setVisible(showAddBookmarkButtons);

            showTabAtTop(WBSettings::settings()->appToolBarPositionedAtTop->get().toBool());

            adaptToolBar();

            connect(mCurrentWebBrowser, SIGNAL(activeViewPageChanged()), this, SLOT(activePageChanged()));

            mCurrentWebBrowser->loadUrl(currentUrl);

            mCurrentWebBrowser->tabWidget()->tabBar()->show();
            mCurrentWebBrowser->tabWidget()->lineEdits()->show();
        }

        WBApplication::applicationController->setMirrorSourceWidget(mCurrentWebBrowser->paintWidget());
        mMainWindow->switchToWebWidget();

        setupPalettes();
        screenLayoutChanged();

        bool mirroring = WBSettings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();
        WBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(mirroring);
        mToolsCurrentPalette->show();
    }

    if (mDownloadViewIsVisible)
        WBBrowserWindow::downloadManager()->show();
}

void WBWebController::show()
{
    webBrowserInstance();
}

void WBWebController::setSourceWidget(QWidget* pWidget)
{
    mBrowserWidget = pWidget;
    WBApplication::applicationController->setMirrorSourceWidget(pWidget);
}

void WBWebController::activePageChanged()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        mMainWindow->actionWebTrap->setChecked(false);

        QUrl latestUrl = mCurrentWebBrowser->currentTabWebView()->url();

        // TODO : Uncomment the next line to continue the youtube button bugfix
        //WBApplication::mainWindow->actionWebOEmbed->setEnabled(hasEmbeddedContent());
        // And remove this line once the previous one is uncommented
        WBApplication::mainWindow->actionWebOEmbed->setEnabled(isOEmbedable(latestUrl));
        WBApplication::mainWindow->actionEduMedia->setEnabled(isEduMedia(latestUrl));

        emit activeWebPageChanged(mCurrentWebBrowser->currentTabWebView());
    }
}

bool WBWebController::hasEmbeddedContent()
{
    bool bHasContent = false;
    if(mCurrentWebBrowser){
		QString html = "";// mCurrentWebBrowser->currentTabWebView()->webPage()->mainFrame()->toHtml();

        // search the presence of "+oembed"
        QString query = "\\+oembed([^>]*)>";
        QRegExp exp(query);
        exp.indexIn(html);
        QStringList results = exp.capturedTexts();
        if(2 <= results.size() && "" != results.at(1)){
            // An embedded content has been found, no need to check the other ones
            bHasContent = true;
        }else{
            QList<QUrl> contentUrls;
            lookForEmbedContent(&html, "embed", "src", &contentUrls);
            lookForEmbedContent(&html, "video", "src", &contentUrls);
            lookForEmbedContent(&html, "object", "data", &contentUrls);

            // TODO: check the hidden iFrame

            if(!contentUrls.empty()){
                bHasContent = true;
            }
        }
    }

    return bHasContent;
}

QPixmap WBWebController::captureCurrentPage()
{
    QPixmap pix;

  /*  if (mCurrentWebBrowser
            && mCurrentWebBrowser->currentTabWebView()
            && mCurrentWebBrowser->currentTabWebView()->page()
            && mCurrentWebBrowser->currentTabWebView()->page()->mainFrame())
    {
		QWebEnginePage* frame = mCurrentWebBrowser->currentTabWebView()->page()->mainFrame();
        QSize size = frame->contentsSize();

        qDebug() << size;

        QVariant top = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientTop");
        QVariant left = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientLeft");
        QVariant width = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientWidth");
        QVariant height = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientHeight");

        QSize vieportSize = mCurrentWebBrowser->currentTabWebView()->page()->viewportSize();
        mCurrentWebBrowser->currentTabWebView()->page()->setViewportSize(frame->contentsSize());
        pix = QPixmap(frame->geometry().width(), frame->geometry().height());

        {
            QPainter p(&pix);
            frame->render(&p);
        }

        if (left.isValid() && top.isValid() && width.isValid() && height.isValid())
        {
            bool okLeft, okTop, okWidth, okHeight;

            int iLeft = left.toInt(&okLeft) * frame->zoomFactor();
            int iTop = top.toInt(&okTop) * frame->zoomFactor();
            int iWidth = width.toInt(&okWidth) * frame->zoomFactor();
            int iHeight = height.toInt(&okHeight) * frame->zoomFactor();

            if(okLeft && okTop && okWidth && okHeight)
            {
                pix = pix.copy(iLeft, iTop, iWidth, iHeight);
            }
        }


        mCurrentWebBrowser->currentTabWebView()->page()->setViewportSize(vieportSize);
    }*/

    return pix;
}


void WBWebController::setupPalettes()
{
    if(!mToolsCurrentPalette)
    {
        mToolsCurrentPalette = new WBWebToolsPalette(WBApplication::mainWindow);
        WBApplication::boardController->paletteManager()->setCurrentWebToolsPalette(mToolsCurrentPalette);
#ifndef Q_OS_WIN
        if (WBPlatformUtils::hasVirtualKeyboard() && WBApplication::boardController->paletteManager()->mKeyboardPalette)
            connect(WBApplication::boardController->paletteManager()->mKeyboardPalette, SIGNAL(closed()),
                    WBApplication::boardController->paletteManager()->mKeyboardPalette, SLOT(onDeactivated()));
#endif

        connect(mMainWindow->actionWebCustomCapture, SIGNAL(triggered()), this, SLOT(customCapture()));
        connect(mMainWindow->actionWebWindowCapture, SIGNAL(triggered()), this, SLOT(captureWindow()));
        connect(mMainWindow->actionWebOEmbed, SIGNAL(triggered()), this, SLOT(captureoEmbed()));
        connect(mMainWindow->actionEduMedia, SIGNAL(triggered()), this, SLOT(captureEduMedia()));

        connect(mMainWindow->actionWebShowHideOnDisplay, SIGNAL(toggled(bool)), this, SLOT(toogleMirroring(bool)));
        connect(mMainWindow->actionWebTrap, SIGNAL(toggled(bool)), this, SLOT(toggleWebTrap(bool)));

        mToolsCurrentPalette->hide();
        mToolsCurrentPalette->adjustSizeAndPosition();

        if (controlView()){
            int left = controlView()->width() - 20 - mToolsCurrentPalette->width();
            int top = (controlView()->height() - mToolsCurrentPalette->height()) / 2;
            mToolsCurrentPalette->setCustomPosition(true);
            mToolsCurrentPalette->move(left, top);
        }
        
		connect(mMainWindow->actionWebTools, SIGNAL(triggered(bool)), this, SLOT(triggerWebTools(bool)));
		mMainWindow->actionWebTools->trigger();
    }
}



void WBWebController::toggleWebTrap(bool checked)
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
        mCurrentWebBrowser->currentTabWebView()->setIsTrapping(checked);
}


void WBWebController::captureWindow()
{
    QPixmap webPagePixmap = captureCurrentPage();

    if (!webPagePixmap.isNull())
        emit imageCaptured(webPagePixmap, true, mCurrentWebBrowser->currentTabWebView()->url());
}


void WBWebController::customCapture()
{
    mToolsCurrentPalette->setVisible(false);
    qApp->processEvents();

    WBCustomCaptureWindow customCaptureWindow(mCurrentWebBrowser);

    customCaptureWindow.show();

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted)
    {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false, mCurrentWebBrowser->currentTabWebView()->url());
    }

    mToolsCurrentPalette->setVisible(true);
}


void WBWebController::toogleMirroring(bool checked)
{
    WBApplication::applicationController->mirroringEnabled(checked);
}


QPixmap WBWebController::getScreenPixmap()
{
    QDesktopWidget *desktop = QApplication::desktop();
    // we capture the screen in which the mouse is.
    const QRect primaryScreenRect = desktop->screenGeometry(QCursor::pos());
    QCoreApplication::flush ();

    return QPixmap::grabWindow(desktop->winId(),
                               primaryScreenRect.x(), primaryScreenRect.y(),
                               primaryScreenRect.width(), primaryScreenRect.height());
}


void WBWebController::screenLayoutChanged()
{
    bool hasDisplay = (WBApplication::applicationController &&
                       WBApplication::applicationController->displayManager() &&
                       WBApplication::applicationController->displayManager()->hasDisplay());

    WBApplication::mainWindow->actionWebShowHideOnDisplay->setVisible(hasDisplay);
}


void WBWebController::closing()
{
    //NOOP
}


void WBWebController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->actionWebReload->setVisible(highResolution);
    mMainWindow->actionStopLoading->setVisible(highResolution);

    //if(mCurrentWebBrowser )
    //    mCurrentWebBrowser->adaptToolBar(highResolution);

}


void WBWebController::showTabAtTop(bool attop)
{
    if (mCurrentWebBrowser)
        mCurrentWebBrowser->showTabAtTop(attop);
}

void WBWebController::captureoEmbed()
{
    if ( mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()){
        // TODO : Uncomment the next lines to continue the youtube button bugfix
        //    getEmbeddableContent();

        // And comment from here

        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isOEmbedable(currentUrl))
        {
            WBGraphicsW3CWidgetItem * widget = WBApplication::boardController->activeScene()->addOEmbed(currentUrl);

            if(widget)
            {
                WBApplication::applicationController->showBoard();
                WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
            }
        }
        // --> Until here
    }
}

void WBWebController::lookForEmbedContent(QString* pHtml, QString tag, QString attribute, QList<QUrl> *pList)
{
    if(NULL != pHtml && NULL != pList){
        QVector<QString> urlsFound;
        // Check for <embed> content
        QRegExp exp(QString("<%0(.*)").arg(tag));
        exp.indexIn(*pHtml);
        QStringList strl = exp.capturedTexts();
        if(2 <= strl.size() && strl.at(1) != ""){
            // Here we call this regular expression:
            // src\s?=\s?['"]([^'"]*)['"]
            // It says: give me all characters that are after src=" (or src = ")
            QRegExp src(QString("%0\\s?=\\s?['\"]([^'\"]*)['\"]").arg(attribute));
            for(int i=1; i<strl.size(); i++){
                src.indexIn(strl.at(i));
                QStringList urls = src.capturedTexts();
                if(2 <= urls.size() && urls.at(1) != "" && !urlsFound.contains(urls.at(1))){
                    urlsFound << urls.at(1);
                    pList->append(QUrl(urls.at(1)));
                }
            }
        }
    }
}

void WBWebController::checkForOEmbed(QString *pHtml)
{
    mOEmbedParser.parse(*pHtml);
}

void WBWebController::getEmbeddableContent()
{
    // Get the source code of the page
    if(mCurrentWebBrowser){
        //QNetworkAccessManager* pNam = mCurrentWebBrowser->currentTabWebView()->webPage()->networkAccessManager();
        //if(NULL != pNam){
        //    QString html = mCurrentWebBrowser->currentTabWebView()->webPage()->mainFrame()->toHtml();
        //    mOEmbedParser.setNetworkAccessManager(pNam);

        //    // First, we have to check if there is some oembed content
        //    checkForOEmbed(&html);

        //    // Note: The other contents will be verified once the oembed ones have been checked
        //}
    }
}

void WBWebController::captureEduMedia()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isEduMedia(currentUrl))
        {
           /* QWebElementCollection objects = webView->page()->currentFrame()->findAllElements("object");

            foreach(QWebElement object, objects)
            {
                foreach(QWebElement param, object.findAll("param"))
                {
                    if(param.attribute("name") == "flashvars")
                    {
                        QString value = param.attribute("value");
                        QString midValue;
                        QString langValue;
                        QString hostValue;

                        QStringList flashVars = value.split("&");

                        foreach(QString flashVar, flashVars)
                        {
                            QStringList var = flashVar.split("=");

                            if (var.length() < 2)
                                break;

                            if (var.at(0) == "mid")
                                midValue = var.at(1);
                            else if (var.at(0) == "lang")
                                langValue = var.at(1);
                            else if (var.at(0) == "host")
                                hostValue = var.at(1);

                        }

                        if (midValue.length() > 0 && langValue.length() > 0 && hostValue.length() > 0)
                        {
                            QString swfUrl = "http://" + hostValue + "/" + langValue + "/fl/" + midValue;

                            WBApplication::boardController->downloadURL(QUrl(swfUrl));

                            WBApplication::applicationController->showBoard();
                            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);

                            return;
                        }
                    }
                }
            }*/
        }
    }
    else
    {
        WBApplication::showMessage("Cannot find any reference to eduMedia content");
    }
}

bool WBWebController::isOEmbedable(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    foreach(QString provider, mOEmbedProviders)
    {
        if(urlAsString.contains(provider))
            return true;
    }

    return false;
}


bool WBWebController::isEduMedia(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    if (urlAsString.contains("edumedia-sciences.com"))
    {
        return true;
    }

    return false;
}


void WBWebController::loadUrl(const QUrl& url)
{
    WBApplication::applicationController->showInternet();
    if (WBSettings::settings()->webUseExternalBrowser->get().toBool())

        QDesktopServices::openUrl(url);
    else
        mCurrentWebBrowser->loadUrlInNewTab(url);


}


QWebEngineView* WBWebController::createNewTab()
{
    if (mCurrentWebBrowser)
        WBApplication::applicationController->showInternet();

    return mCurrentWebBrowser->createNewTab();
}


void WBWebController::copy()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Copy);
        if(act)
            act->trigger();
    }
}


void WBWebController::paste()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Paste);
        if(act)
            act->trigger();
    }
}


void WBWebController::cut()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Cut);
        if(act)
            act->trigger();
    }
}

void WBWebController::onOEmbedParsed(QVector<sOEmbedContent> contents)
{
    QList<QUrl> urls;

    foreach(sOEmbedContent cnt, contents){
        urls << QUrl(cnt.url);
    }

    // TODO : Implement this
    //lookForEmbedContent(&html, "embed", "src", &urls);
    //lookForEmbedContent(&html, "video", "src", &contentUrls);
    //lookForEmbedContent(&html, "object", "data", &contentUrls);

    // TODO: check the hidden iFrame

    if(!urls.empty()){
        QUrl contentUrl;    // The selected content url

        if(1 == urls.size()){
            contentUrl = urls.at(0);
        }else{
            // TODO : Display a dialog box asking the user which content to get and set contentUrl to the selected content

        }

        WBGraphicsW3CWidgetItem * widget = WBApplication::boardController->activeScene()->addOEmbed(contentUrl);

        if(widget)
        {
            WBApplication::applicationController->showBoard();
            WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
        }
    }
}

void WBWebController::triggerWebTools(bool checked)
{
	mToolsCurrentPalette->setVisible(checked);

}