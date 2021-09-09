#ifndef WBIMPORTPDF_H_
#define WBIMPORTPDF_H_

#include <QtGui>
#include "WBImportAdaptor.h"

class WBDocumentProxy;

class WBImportPDF : public WBPageBasedImportAdaptor
{
    Q_OBJECT;

    public:
        WBImportPDF(QObject *parent = 0);
        virtual ~WBImportPDF();

        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        virtual QList<WBGraphicsItem*> import(const QUuid& uuid, const QString& filePath);
        virtual void placeImportedItemToScene(WBGraphicsScene* scene, WBGraphicsItem* item);
        virtual const QString& folderToCopy();

    private:
        int dpi;
};

#endif /* WBIMPORTPDF_H_ */
