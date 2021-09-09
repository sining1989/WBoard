#ifndef WBWEBCONTROLLER_H_
#define WBWEBCONTROLLER_H_

#include <QtWidgets>
#include <QtWebEngine>
#include <QWebEnginePage>
#include <QWebEngineView>

#include "WBOEmbedParser.h"

class WBBrowserWindow;
class WBApplication;
class WBTrapFlashController;
class WBMainWindow;
class WBWebToolsPalette;
class WBWebView;
class WBServerXMLHttpRequest;

class WBWebController : public QObject
{
    Q_OBJECT

public:
    WBWebController(WBMainWindow* mainWindow);
    virtual ~WBWebController();
    void closing();
    void adaptToolBar();

    QPixmap captureCurrentPage();

    void showTabAtTop(bool attop);

    void loadUrl(const QUrl& url);

    QWebEngineView* createNewTab();

    QUrl currentPageUrl() const;

    void show();

    WBBrowserWindow* GetCurrentWebBrowser(){return mCurrentWebBrowser;}

protected:
    void setupPalettes();
    QPixmap getScreenPixmap();

signals:
	void imageCaptured(const QPixmap& pCapturedPixmap, bool pageMode, const QUrl& source);
	void activeWebPageChanged(WBWebView* pWebView);

public slots:
    void screenLayoutChanged();

    void setSourceWidget(QWidget* pWidget);
    void captureWindow();
    void customCapture();
    void toogleMirroring(bool checked);

    QWidget* controlView()
    {
        return mBrowserWidget;
    }

    void captureoEmbed();
    void captureEduMedia();

    bool isOEmbedable(const QUrl& pUrl);
    bool hasEmbeddedContent();
    void getEmbeddableContent();

    bool isEduMedia(const QUrl& pUrl);

    void copy();
    void paste();
    void cut();
	void triggerWebTools(bool checked);

private slots:
	void activePageChanged();
	void toggleWebTrap(bool checked);
	void onOEmbedParsed(QVector<sOEmbedContent> contents);

private:
    void initialiazemOEmbedProviders();
    void webBrowserInstance();
    void lookForEmbedContent(QString* pHtml, QString tag, QString attribute, QList<QUrl>* pList);
    void checkForOEmbed(QString* pHtml);

    WBMainWindow *mMainWindow;

    WBBrowserWindow* mCurrentWebBrowser;

    QWidget* mBrowserWidget;

    WBWebToolsPalette* mToolsCurrentPalette;

    bool mToolsPalettePositionned;

    bool mDownloadViewIsVisible;

    QStringList mOEmbedProviders;

    WBOEmbedParser mOEmbedParser;

};

#endif /* WBWEBCONTROLLER_H_ */
