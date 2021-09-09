#ifndef WBWEBTRAPWEBVIEW_H_
#define WBWEBTRAPWEBVIEW_H_

#include <QtWidgets>
#include <QtWebEngine>
#include <QWebEngineView>


class WBWebTrapWebView : public QWebEngineView
{
    Q_OBJECT;

public:
    WBWebTrapWebView(QWidget* parent = 0);
    virtual ~WBWebTrapWebView();

    void setIsTrapping(bool pIsTrapping);

    bool isTrapping()
    {
        return mIsTrapping;
    }

    void highliteElementAtPos(const QPoint& pos);
    void trapElementAtPos(const QPoint& pos);

signals:
    void pixmapCaptured(const QPixmap& pixmap, bool pageMode);
    void objectCaptured(const QUrl& pUrl, const QString& pMimeType, int pWidth, int pHeight);
    void embedCodeCaptured(const QString& embedCode);

    void webElementCaptured(const QUrl& pUrl, const QString& query);

private slots:
	void viewLoadFinished(bool ok);

protected:
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );
    virtual void hideEvent ( QHideEvent * event );

    virtual void paintEvent ( QPaintEvent * event );

    QString potentialEmbedCodeAtPos(const QPoint& pos);

    enum WebContentType
    {
        Unknown = 0, Image, Input, ObjectOrEmbed, ElementByQuery
    };

private:
    QRect mDomElementRect;
    QRect mWebViewElementRect;
    QString mElementQuery;
    WebContentType mCurrentContentType;

    bool mIsTrapping;

    QWidget* mTrapingWidget;

};

class UBWebTrapMouseEventMask : public QWidget
{
    public:
        UBWebTrapMouseEventMask(WBWebTrapWebView* pWebView);
        virtual ~UBWebTrapMouseEventMask();

    protected:
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void paintEvent(QPaintEvent *);

   private:
       WBWebTrapWebView *mTrappedWebView;
};

#endif /* WBWEBTRAPWEBVIEW_H_ */
