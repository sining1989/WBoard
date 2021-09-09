#include "WBExportCFF.h"
#include "WBCFFAdaptor.h"
#include "document/WBDocumentProxy.h"
#include "core/WBDocumentManager.h"
#include "core/WBApplication.h"
#include "core/memcheck.h"
#include "document/WBDocumentController.h"

#include <QModelIndex>
#include <QObject>


WBExportCFF::WBExportCFF(QObject *parent)
: WBExportAdaptor(parent)
{

}

WBExportCFF::~WBExportCFF()
{

}
QString WBExportCFF::exportName()
{
    return tr("Export to IWB");
}

QString WBExportCFF::exportExtention()
{
    return QString(".iwb");
}

void WBExportCFF::persist(WBDocumentProxy* pDocument)
{
    QString src = pDocument->persistencePath();

    if (!pDocument)
        return;

    QString filename = askForFileName(pDocument, tr("Export as IWB File"));

    if (filename.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
            WBApplication::showMessage(tr("Exporting document..."));

            WBCFFAdaptor toIWBExporter;
            if (toIWBExporter.convertWBZToIWB(src, filename))
            {
                if (mIsVerbose)
                    WBApplication::showMessage(tr("Export successful."));
            }
            else 
                if (mIsVerbose)
                    WBApplication::showMessage(tr("Export failed."));

        showErrorsList(toIWBExporter.getConversionMessages());

        QApplication::restoreOverrideCursor();

    }
    
}

bool WBExportCFF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const WBDocumentTreeModel *docModel = qobject_cast<const WBDocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}
