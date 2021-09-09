#ifndef WBPERSISTENCEWORKER_H
#define WBPERSISTENCEWORKER_H

#include <QObject>
#include <QSemaphore>
#include "document/WBDocumentProxy.h"
#include "domain/WBGraphicsScene.h"

typedef enum{
    WriteScene = 0,
    ReadScene,
    WriteMetadata
}ActionType;

typedef struct{
    ActionType action;
    WBDocumentProxy* proxy;
    WBGraphicsScene* scene;
    int sceneIndex;
}PersistenceInformation;

class WBPersistenceWorker : public QObject
{
    Q_OBJECT
public:
    explicit WBPersistenceWorker(QObject *parent = 0);

    void saveScene(WBDocumentProxy* proxy, WBGraphicsScene* scene, const int pageIndex);
    void readScene(WBDocumentProxy* proxy, const int pageIndex);
    void saveMetadata(WBDocumentProxy* proxy);

signals:
   void finished();
   void error(QString string);
   void sceneLoaded(QByteArray text,WBDocumentProxy* proxy, const int pageIndex);
   void scenePersisted(WBGraphicsScene* scene);
   void metadataPersisted(WBDocumentProxy* proxy);

public slots:
   void process();
   void applicationWillClose();

protected:
   bool mReceivedApplicationClosing;
   QSemaphore mSemaphore;
   QList<PersistenceInformation> saves;
};

#endif // WBPERSISTENCEWORKER_H
