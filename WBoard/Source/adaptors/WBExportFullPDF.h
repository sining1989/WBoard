#ifndef WBExportFullPDF_H_
#define WBExportFullPDF_H_

#include <QtCore>
#include "WBExportAdaptor.h"
#include "WBExportPDF.h"

class WBDocumentProxy;

class WBExportFullPDF : public WBExportAdaptor
{
    Q_OBJECT;

    public:
        WBExportFullPDF(QObject *parent = 0);
        virtual ~WBExportFullPDF();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(WBDocumentProxy* pDocument);
        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

        virtual bool persistsDocument(WBDocumentProxy* pDocument, const QString& filename);

    protected:
        void saveOverlayPdf(WBDocumentProxy* pDocumentProxy, const QString& filename);

    private:
        float mScaleFactor;
        bool mHasPDFBackgrounds;

        WBExportPDF * mSimpleExporter;
};

#endif /* WBExportFullPDF_H_ */
