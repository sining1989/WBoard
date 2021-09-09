#ifndef WBLIBRARYAPI_H_
#define WBLIBRARYAPI_H_

#include <QtWidgets>
#include <QtWebEngine>
#include <QWebEngineView>

class WBLibraryAPI : public QObject
{
    Q_OBJECT

    public:
        WBLibraryAPI(QWebEngineView *pWebView = 0);
        virtual ~WBLibraryAPI();

    public slots:
        void addObject(QString pUrl, int width = 0, int height = 0, int x = 0, int y = 0, bool background = false);
        void startDrag(QString pUrl);
        void addTool(QString pUrl, int x = 0, int y = 0);

    signals:
       void downloadTriggered(const QUrl& url, const QPointF& pos, const QSize& pSize, bool background);

    private:
		QWebEngineView* mWebView;
};

#endif /* WBLIBRARYAPI_H_ */
