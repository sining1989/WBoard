#include "WBImportImage.h"

#include "document/WBDocumentProxy.h"

#include "board/WBBoardController.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBDocumentManager.h"

#include "domain/WBGraphicsPixmapItem.h"

#include "pdf/PDFRenderer.h"

#include "core/memcheck.h"

WBImportImage::WBImportImage(QObject *parent)
    : WBPageBasedImportAdaptor(parent)
{
    // NOOP
}


WBImportImage::~WBImportImage()
{
    // NOOP
}


QStringList WBImportImage::supportedExtentions()
{
    QStringList formats;

    for ( int i = 0; i < QImageReader::supportedImageFormats().count(); ++i )
    {
            formats << QString(QImageReader::supportedImageFormats().at(i)).toLower();
    }

    return formats;
}


QString WBImportImage::importFileFilter()
{
    QString filter = tr("Image Format (");
    QStringList formats = supportedExtentions();
    bool isFirst = true;

    foreach(QString format, formats)
    {
            if(isFirst)
                    isFirst = false;
            else
                    filter.append(" ");

        filter.append("*."+format);
    }

    filter.append(")");

    return filter;
}

QList<WBGraphicsItem*> WBImportImage::import(const QUuid& uuid, const QString& filePath)
{
    Q_UNUSED(uuid);
    QList<WBGraphicsItem*> result;

    QPixmap pix(filePath);
    if (pix.isNull())
        return result;

    WBGraphicsPixmapItem* pixmapItem = new WBGraphicsPixmapItem();
    pixmapItem->setPixmap(pix);
    result << pixmapItem;

    return result;
}

void WBImportImage::placeImportedItemToScene(WBGraphicsScene* scene, WBGraphicsItem* item)
{
    WBGraphicsPixmapItem* pixmapItem = (WBGraphicsPixmapItem*)item;

     WBGraphicsPixmapItem* sceneItem = scene->addPixmap(pixmapItem->pixmap(), NULL, QPointF(0, 0),1.0,false,true);
     scene->setAsBackgroundObject(sceneItem, true);

     // Only stored pixmap, should be deleted now
     delete pixmapItem;
}

const QString& WBImportImage::folderToCopy()
{
    static QString f("");
    return f;
}
