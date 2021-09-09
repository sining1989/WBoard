#ifndef WBEXPORTDOCUMENTSETADAPTOR_H
#define WBEXPORTDOCUMENTSETADAPTOR_H

#include <QtCore>
#include "WBExportAdaptor.h"
#include "frameworks/WBFileSystemUtils.h"
#include "globals/WBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

class WBDocumentProxy;
class WBDocumentTreeModel;

class WBExportDocumentSetAdaptor : public WBExportAdaptor
{
    Q_OBJECT

    public:
        WBExportDocumentSetAdaptor(QObject *parent = 0);
        virtual ~WBExportDocumentSetAdaptor();

        virtual QString exportName();
        virtual QString exportExtention();

        virtual void persist(WBDocumentProxy* pDocument);
        bool persistData(const QModelIndex &pRootIndex, QString filename);
        bool addDocumentToZip(const QModelIndex &pIndex, WBDocumentTreeModel *model, QuaZip &zip);

        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

#endif // WBEXPORTDOCUMENTSETADAPTOR_H
