#include "WBExportPDF.h"

#include <QtCore>
#include <QtSvg>
#include <QPrinter>
#include <QPdfWriter>

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

#include "core/memcheck.h"

WBExportPDF::WBExportPDF(QObject *parent)
    : WBExportAdaptor(parent)
{
    // NOOP
}

WBExportPDF::~WBExportPDF()
{
    // NOOP
}

void WBExportPDF::persist(WBDocumentProxy* pDocumentProxy)
{
    persistLocally(pDocumentProxy, tr("Export as PDF File"));
}

bool WBExportPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const WBDocumentTreeModel *docModel = qobject_cast<const WBDocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}


bool WBExportPDF::persistsDocument(WBDocumentProxy* pDocumentProxy, const QString& filename)
{
    QPdfWriter pdfWriter(filename);

    qDebug() << "exporting document to PDF" << filename;

    pdfWriter.setResolution(WBSettings::settings()->pdfResolution->get().toInt());
    pdfWriter.setPageMargins(QMarginsF());
    pdfWriter.setTitle(pDocumentProxy->name());
    pdfWriter.setCreator("WBoard PDF export");

    //need to calculate screen resolution
    QDesktopWidget* desktop = WBApplication::desktop();
    int dpiCommon = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
    float scaleFactor = 72.0f / dpiCommon;

    QPainter pdfPainter;
    bool painterNeedsBegin = true;

    int existingPageCount = pDocumentProxy->pageCount();

    for(int pageIndex = 0 ; pageIndex < existingPageCount; pageIndex++) {

        WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, pageIndex);
        WBApplication::showMessage(tr("Exporting page %1 of %2").arg(pageIndex + 1).arg(existingPageCount));

        // set background to white, no crossing for PDF output
        bool isDark = scene->isDarkBackground();
        WBPageBackground pageBackground = scene->pageBackground();
        scene->setBackground(false, WBPageBackground::plain);

        // pageSize is the output PDF page size; it is set to equal the scene's boundary size; if the contents
        // of the scene overflow from the boundaries, they will be scaled down.
        QSize pageSize = scene->sceneSize();

        // set high res rendering
        scene->setRenderingQuality(WBItem::RenderingQualityHigh);
        scene->setRenderingContext(WBGraphicsScene::NonScreen);

        // Setting output page size
        QPageSize outputPageSize = QPageSize(QSizeF(pageSize.width()*scaleFactor, pageSize.height()*scaleFactor), QPageSize::Point);
        pdfWriter.setPageSize(outputPageSize);

        // Call begin only once
        if(painterNeedsBegin)
            painterNeedsBegin = !pdfPainter.begin(&pdfWriter);

        else if (pageIndex < existingPageCount)
            pdfWriter.newPage();

        // Render the scene
        scene->render(&pdfPainter, QRectF(), scene->normalizedSceneRect());

        // Restore screen rendering quality
        scene->setRenderingContext(WBGraphicsScene::Screen);
        scene->setRenderingQuality(WBItem::RenderingQualityNormal);

        // Restore background state
        scene->setBackground(isDark, pageBackground);
    }

    if(!painterNeedsBegin)
        pdfPainter.end();

    return true;
}

QString WBExportPDF::exportExtention()
{
    return QString(".pdf");
}

QString WBExportPDF::exportName()
{
    return tr("Export to PDF");
}
