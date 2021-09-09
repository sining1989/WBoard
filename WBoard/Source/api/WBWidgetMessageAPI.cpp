#include <QWebEnginePage>
#include "WBWidgetMessageAPI.h"

#include "core/WBApplication.h"

#include "core/memcheck.h"

WBWidgetMessageAPI::WBWidgetMessageAPI(WBGraphicsWidgetItem *graphicsWidgetItem, QObject *parent)
    : QObject(parent)
    , mGraphicsWidgetItem(graphicsWidgetItem)
{
    connect(WBWidgetAPIMessageBroker::instance(), SIGNAL(newMessage(const QString&, const QString&)), this, SLOT(onNewMessage(const QString&, const QString&)), Qt::QueuedConnection);
}

WBWidgetMessageAPI::~WBWidgetMessageAPI()
{
    // NOOP
}


void WBWidgetMessageAPI::sendMessage(const QString& pTopicName, const QString& pMessage)
{
    WBWidgetAPIMessageBroker::instance()->sendMessage(pTopicName, pMessage);
}


void WBWidgetMessageAPI::onNewMessage(const QString& pTopicName, const QString& pMessage)
{
	//20210202 zhusizhi
    if (mSubscribedTopics.contains(pTopicName))
    {
        //if (mGraphicsWidgetItem && mGraphicsWidgetItem->page() && mGraphicsWidgetItem->page()->mainFrame())
        {
            QString js;
            js += "if(widget && widget.messages && widget.messages.onmessage)";
            js += "{widget.messages.onmessage('";
            js += pMessage + "', '" + pTopicName + "')}";

            //mGraphicsWidgetItem->page()->
            //    mainFrame()->evaluateJavaScript(js);

        }
    }
}



WBWidgetAPIMessageBroker* WBWidgetAPIMessageBroker::sInstance = 0;


WBWidgetAPIMessageBroker::WBWidgetAPIMessageBroker(QObject *parent)
    : QObject(parent)
{
    // NOOP
}


WBWidgetAPIMessageBroker::~WBWidgetAPIMessageBroker()
{
    // NOOP
}


WBWidgetAPIMessageBroker* WBWidgetAPIMessageBroker::instance()
{
    if (!sInstance)
        sInstance = new WBWidgetAPIMessageBroker(WBApplication::staticMemoryCleaner);

    return sInstance;

}


void WBWidgetAPIMessageBroker::sendMessage(const QString& pTopicName, const QString& pMessage)
{
    emit newMessage(pTopicName, pMessage);
}
