#ifndef WBWEBPAGE_H_
#define WBWEBPAGE_H_

#include <QtCore>
#include <QWebEnginePage>

class WBWebPage_ : public QWebEnginePage
{
    Q_OBJECT;

public:
    WBWebPage_(QObject *parent = 0);
    virtual ~WBWebPage_();

    virtual void javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID);

    virtual QString userAgentForUrl(const QUrl& url) const;

private:
    QString mCachedUserAgentString;
    /*QWebPluginFactory *mPluginFactory;*/

};

#endif /* WBWEBPAGE_H_ */
