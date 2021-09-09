#include "WBThumbnailAdaptor.h"

#include <QtCore>

#include "frameworks/WBFileSystemUtils.h"

#include "core/WBPersistenceManager.h"
#include "core/WBApplication.h"
#include "core/WBSettings.h"

#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"

#include "document/WBDocumentProxy.h"

#include "domain/WBGraphicsScene.h"

#include "WBSvgSubsetAdaptor.h"

#include "core/memcheck.h"

void WBThumbnailAdaptor::generateMissingThumbnails(WBDocumentProxy* proxy)
{
    int existingPageCount = proxy->pageCount();

    for (int iPageNo = 0; iPageNo < existingPageCount; ++iPageNo)
    {
        QString thumbFileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", iPageNo);

        QFile thumbFile(thumbFileName);

        if (!thumbFile.exists())
        {
            bool displayMessage = (existingPageCount > 5);

            int thumbCount = 0;

            WBGraphicsScene* scene = WBSvgSubsetAdaptor::loadScene(proxy, iPageNo);

            if (scene)
            {
                thumbCount++;

                if (displayMessage && thumbCount == 1)
                    WBApplication::showMessage(tr("Generating preview thumbnails ..."));

                persistScene(proxy, scene, iPageNo);
            }

            if (displayMessage && thumbCount > 0)
                WBApplication::showMessage(tr("%1 thumbnails generated ...").arg(thumbCount));

        }
    }
}

const QPixmap* WBThumbnailAdaptor::get(WBDocumentProxy* proxy, int pageIndex)
{
    QString fileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    QFile file(fileName);
    if (!file.exists())
    {
        generateMissingThumbnails(proxy);
    }

    QPixmap* pix = new QPixmap();
    if (file.exists())
    {
        //Warning. Works only with modified Qt
#ifdef Q_OS_LINUX
        pix->load(fileName, 0, Qt::AutoColor);
#else
        pix->load(fileName, 0, Qt::AutoColor);
#endif
    }
    return pix;
}

void WBThumbnailAdaptor::load(WBDocumentProxy* proxy, QList<const QPixmap*>& list)
{
    generateMissingThumbnails(proxy);

    foreach(const QPixmap* pm, list){
        delete pm;
        pm = NULL;
    }
    list.clear();
    for(int i=0; i<proxy->pageCount(); i++)
        list.append(get(proxy, i));
}

void WBThumbnailAdaptor::persistScene(WBDocumentProxy* proxy, WBGraphicsScene* pScene, int pageIndex, bool overrideModified)
{
    QString fileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    QFile thumbFile(fileName);

    if (pScene->isModified() || overrideModified || !thumbFile.exists())
    {
        qreal nominalWidth = pScene->nominalSize().width();
        qreal nominalHeight = pScene->nominalSize().height();
        qreal ratio = nominalWidth / nominalHeight;
        QRectF sceneRect = pScene->normalizedSceneRect(ratio);

        qreal width = WBSettings::maxThumbnailWidth;
        qreal height = width / ratio;

        QImage thumb(width, height, QImage::Format_ARGB32);

        QRectF imageRect(0, 0, width, height);

        QPainter painter(&thumb);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (pScene->isDarkBackground())
        {
            painter.fillRect(imageRect, Qt::black);
        }
        else
        {
            painter.fillRect(imageRect, Qt::white);
        }

        pScene->setRenderingContext(WBGraphicsScene::NonScreen);
        pScene->setRenderingQuality(WBItem::RenderingQualityHigh);

        pScene->render(&painter, imageRect, sceneRect, Qt::KeepAspectRatio);

        pScene->setRenderingContext(WBGraphicsScene::Screen);
        pScene->setRenderingQuality(WBItem::RenderingQualityNormal);

        thumb.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(fileName, "JPG");
    }
}


QUrl WBThumbnailAdaptor::thumbnailUrl(WBDocumentProxy* proxy, int pageIndex)
{
    QString fileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    return QUrl::fromLocalFile(fileName);
}
