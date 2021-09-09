#ifndef WBEXPORTDOCUMENT_H_
#define WBEXPORTDOCUMENT_H_

#include <QtCore>

#include "WBExportAdaptor.h"
#include "frameworks/WBFileSystemUtils.h"

class WBDocumentProxy;

class WBExportDocument : public WBExportAdaptor, public WBProcessingProgressListener
{
    Q_OBJECT

    public:
        WBExportDocument(QObject *parent = 0);
        virtual ~WBExportDocument();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(WBDocumentProxy* pDocument);

        virtual bool persistsDocument(WBDocumentProxy* pDocument, const QString& filename);

        virtual void processing(const QString& pObjectName, int pCurrent, int pTotal);

        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

#endif /* WBEXPORTDOCUMENT_H_ */
