#include "WBImportPDF.h"

#include "document/WBDocumentProxy.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"

#include "domain/WBGraphicsPDFItem.h"

#include "pdf/PDFRenderer.h"

#include "core/memcheck.h"

WBImportPDF::WBImportPDF(QObject *parent)
    : WBPageBasedImportAdaptor(parent)
{
    QDesktopWidget* desktop = WBApplication::desktop();
    this->dpi = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
}


WBImportPDF::~WBImportPDF()
{
    // NOOP
}


QStringList WBImportPDF::supportedExtentions()
{
    return QStringList("pdf");
}


QString WBImportPDF::importFileFilter()
{
    return tr("Portable Document Format (*.pdf)");
}


QList<WBGraphicsItem*> WBImportPDF::import(const QUuid& uuid, const QString& filePath)
{
    QList<WBGraphicsItem*> result;

    PDFRenderer *pdfRenderer = PDFRenderer::rendererForUuid(uuid, filePath, true); // renderer is automatically deleted when not used anymore

    if (!pdfRenderer->isValid())
    {
        WBApplication::showMessage(tr("PDF import failed."));
        return result;
    }
    pdfRenderer->setDPI(this->dpi);

    int pdfPageCount = pdfRenderer->pageCount();

    for(int pdfPageNumber = 1; pdfPageNumber <= pdfPageCount; pdfPageNumber++)
    {
        WBApplication::showMessage(tr("Importing page %1 of %2").arg(pdfPageNumber).arg(pdfPageCount), true);
        result << new WBGraphicsPDFItem(pdfRenderer, pdfPageNumber); // deleted by the scene
    }
    return result;
}

void WBImportPDF::placeImportedItemToScene(WBGraphicsScene* scene, WBGraphicsItem* item)
{
    WBGraphicsPDFItem *pdfItem = (WBGraphicsPDFItem*)item;

    pdfItem->setPos(-pdfItem->boundingRect().width() / 2, -pdfItem->boundingRect().height() / 2);

    scene->setAsBackgroundObject(pdfItem, false, false);

    scene->setNominalSize(pdfItem->boundingRect().width(), pdfItem->boundingRect().height());
}

const QString& WBImportPDF::folderToCopy()
{
    return WBPersistenceManager::objectDirectory;
}
