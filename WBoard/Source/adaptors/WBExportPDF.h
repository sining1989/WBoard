#ifndef WBEXPORTPDF_H_
#define WBEXPORTPDF_H_

#include <QtCore>
#include "WBExportAdaptor.h"

class WBDocumentProxy;

class WBExportPDF : public WBExportAdaptor
{
    Q_OBJECT

    public:
        WBExportPDF(QObject *parent = 0);
        virtual ~WBExportPDF();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(WBDocumentProxy* pDocument);
        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

        virtual bool persistsDocument(WBDocumentProxy* pDocument, const QString& filename);
};

#endif /* WBEXPORTPDF_H_ */
