#include "WBWebPage_.h"

#include <QtCore>
//#include <QWebPluginFactory>

//#include "pdf/UBWebPluginPDFWidget.h"

#include "core/memcheck.h"
//
//class UBWebPluginFactory : public QWebPluginFactory
//{
//    public:
//
//    UBWebPluginFactory(QObject *parent = 0) : QWebPluginFactory(parent)
//    {
//        // NOOP
//    }
//
//    QList<Plugin> plugins() const
//    {
//        QStringList pdfExtensions;
//        pdfExtensions << "pdf";
//        MimeType pdfMimeType = {"application/x-ub-pdf", "Portable Document Format", pdfExtensions};
//        QList<MimeType> mimeTypes;
//        mimeTypes << pdfMimeType;
//        Plugin pdfPlugin = {"PDF Plugin", "Display PDF files", mimeTypes};
//        QList<Plugin> plugins;
//        plugins << pdfPlugin;
//        return plugins;
//    }
//
//    QObject* create(const QString &mimeType, const QUrl &url, const QStringList &argumentNames, const QStringList &argumentValues) const
//    {
//        Q_UNUSED(url);
//        Q_UNUSED(argumentNames);
//        Q_UNUSED(argumentValues);
//
//        if (mimeType == "application/x-ub-pdf")
//        {
//            UBWebPluginPDFWidget *pdfWidget = new UBWebPluginPDFWidget(url);
//            pdfWidget->setObjectName("PDFWebPluginWidget");
//            return pdfWidget;
//        }
//        return 0;
//    }
//};
//


WBWebPage_::WBWebPage_(QObject *parent)
    : QWebEnginePage(parent)
    //, mPluginFactory(0)
{
	mCachedUserAgentString = "";// QWebEnginePage::userAgentForUrl(QUrl());
    //mPluginFactory = new UBWebPluginFactory();
    //setPluginFactory(mPluginFactory);

    //qDebug() << "caching user agent string" << mCachedUserAgentString;
}

WBWebPage_::~WBWebPage_()
{
//    delete mPluginFactory;
}


void WBWebPage_::javaScriptConsoleMessage(const QString &message, int lineNumber, const QString &sourceID)
{
    qDebug("JavaScript> %s (%s:%d)", qPrintable(message), qPrintable(sourceID), lineNumber);
}


QString WBWebPage_::userAgentForUrl(const QUrl& url) const
{
    Q_UNUSED(url);
    return mCachedUserAgentString;
}


