#ifndef WBIMPORTIMAGE_H_
#define WBIMPORTIMAGE_H_

#include <QtGui>
#include "WBImportAdaptor.h"

class WBDocumentProxy;

class WBImportImage : public WBPageBasedImportAdaptor
{
    Q_OBJECT;

    public:
        WBImportImage(QObject *parent = 0);
        virtual ~WBImportImage();

        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        virtual QList<WBGraphicsItem*> import(const QUuid& uuid, const QString& filePath);
        virtual void placeImportedItemToScene(WBGraphicsScene* scene, WBGraphicsItem* item);
        virtual const QString& folderToCopy();
};

#endif /* WBIMPORTIMAGE_H_ */
