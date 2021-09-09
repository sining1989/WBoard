#include "WBResources.h"

#include <QtWidgets>

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "frameworks/WBFileSystemUtils.h"
#include "core/memcheck.h"


WBResources* WBResources::sSingleton = 0;

WBResources::WBResources(QObject* pParent)
 : QObject(pParent)
{
    // NOOP
}

WBResources::~WBResources()
{
    // NOOP
}

WBResources* WBResources::resources()
{
    if (!sSingleton)
    {
        sSingleton = new WBResources(WBApplication::staticMemoryCleaner);
        sSingleton->init();
        sSingleton->buildFontList();
    }

    return sSingleton;

}

void WBResources::init()
{
    // Cursors
    penCursor       = QCursor(Qt::CrossCursor);
    eraserCursor    = QCursor(QPixmap(":/images/cursors/eraser.png"), 5, 25);
    markerCursor    = QCursor(QPixmap(":/images/cursors/marker.png"), 3, 30);
    pointerCursor   = QCursor(QPixmap(":/images/cursors/laser.png"), 2, 1);
    handCursor      = QCursor(Qt::OpenHandCursor);
    zoomInCursor    = QCursor(QPixmap(":/images/cursors/zoomIn.png"), 9, 9);
    zoomOutCursor   = QCursor(QPixmap(":/images/cursors/zoomOut.png"), 9, 9);
    arrowCursor     = QCursor(Qt::ArrowCursor);
    playCursor      = QCursor(QPixmap(":/images/cursors/play.png"), 6, 1);
    textCursor      = QCursor(Qt::ArrowCursor);
    rotateCursor    = QCursor(QPixmap(":/images/cursors/rotate.png"), 16, 16);
    drawLineRulerCursor = QCursor(QPixmap(":/images/cursors/drawRulerLine.png"), 3, 12);
}

void WBResources::buildFontList()
{
    QString customFontDirectory = WBSettings::settings()->applicationCustomFontDirectory();
    QStringList fontFiles = WBFileSystemUtils::allFiles(customFontDirectory);
    foreach(QString fontFile, fontFiles){
        int fontId = QFontDatabase::addApplicationFont(fontFile);
        mCustomFontList << QFontDatabase::applicationFontFamilies(fontId);
    }
}
