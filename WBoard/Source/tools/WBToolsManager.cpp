#include "WBToolsManager.h"

#include "core/memcheck.h"

WBToolsManager* WBToolsManager::sManager = 0;

WBToolsManager* WBToolsManager::manager()
{
    if (!sManager)
        sManager = new WBToolsManager(WBApplication::staticMemoryCleaner);
    return sManager;
}

void WBToolsManager::destroy()
{
    if (sManager)
        delete sManager;
    sManager = NULL;
}


WBToolsManager::WBToolsManager(QObject *parent)
    : QObject(parent)
{

    mask.id = "wboardtool://wboard/mask";
    mask.icon = QPixmap(":/images/toolPalette/maskTool.png");
    mask.label = tr("Mask");
    mask.version = "1.0";
    mToolsIcon.insert(mask.id, ":/images/toolPalette/maskTool.png");
    mDescriptors << mask;


    ruler.id = "wboardtool://ruler";
    ruler.icon = QPixmap(":/images/toolPalette/rulerTool.png");
    ruler.label = tr("Ruler");
    ruler.version = "1.0";
    mToolsIcon.insert(ruler.id, ":/images/toolPalette/rulerTool.png");
    mDescriptors << ruler;


    compass.id = "wboardtool://compass";
    compass.icon = QPixmap(":/images/toolPalette/compassTool.png");
    compass.label = tr("Compass");
    compass.version = "1.0";
    mToolsIcon.insert(compass.id, ":/images/toolPalette/compassTool.png");
    mDescriptors << compass;


    protractor.id = "wboardtool://protractor";
    protractor.icon = QPixmap(":/images/toolPalette/protractorTool.png");
    protractor.label = tr("Protractor");
    protractor.version = "1.0";
    mToolsIcon.insert(protractor.id,":/images/toolPalette/protractorTool.png");
    mDescriptors << protractor;


    triangle.id = "wboardtool://triangle";
    triangle.icon = QPixmap(":/images/toolPalette/triangleTool.png");
    triangle.label = tr("Triangle");
    triangle.version = "1.0";
    mToolsIcon.insert(triangle.id,":/images/toolPalette/triangleTool.png");
    mDescriptors << triangle;


    magnifier.id = "wboardtool://magnifier";
    magnifier.icon = QPixmap(":/images/toolPalette/magnifierTool.png");
    magnifier.label = tr("Magnifier");
    magnifier.version = "1.0";
    mToolsIcon.insert(magnifier.id,":/images/toolPalette/magnifierTool.png");
    mDescriptors << magnifier;


    cache.id = "wboardtool://cache";
    cache.icon = QPixmap(":/images/toolPalette/cacheTool.png");
    cache.label = tr("Cache");
    cache.version = "1.0";
    mToolsIcon.insert(cache.id, ":/images/toolPalette/cacheTool.png");
    mDescriptors << cache;

}

WBToolsManager::~WBToolsManager()
{
    // NOOP
}
