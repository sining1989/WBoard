#include "WBBrowserWindow.h"
#include "WBDownloadManager_.h"
#include "WBTabWidget.h"
#include "WBWebView.h"
#include "web/WBWebPage_.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "board/WBBoardController.h"
#include "core/WBSettings.h"

#include "network/WBNetworkAccessManager.h"
#include "network/UBCookieJar.h"

#include <QtWidgets>
#include <QtWebEngine>
#include <QtUiTools/QUiLoader>
#include <QMessageBox>
#include <QWebChannel>

#include "core/memcheck.h"

WBWebPage::WBWebPage(QObject *parent)
    : WBWebPage_(parent)
    , mKeyboardModifiers(Qt::NoModifier)
    , mPressedButtons(Qt::NoButton)
    , mOpenInNewTab(false)
{
    //setNetworkAccessManager(WBNetworkAccessManager::defaultAccessManager());

    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(handleUnsupportedContent(QNetworkReply *)));
}

WBBrowserWindow *WBWebPage::mainWindow()
{
    QObject *w = this->parent();
    while (w)
    {
        if (WBBrowserWindow *mw = qobject_cast<WBBrowserWindow*>(w))
            return mw;

        w = w->parent();
    }

    return 0;
}

bool WBWebPage::acceptNavigationRequest(QWebChannel *frame, const QNetworkRequest &request, NavigationType type)
{
    if (type == QWebEnginePage::NavigationTypeLinkClicked
        && (mKeyboardModifiers & Qt::ControlModifier
            || mPressedButtons == Qt::MidButton))
    {
        WBWebView *webView;

        bool selectNewTab = (mKeyboardModifiers & Qt::ShiftModifier);
        webView = mainWindow()->tabWidget()->newTab(selectNewTab);

        webView->load(request);
        mKeyboardModifiers = Qt::NoModifier;
        mPressedButtons = Qt::NoButton;

        return false;
    }

    //if (frame == mainFrame()) //zhusizhi
    {
        mLoadingUrl = request.url();
        emit loadingUrl(mLoadingUrl);
    }

	return true;// QWebEnginePage::acceptNavigationRequest(frame, request, type);
}


QWebEnginePage *WBWebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);

    return mainWindow()->tabWidget()->newTab()->page();
}


QObject *WBWebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames
        , const QStringList &paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);

    QUiLoader loader;

    return loader.createWidget(classId, view());
}


void WBWebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    if(reply->url().scheme() == "mailto")
    {
        bool result = QDesktopServices::openUrl(reply->url());
        if (result)
            return;
    }

    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    bool isPDF = (contentType == "application/pdf");

    // Delete this big "if (isPDF)" block to get the pdf directly inside the browser
    if (isPDF)
    {
        QMessageBox messageBox(mainWindow());
        messageBox.setText(tr("Download PDF Document: would you prefer to download the PDF file or add it to the current WBoard document?"));

        messageBox.addButton(tr("Download"), QMessageBox::AcceptRole);
        QAbstractButton *addButton = messageBox.addButton(tr("Add to Current Document"), QMessageBox::AcceptRole);

        messageBox.exec();
        if (messageBox.clickedButton() == addButton)
        {
            WBApplication::applicationController->showBoard();
            WBApplication::boardController->downloadURL(reply->request().url());
            return;
        }
        else
        {
            isPDF = false;
        }
    }

    if (!isPDF && reply->error() == QNetworkReply::NoError)
    {
        if(contentType == "application/widget")
            WBBrowserWindow::downloadManager()->handleUnsupportedContent(reply,false, WBSettings::settings()->userGipLibraryDirectory());
        else
            WBBrowserWindow::downloadManager()->handleUnsupportedContent(reply);
        return;
    }

    QFile file;
    file.setFileName(isPDF ? QLatin1String(":/webbrowser/object-wrapper.html") : QLatin1String(":/webbrowser/notfound.html"));

    bool isOpened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpened);
    QString html;
    if (isPDF)
    {
        html = QString(QLatin1String(file.readAll()))
                        .arg(tr("PDF"))
                        .arg("application/x-ub-pdf")
                        .arg(reply->url().toString());
    }
    else
    {
        QString title = tr("Error loading page: %1").arg(reply->url().toString());
        html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(reply->errorString())
                        .arg(reply->url().toString());
    }

  //  QList<QWebEnginePage*> frames;
  //  frames.append(page());
  //  while (!frames.isEmpty())
  //  {
		//QWebEnginePage *frame = frames.takeFirst();
  //      if (frame->url() == reply->url())
  //      {
  //          frame->setHtml(html, reply->url());
  //          return;
  //      }
		//QList<QWebEnginePage *> children = frame->childFrames();
  //      foreach(QWebEnginePage *frame, children)
  //          frames.append(frame);
  //  }

  //  if (mLoadingUrl == reply->url())
  //  {
  //      //mainFrame()->setHtml(html, reply->url());
  //  }
}


WBWebView::WBWebView(QWidget* parent)
    : WBWebTrapWebView(parent)
    , mProgress(0)
    , mPage(new WBWebPage(this))
{
    setObjectName("wbBrowserWebView");

    setPage(mPage);

    //QWebEngineView::setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    connect(page(), SIGNAL(statusBarMessage(const QString&)),
            this, SLOT(setStatusBarText(const QString&)));

    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(setProgress(int)));

    connect(this, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished(bool)));

    connect(this, SIGNAL(loadStarted()),
            this, SLOT(loadStarted()));

    connect(page(), SIGNAL(loadingUrl(const QUrl&)),
            this, SIGNAL(urlChanged(const QUrl &)));

    connect(page(), SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(downloadRequested(const QNetworkRequest &)));

//    page()->setForwardUnsupportedContent(true);

}


void WBWebView::contextMenuEvent(QContextMenuEvent *event)
{
    //QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());

    //if (!r.linkUrl().isEmpty())
    //{
    //    QMenu menu(this);
    //    menu.addAction(pageAction(QWebEnginePage::OpenLinkInNewWindow));
    //    menu.addAction(tr("Open in New Tab"), this, SLOT(openLinkInNewTab()));
    //    menu.addSeparator();
    //    menu.addAction(pageAction(QWebEnginePage::DownloadLinkToDisk));
    //    // Add link to bookmarks...
    //    menu.addSeparator();
    //    menu.addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
    //    if (page()->settings()->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled))
    //        menu.addAction(pageAction(QWebEnginePage::InspectElement));
    //    menu.exec(mapToGlobal(event->pos()));
    //    return;
    //}
    WBWebTrapWebView::contextMenuEvent(event);
}


void WBWebView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        //setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
    WBWebTrapWebView::wheelEvent(event);
}


void WBWebView::openLinkInNewTab()
{
    mPage->mOpenInNewTab = true;
    pageAction(QWebEnginePage::OpenLinkInNewWindow)->trigger();
}


void WBWebView::setProgress(int progress)
{
    //qDebug() << "loading progress" << progress << "% in" << mLoadStartTime.elapsed() << "ms";

    mProgress = progress;
}


void WBWebView::loadStarted()
{
    mLoadStartTime.start();
}


void WBWebView::loadFinished(bool ok)
{
    if (100 != mProgress)
    {
        qWarning() << "Received finished signal while progress is still:" << progress()
                   << "Url:" << url();
    }

    QString error;

    if (!ok)
        error = "with error";

    qDebug() << "page loaded in" << mLoadStartTime.elapsed() << "ms" << url().toString() << error;

    mProgress = 0;
}


void WBWebView::load(const QUrl &url)
{
    qDebug() << "loading " << url.toString();

    mInitialUrl = url;

    WBWebTrapWebView::load(url);
}


QString WBWebView::lastStatusBarText() const
{
    return mLastStatusBarText;
}


QUrl WBWebView::url() const
{
    QUrl url;
    try{
        url = QWebEngineView::url();
    } catch(...)
    {}

    if (!url.isEmpty())
       return url;

    return mInitialUrl;
}

void WBWebView::mousePressEvent(QMouseEvent *event)
{
    mPage->mPressedButtons = event->buttons();
    mPage->mKeyboardModifiers = event->modifiers();

    WBWebTrapWebView::mousePressEvent(event);
}

void WBWebView::mouseReleaseEvent(QMouseEvent *event)
{
    WBWebTrapWebView::mouseReleaseEvent(event);

    if (!event->isAccepted() && (mPage->mPressedButtons & Qt::MidButton))
    {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty())
        {
            setUrl(url);
        }
    }
}


void WBWebView::setStatusBarText(const QString &string)
{
    //qDebug() << "WebView::setStatusBarText" << string;

    mLastStatusBarText = string;
}


void WBWebView::downloadRequested(const QNetworkRequest &request)
{
    WBBrowserWindow::downloadManager()->download(request);
}

void WBWebView::load(const QNetworkRequest & request, QNetworkAccessManager::Operation operation, const QByteArray & body)
{
    //WBWebTrapWebView::load(request, operation, body);
}

