#include "WBWidgetUniboardAPI.h"

#include <QWebEngineView>
#include <QDomDocument>
#include <QtWidgets>

#include "core/WB.h"
#include "core/WBApplication.h"
#include "core/WBSettings.h"

#include "document/WBDocumentProxy.h"

#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"
#include "board/WBBoardPaletteManager.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsWidgetItem.h"

#include "adaptors/WBThumbnailAdaptor.h"

#include "WBWidgetMessageAPI.h"
#include "frameworks/WBFileSystemUtils.h"
#include "core/WBDownloadManager.h"

#include "core/memcheck.h"

//Known extentions for files, add if you know more supported
const QString audioExtentions = ".mp3.wma.ogg";
const QString videoExtentions = ".avi.flv";
const QString imageExtentions = ".png.jpg.tif.bmp.tga";
const QString htmlExtentions = ".htm.html.xhtml";

//Allways use aliases instead of const char* itself
const QString imageAlias    = "image";
const QString imageAliasCap = "Image";
const QString videoAlias    = "video";
const QString videoAliasCap = "Video";
const QString audioAlias    = "audio";
const QString audioAliasCap = "Audio";

//Xml tag names
const QString tMainSection = "mimedata";
const QString tType = "type";
const QString tPath = "path";
const QString tMessage = "message";
const QString tReady = "ready";

const QString tMimeText = "text/plain";


//Name of path inside widget to store objects
const QString objectsPath = "objects";

WBWidgetUniboardAPI::WBWidgetUniboardAPI(WBGraphicsScene *pScene, WBGraphicsWidgetItem *widget)
    : QObject(pScene)
    , mScene(pScene)
    , mGraphicsWidget(widget)
    , mIsVisible(false)
    , mMessagesAPI(0)
    , mDatastoreAPI(0)
 {
    WBGraphicsW3CWidgetItem* w3CGraphicsWidget = dynamic_cast<WBGraphicsW3CWidgetItem*>(widget);

    if (w3CGraphicsWidget)
    {
        mMessagesAPI = new WBWidgetMessageAPI(w3CGraphicsWidget);
        mDatastoreAPI = new WBDatastoreAPI(w3CGraphicsWidget);
    }

    connect(WBDownloadManager::downloadManager(), SIGNAL(downloadFinished(bool,sDownloadFileDesc,QByteArray)), this, SLOT(onDownloadFinished(bool,sDownloadFileDesc,QByteArray)));
}


WBWidgetUniboardAPI::~WBWidgetUniboardAPI()
{
    // NOOP
}

QObject* WBWidgetUniboardAPI::messages()
{
    return mMessagesAPI;
}


QObject* WBWidgetUniboardAPI::datastore()
{
    return mDatastoreAPI;
}


void WBWidgetUniboardAPI::setTool(const QString& toolString)
{
    if (WBApplication::boardController->activeScene() != mScene)
        return;

    const QString lower = toolString.toLower();

    if (lower == "pen")
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Pen);
    }
    else if (lower == "marker")
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Marker);
    }
    else if (lower == "arrow")
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
    }
    else if (lower == "play")
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Play);
    }
    else if (lower == "line")
    {
        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Line);
    }
}


void WBWidgetUniboardAPI::setPenColor(const QString& penColor)
{
    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBSettings* settings = WBSettings::settings();

    bool conversionState = false;

    int index = penColor.toInt(&conversionState) - 1;

    if (conversionState && index > 0 && index <= 4)
    {
        WBApplication::boardController->setPenColorOnDarkBackground(settings->penColors(true).at(index - 1));
        WBApplication::boardController->setPenColorOnLightBackground(settings->penColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor.setNamedColor(penColor);
        if (svgColor.isValid())
        {
            WBApplication::boardController->setPenColorOnDarkBackground(svgColor);
            WBApplication::boardController->setPenColorOnLightBackground(svgColor);
        }
    }
}


void WBWidgetUniboardAPI::setMarkerColor(const QString& penColor)
{
    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBSettings* settings = WBSettings::settings();

    bool conversionState = false;

    int index = penColor.toInt(&conversionState);

    if (conversionState && index > 0 && index <= 4)
    {
        WBApplication::boardController->setMarkerColorOnDarkBackground(settings->markerColors(true).at(index - 1));
        WBApplication::boardController->setMarkerColorOnLightBackground(settings->markerColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor.setNamedColor(penColor);
        if (svgColor.isValid())
        {
            WBApplication::boardController->setMarkerColorOnDarkBackground(svgColor);
            WBApplication::boardController->setMarkerColorOnLightBackground(svgColor);
        }
    }
}


void WBWidgetUniboardAPI::addObject(QString pUrl, int width, int height, int x, int y, bool background)
{
    // not great, but make it easily scriptable --
    //
    // download url should be moved to the scene from the controller
    //

    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBApplication::boardController->downloadURL(QUrl(pUrl), QString(), QPointF(x, y), QSize(width, height), background);

}


void WBWidgetUniboardAPI::setBackground(bool pIsDark, bool pIsCrossed)
{
    if (mScene) {
        if (pIsCrossed)
            mScene->setBackground(pIsDark, WBPageBackground::crossed);
        else
            mScene->setBackground(pIsDark, WBPageBackground::plain);
    }
}


void WBWidgetUniboardAPI::moveTo(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (mScene)
    mScene->moveTo(QPointF(x, y));
}


void WBWidgetUniboardAPI::drawLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth)
        || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
        return;

    if (mScene)
    mScene->drawLineTo(QPointF(x, y), pWidth, 
        WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Line);
}


void WBWidgetUniboardAPI::eraseLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth)
       || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
       return;

    if (mScene)
    mScene->eraseLineTo(QPointF(x, y), pWidth);
}


void WBWidgetUniboardAPI::clear()
{
    if (mScene)
            mScene->clearContent(WBGraphicsScene::clearItemsAndAnnotations);
}


void WBWidgetUniboardAPI::zoom(const qreal factor, const qreal x, const qreal y)
{
   if (qIsNaN(factor) || qIsNaN(x) || qIsNaN(y)
       || qIsInf(factor) || qIsInf(x) || qIsInf(y))
       return;


    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBApplication::boardController->zoom(factor, QPointF(x, y));
}


void WBWidgetUniboardAPI::centerOn(const qreal x, const qreal y)
{
   if (qIsNaN(x) || qIsNaN(y)
       || qIsInf(x) || qIsInf(y))
       return;

    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBApplication::boardController->centerOn(QPointF(x, y));
}


void WBWidgetUniboardAPI::move(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (WBApplication::boardController->activeScene() != mScene)
        return;

    WBApplication::boardController->handScroll(x, y);
}


void WBWidgetUniboardAPI::addText(const QString& text, const qreal x, const qreal y, const int size, const QString& font
        , bool bold, bool italic)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (WBApplication::boardController->activeScene() != mScene)
        return;

    if (mScene)
        mScene->addTextWithFont(text, QPointF(x, y), size, font, bold, italic);

}


int WBWidgetUniboardAPI::pageCount()
{
    if (mScene && mScene->document())
        return mScene->document()->pageCount();
    else
        return -1;
}


int WBWidgetUniboardAPI::currentPageNumber()
{
    if (WBApplication::boardController->activeScene() != mScene)
        return -1;

    return WBApplication::boardController->activeSceneIndex() + 1;
}

QString WBWidgetUniboardAPI::getObjDir()
{
    return mGraphicsWidget->getOwnFolder().toLocalFile() + "/" + objectsPath + "/";
}

void WBWidgetUniboardAPI::showMessage(const QString& message)
{
    WBApplication::boardController->showMessage(message, false);
}


QString WBWidgetUniboardAPI::pageThumbnail(const int pageNumber)
{
    if (WBApplication::boardController->activeScene() != mScene)
        return "";

    WBDocumentProxy *doc = WBApplication::boardController->selectedDocument();

    if (!doc)
        return "";

    if (pageNumber > doc->pageCount())
        return "";

    QUrl url = WBThumbnailAdaptor::thumbnailUrl(doc, pageNumber - 1);

    return url.toString();

}


void WBWidgetUniboardAPI::resize(qreal width, qreal height)
{
    if (qIsNaN(width) || qIsNaN(height)
        || qIsInf(width) || qIsInf(height))
        return;

    if (mGraphicsWidget)
    {
        mGraphicsWidget->resize(width, height);
    }
}


void WBWidgetUniboardAPI::setPreference(const QString& key, QString value)
{
    if (mGraphicsWidget)
    {
            mGraphicsWidget->setPreference(key, value);
    }
}


QString WBWidgetUniboardAPI::preference(const QString& key , const QString& pDefault)
{
    if (mGraphicsWidget && mGraphicsWidget->preferences().contains(key))
    {
        return mGraphicsWidget->preference(key);
    }
    else
    {
        return pDefault;
    }
}


QStringList WBWidgetUniboardAPI::preferenceKeys()
{
    QStringList keys;

    if (mGraphicsWidget)
        keys = mGraphicsWidget->preferences().keys();

    return keys;
}


QString WBWidgetUniboardAPI::uuid()
{
    if (mGraphicsWidget)
        return WBStringUtils::toCanonicalUuid(mGraphicsWidget->uuid());
    else
        return "";
}


QString WBWidgetUniboardAPI::locale()
{
    return QLocale().name();
}

QString WBWidgetUniboardAPI::lang()
{
    QString lang = QLocale().name();

    if (lang.length() > 2)
        lang[2] = QLatin1Char('-');

    return lang;
}

void WBWidgetUniboardAPI::returnStatus(const QString& method, const QString& status)
{
    QString msg = QString(tr("%0 called (method=%1, status=%2)")).arg("returnStatus").arg(method).arg(status);
    WBApplication::showMessage(msg);
}

void WBWidgetUniboardAPI::usedMethods(QStringList methods)
{
    // TODO: Implement this method
    foreach(QString method, methods)
    {

    }
}

void WBWidgetUniboardAPI::response(bool correct)
{
    Q_UNUSED(correct);
    // TODO: Implement this method
}

void WBWidgetUniboardAPI::sendFileMetadata(QString metaData)
{
    //  Build the QMap of metadata and send it to application
    QMap<QString, QString> qmMetaDatas;
    QDomDocument domDoc;
    domDoc.setContent(metaData);
    QDomElement rootElem = domDoc.documentElement();
    QDomNodeList children = rootElem.childNodes();
    for(int i=0; i<children.size(); i++){
        QDomNode dataNode = children.at(i);
        QDomElement keyElem = dataNode.firstChildElement("key");
        QDomElement valueElem = dataNode.firstChildElement("value");
        qmMetaDatas[keyElem.text()] = valueElem.text();
    }
    WBApplication::boardController->displayMetaData(qmMetaDatas);
}

void WBWidgetUniboardAPI::enableDropOnWidget(bool enable)
{
    if (mGraphicsWidget)
    {
        mGraphicsWidget->setAcceptDrops(enable);
    }
}

void WBWidgetUniboardAPI::ProcessDropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QMimeData *pMimeData = event->mimeData();

    QString destFileName;
    QString contentType;
    bool downloaded = false;

    QGraphicsView *tmpView = mGraphicsWidget->scene()->views().at(0);
    QPoint dropPoint(mGraphicsWidget->mapFromScene(tmpView->mapToScene(event->pos().toPoint())).toPoint());
    Qt::DropActions dropActions = event->possibleActions();
    Qt::MouseButtons dropMouseButtons = event->buttons();
    Qt::KeyboardModifiers dropModifiers = event->modifiers();
    QMimeData *dropMimeData = new QMimeData;
    qDebug() << event->possibleActions();


    if (pMimeData->hasHtml()) { //Dropping element from web browser
        QString qsHtml = pMimeData->html();
        QString url = WBApplication::urlFromHtml(qsHtml);

        if(!url.isEmpty()) {
            QString str = "test string";

            QMimeData mimeData;
            mimeData.setData(tMimeText, str.toLatin1());

            sDownloadFileDesc desc;
            desc.dest = sDownloadFileDesc::graphicsWidget;
            desc.modal = true;
            desc.srcUrl = url;
            desc.currentSize = 0;
            desc.name = QFileInfo(url).fileName();
            desc.totalSize = 0; // The total size will be retrieved during the download

            desc.dropPoint = event->pos().toPoint(); //Passing pure event point. No modifications
            desc.dropActions = dropActions;
            desc.dropMouseButtons = dropMouseButtons;
            desc.dropModifiers = dropModifiers;

            registerIDWidget(WBDownloadManager::downloadManager()->addFileToDownload(desc));

        }

    } else  if (pMimeData->hasUrls()) { //Local file processing
        QUrl curUrl = pMimeData->urls().first();
        QString sUrl = curUrl.toString();

        if (sUrl.startsWith("file://") || sUrl.startsWith("/")) {
            QString fileName = curUrl.toLocalFile();
            QString extention = WBFileSystemUtils::extension(fileName);
            contentType = WBFileSystemUtils::mimeTypeFromFileName(fileName);

            if (supportedTypeHeader(contentType)) {
                destFileName = getObjDir() + QUuid::createUuid().toString() + "." + extention;

                if (!WBFileSystemUtils::copyFile(fileName, destFileName)) {
                    qDebug() << "can't copy from" << fileName << "to" << destFileName;
                    return;
                }
                downloaded = true;

            }
        }
    }
    qDebug() << destFileName;
    QString mimeText = createMimeText(downloaded, contentType, destFileName);
    dropMimeData->setData(tMimeText, mimeText.toLatin1());

    event->setMimeData(dropMimeData);
}

void WBWidgetUniboardAPI::onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData)
{
    //if widget recieves is waiting for this id then process
    if (!takeIDWidget(desc.id))
        return;

    if (!pSuccess) {
        qDebug() << "can't download the whole data. An error occured";
        return;
    }

    QString contentType = desc.contentTypeHeader;
    QString extention = WBFileSystemUtils::fileExtensionFromMimeType(contentType);

    if (!supportedTypeHeader(contentType)) {
        qDebug() << "actions for mime type" << contentType << "are not supported";
        return;
    }

    QString objDir = getObjDir();
    if (!QDir().exists(objDir)) {
        if (!QDir().mkpath(objDir)) {
            qDebug() << "can't create objects directory path. Check the permissions";
            return;
        }
    }

    QString destFileName = objDir + QUuid::createUuid().toString() + "." + extention;
    QFile destFile(destFileName);

    if (!destFile.open(QIODevice::WriteOnly)) {
        qDebug() << "can't open" << destFileName << "for wrighting";
        return;
    }

    if (destFile.write(pData) == -1) {
        qDebug() << "can't implement data writing";
        return;
    }

    QGraphicsView *tmpView = mGraphicsWidget->scene()->views().at(0);
    QPoint dropPoint(mGraphicsWidget->mapFromScene(tmpView->mapToScene(desc.dropPoint)).toPoint());

    QMimeData dropMimeData;
    QString mimeText = createMimeText(true, contentType, destFileName);
    dropMimeData.setData(tMimeText, mimeText.toLatin1());

    destFile.close();

    //To make js interpreter accept drop event we need to generate move event first.
    QDragMoveEvent pseudoMove(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    QApplication::sendEvent(mGraphicsWidget,&pseudoMove);

    QDropEvent readyEvent(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    //sending event to destination either it had been downloaded or not
    QApplication::sendEvent(mGraphicsWidget,&readyEvent);
    readyEvent.acceptProposedAction();
}

QString WBWidgetUniboardAPI::createMimeText(bool downloaded, const QString &mimeType, const QString &fileName)
{
    QString mimeXml;
    QXmlStreamWriter writer(&mimeXml);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(tMainSection);

    writer.writeTextElement(tReady, boolToStr(downloaded));

    if (downloaded) {
        if (!mimeType.isEmpty()) {
            writer.writeTextElement(tType, mimeType);  //writing type of element
        }
        if (!QFile::exists(fileName)) {
            qDebug() << "file" << fileName << "doesn't exist";
            return QString();
        }

        QString relatedFileName = fileName;
        relatedFileName = relatedFileName.remove(mGraphicsWidget->getOwnFolder().toLocalFile() + "/");
        writer.writeTextElement(tPath, relatedFileName);   //writing path to created object
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    return mimeXml;
}

bool WBWidgetUniboardAPI::supportedTypeHeader(const QString &typeHeader) const
{
    return typeHeader.startsWith(imageAlias) || typeHeader.startsWith(imageAliasCap)
            || typeHeader.startsWith(audioAlias) || typeHeader.startsWith(audioAliasCap)
            || typeHeader.startsWith(videoAlias) || typeHeader.startsWith(videoAliasCap);
}

bool WBWidgetUniboardAPI::takeIDWidget(int id)
{
    if (webDownloadIds.contains(id)) {
        webDownloadIds.removeAll(id);
        return true;
    }
    return false;
}

bool WBWidgetUniboardAPI::isDropableData(const QMimeData *pMimeData) const
{
    QString fileName = QString();
    if (pMimeData->hasHtml()) {
        fileName = WBApplication::urlFromHtml(pMimeData->html());
        if (fileName.isEmpty())
            return false;
    } else if (pMimeData->hasUrls()) {
        fileName = pMimeData->urls().at(0).toLocalFile();
        if (fileName.isEmpty())
            return false;
    }

    if (supportedTypeHeader(WBFileSystemUtils::mimeTypeFromFileName(fileName)))
        return true;

    return false;
}


WBDocumentDatastoreAPI::WBDocumentDatastoreAPI(WBGraphicsW3CWidgetItem *graphicsWidget)
    : WBW3CWebStorage(graphicsWidget)
    , mGraphicsW3CWidget(graphicsWidget)
{
    // NOOP
}



WBDocumentDatastoreAPI::~WBDocumentDatastoreAPI()
{
    // NOOP
}


QString WBDocumentDatastoreAPI::key(int index)
{
   QMap<QString, QString> entries = mGraphicsW3CWidget->datastoreEntries();

   if (index < entries.size())
       return entries.keys().at(index);
   else
       return "";

}


QString WBDocumentDatastoreAPI::getItem(const QString& key)
{
    QMap<QString, QString> entries = mGraphicsW3CWidget->datastoreEntries();
    if (entries.contains(key))
    {
        return entries.value(key);
    }
    else
    {
        return "";
    }
}


int WBDocumentDatastoreAPI::length()
{
   return mGraphicsW3CWidget->datastoreEntries().size();
}


void WBDocumentDatastoreAPI::setItem(const QString& key, const QString& value)
{
    if (mGraphicsW3CWidget)
    {
        mGraphicsW3CWidget->setDatastoreEntry(key, value);
    }
}


void WBDocumentDatastoreAPI::removeItem(const QString& key)
{
    mGraphicsW3CWidget->removeDatastoreEntry(key);
}
void

 WBDocumentDatastoreAPI::clear()
{
    mGraphicsW3CWidget->removeAllDatastoreEntries();
}


WBDatastoreAPI::WBDatastoreAPI(WBGraphicsW3CWidgetItem *widget)
    : QObject(widget)
{
    mDocumentDatastore = new WBDocumentDatastoreAPI(widget);
}


QObject* WBDatastoreAPI::document()
{
    return mDocumentDatastore;
}




