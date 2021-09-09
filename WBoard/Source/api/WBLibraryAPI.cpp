#include "WBLibraryAPI.h"

#include "core/WBApplication.h"
#include "board/WBBoardController.h"

#include "core/memcheck.h"

WBLibraryAPI::WBLibraryAPI(QWebEngineView *pWebView)
    : QObject(pWebView)
    , mWebView(pWebView)
{
    connect(this, SIGNAL(downloadTriggered(const QUrl&, const QPointF&, const QSize&, bool)),
            WBApplication::boardController, SLOT(downloadURL(const QUrl&, const QPointF&, const QSize&, bool)),
            Qt::QueuedConnection);

}


WBLibraryAPI::~WBLibraryAPI()
{
    // NOOP
}


void WBLibraryAPI::addObject(QString pUrl, int width, int height, int x, int y, bool background)
{
    if (WBApplication::boardController)
        WBApplication::boardController->downloadURL(QUrl(pUrl), QString(), QPointF(x, y), QSize(width, height), background);

}


void WBLibraryAPI::addTool(QString pUrl, int x, int y)
{
    if (WBApplication::boardController)
    {
        emit downloadTriggered(pUrl, QPointF(x , y), QSize(), false);
    }
}


void WBLibraryAPI::startDrag(QString pUrl)
{
    QDrag *drag = new QDrag(mWebView);
    QMimeData *mimeData = new QMimeData;

    QList<QUrl> urls;
    urls << QUrl(pUrl);

    mimeData->setUrls(urls);
    drag->setMimeData(mimeData);

    drag->exec(Qt::CopyAction);

}

