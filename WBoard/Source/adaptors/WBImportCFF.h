#ifndef WBIMPORTCFF_H
#define WBIMPORTCFF_H

#include <QtGui>
#include "WBImportAdaptor.h"

class WBDocumentProxy;

class WBImportCFF : public WBDocumentBasedImportAdaptor
{
    Q_OBJECT;

    public:
        WBImportCFF(QObject *parent = 0);
        virtual ~WBImportCFF();

        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        virtual bool addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile);
        virtual WBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup);

    private:
        QString expandFileToDir(const QFile& pZipFile, const QString& pDir);
};

#endif // WBIMPORTCFF_H
