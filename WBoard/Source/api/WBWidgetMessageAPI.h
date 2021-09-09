#ifndef WBWIDGETMESSAGEAPI_H_
#define WBWIDGETMESSAGEAPI_H_

#include <QtCore>

#include "domain/WBGraphicsWidgetItem.h"

class WBWidgetMessageAPI : public QObject
{
    Q_OBJECT

    public:
        WBWidgetMessageAPI(WBGraphicsWidgetItem *graphicsWidgetItem, QObject *parent = 0);
        virtual ~WBWidgetMessageAPI();

    public slots:
        void sendMessage(const QString& pTopicName, const QString& pMessage);

        void subscribeToTopic(const QString& pTopicName)
        {
            mSubscribedTopics << pTopicName;
        }

        void unsubscribeFromTopic(const QString& pTopicName)
        {
            mSubscribedTopics.remove(pTopicName);
        }

    private slots:
         void onNewMessage(const QString& pTopicName, const QString& pMessage);

    private:
        QSet<QString> mSubscribedTopics;
        WBGraphicsWidgetItem *mGraphicsWidgetItem;
};

class WBWidgetAPIMessageBroker : public QObject
{
    Q_OBJECT

    private:
        WBWidgetAPIMessageBroker(QObject *parent = 0);
        ~WBWidgetAPIMessageBroker();

    public:
        static WBWidgetAPIMessageBroker* instance();

    public slots:
        void sendMessage(const QString& pTopicName, const QString& pMessage);

    signals:
        void newMessage(const QString& pTopicName, const QString& pMessage);

    private:
      static WBWidgetAPIMessageBroker* sInstance;

};

#endif /* WBWIDGETMESSAGEAPI_H_ */
