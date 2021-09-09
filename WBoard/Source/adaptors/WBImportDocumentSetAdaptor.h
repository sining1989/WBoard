#ifndef WBIMPORTDOCUMENTSETADAPTOR_H
#define WBIMPORTDOCUMENTSETADAPTOR_H

#include <QtGui>
#include "WBImportAdaptor.h"

class WBDocumentProxy;

class WBImportDocumentSetAdaptor : public WBImportAdaptor
{
    Q_OBJECT

    public:
        WBImportDocumentSetAdaptor(QObject *parent = 0);
        virtual ~WBImportDocumentSetAdaptor();

        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        QFileInfoList importData(const QString &zipFile, const QString &destination);

//        virtual WBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup);
//        virtual bool addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile);

    private:
        bool extractFileToDir(const QFile& pZipFile, const QString& pDir);

};

#endif // WBIMPORTDOCUMENTSETADAPTOR_H
