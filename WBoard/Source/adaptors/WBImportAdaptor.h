#ifndef WBIMPORTADAPTOR_H_
#define WBIMPORTADAPTOR_H_

#include <QtGui>

class WBGraphicsItem;
class WBGraphicsScene;
class WBDocumentProxy;

class WBImportAdaptor : public QObject
{
    Q_OBJECT;

    protected:
        WBImportAdaptor(bool _documentBased, QObject *parent = 0);
        virtual ~WBImportAdaptor();

    public:
        virtual QStringList supportedExtentions() = 0;
        virtual QString importFileFilter() = 0;

        bool isDocumentBased(){return documentBased;}

    private:
        bool documentBased;
        
};

class WBPageBasedImportAdaptor : public WBImportAdaptor
{
	protected:
        WBPageBasedImportAdaptor(QObject *parent = 0);

	public:
        virtual QList<WBGraphicsItem*> import(const QUuid& uuid, const QString& filePath) = 0;
        virtual void placeImportedItemToScene(WBGraphicsScene* scene, WBGraphicsItem* item) = 0;
        virtual const QString& folderToCopy() = 0;
};

class WBDocumentBasedImportAdaptor : public WBImportAdaptor
{
	protected:
        WBDocumentBasedImportAdaptor(QObject *parent = 0);
	public:
    virtual WBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup) = 0;
    virtual bool addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile) = 0;
};


#endif /* WBIMPORTADAPTOR_H_ */
