#include "WBExportFullPDF.h"

#include <QtCore>
#include <QtSvg>
#include <QPrinter>

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBPersistenceManager.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsPDFItem.h"

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentController.h"

#include "pdf/GraphicsPDFItem.h"

#include "WBExportPDF.h"

//#include <Merger.h>
//#include <Exception.h>
//#include <Transformation.h>

#include "core/memcheck.h"


//using namespace merge_lib;


WBExportFullPDF::WBExportFullPDF(QObject *parent)
    : WBExportAdaptor(parent)
{
    //need to calculate screen resolution
    QDesktopWidget* desktop = WBApplication::desktop();
    int dpiCommon = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
    mScaleFactor = 72.0f / dpiCommon; // 1pt = 1/72 inch

    mSimpleExporter = new WBExportPDF();
}


WBExportFullPDF::~WBExportFullPDF()
{
    // NOOP
}


void WBExportFullPDF::saveOverlayPdf(WBDocumentProxy* pDocumentProxy, const QString& filename)
{
    if (!pDocumentProxy || filename.length() == 0 || pDocumentProxy->pageCount() == 0)
        return;

    mSimpleExporter->persistsDocument(pDocumentProxy, filename);
}


void WBExportFullPDF::persist(WBDocumentProxy* pDocumentProxy)
{
    persistLocally(pDocumentProxy, tr("Export as PDF File"));
}


bool WBExportFullPDF::persistsDocument(WBDocumentProxy* pDocumentProxy, const QString& filename)
{
    QFile file(filename);
    if (file.exists()) file.remove();

    QString overlayName = filename;
    overlayName.replace(".pdf", "_overlay.pdf");

    QFile previousOverlay(overlayName);
    if (previousOverlay.exists())
        previousOverlay.remove();

    mHasPDFBackgrounds = false;

    saveOverlayPdf(pDocumentProxy, overlayName);

    if (!mHasPDFBackgrounds)
    {
        QFile f(overlayName);
        f.rename(filename);
    }
    else
    {
        //Merger merger;
        //try
        //{
        //    merger.addOverlayDocument(QFile::encodeName(overlayName).constData());

        //    MergeDescription mergeInfo;

        //    int existingPageCount = pDocumentProxy->pageCount();

        //    for(int pageIndex = 0 ; pageIndex < existingPageCount; pageIndex++)
        //    {
        //        WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, pageIndex);
        //        WBGraphicsPDFItem *pdfItem = qgraphicsitem_cast<WBGraphicsPDFItem*>(scene->backgroundObject());

        //        QSize pageSize = scene->nominalSize();
        //        
        //        if (pdfItem)
        //        {
        //            QString pdfName = WBPersistenceManager::objectDirectory + "/" + pdfItem->fileUuid().toString() + ".pdf";
        //            QString backgroundPath = pDocumentProxy->persistencePath() + "/" + pdfName;
        //            QRectF annotationsRect = scene->annotationsBoundingRect();

        //            // Original datas
        //            double xAnnotation = qRound(annotationsRect.x());
        //            double yAnnotation = qRound(annotationsRect.y());
        //            double xPdf = qRound(pdfItem->sceneBoundingRect().x());
        //            double yPdf = qRound(pdfItem->sceneBoundingRect().y());
        //            double hPdf = qRound(pdfItem->sceneBoundingRect().height());

        //            // Exportation-transformed datas
        //            double hScaleFactor = pageSize.width()/annotationsRect.width();
        //            double vScaleFactor = pageSize.height()/annotationsRect.height();
        //            double scaleFactor = qMin(hScaleFactor, vScaleFactor);

        //            double xAnnotationsOffset = 0;
        //            double yAnnotationsOffset = 0;
        //            double hPdfTransformed = qRound(hPdf * scaleFactor);

        //            // Here, we force the PDF page to be on the topleft corner of the page
        //            double xPdfOffset = 0;
        //            double yPdfOffset = (hPdf - hPdfTransformed) * mScaleFactor;

        //            // Now we align the items
        //            xPdfOffset += (xPdf - xAnnotation) * scaleFactor * mScaleFactor;
        //            yPdfOffset -= (yPdf - yAnnotation) * scaleFactor * mScaleFactor;

        //            // If the PDF was scaled when added to the scene (e.g if it was loaded from a document with a different DPI
        //            // than the current one), it should also be scaled here.
        //            qreal pdfScale = pdfItem->scale();

        //            TransformationDescription pdfTransform(xPdfOffset, yPdfOffset, scaleFactor * pdfScale, 0);
        //            TransformationDescription annotationTransform(xAnnotationsOffset, yAnnotationsOffset, 1, 0);

        //            MergePageDescription pageDescription(pageSize.width() * mScaleFactor,
        //                                                 pageSize.height() * mScaleFactor,
        //                                                 pdfItem->pageNumber(),
        //                                                 QFile::encodeName(backgroundPath).constData(),
        //                                                 pdfTransform,
        //                                                 pageIndex + 1,
        //                                                 annotationTransform,
        //                                                 false, false);

        //            mergeInfo.push_back(pageDescription);

        //            merger.addBaseDocument(QFile::encodeName(backgroundPath).constData());
        //        }
        //        else
        //        {
        //            MergePageDescription pageDescription(pageSize.width() * mScaleFactor,
        //                     pageSize.height() * mScaleFactor,
        //                     0,
        //                     "",
        //                     TransformationDescription(),
        //                     pageIndex + 1,
        //                     TransformationDescription(),
        //                     false, true);

        //            mergeInfo.push_back(pageDescription);
        //        }
        //    }

        //    merger.merge(QFile::encodeName(overlayName).constData(), mergeInfo);

        //    merger.saveMergedDocumentsAs(QFile::encodeName(filename).constData());

        //}
        //catch(Exception e)
        //{
        //    qDebug() << "PdfMerger failed to merge documents to " << filename << " - Exception : " << e.what();

        //    // default to raster export
        //    mSimpleExporter->persistsDocument(pDocumentProxy, filename);
        //}

        if (!WBApplication::app()->isVerbose())
        {
            QFile::remove(overlayName);
        }
    }

    return true;
}

bool WBExportFullPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const WBDocumentTreeModel *docModel = qobject_cast<const WBDocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}


QString WBExportFullPDF::exportExtention()
{
    return QString(".pdf");
}

QString WBExportFullPDF::exportName()
{
    return tr("Export to PDF");
}
