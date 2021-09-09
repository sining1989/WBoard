#ifndef WBIMPORTDOCUMENT_H_
#define WBIMPORTDOCUMENT_H_

#include <QtGui>
#include "WBImportAdaptor.h"

class WBDocumentProxy;

class WBImportDocument : public WBDocumentBasedImportAdaptor
{
    Q_OBJECT;

    public:
        WBImportDocument(QObject *parent = 0);
        virtual ~WBImportDocument();
		
        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        virtual WBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup);
        virtual bool addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile);

    private:
        bool extractFileToDir(const QFile& pZipFile, const QString& pDir, QString& documentRoot);
};

#endif /* WBIMPORTDOCUMENT_H_ */
