#include <QtNetwork>
#include <QtXml>
#include <QtWebChannel/QWebChannel>

#include "WBGraphicsWidgetItem.h"
#include "WBGraphicsScene.h"
#include "WBGraphicsItemDelegate.h"
#include "WBGraphicsWidgetItemDelegate.h"
#include "WBGraphicsDelegateFrame.h"

#include "api/WBWidgetUniboardAPI.h"
#include "api/WBW3CWidgetAPI.h"

 #include "board/WBBoardController.h"

#include "core/memcheck.h"
#include "core/WBApplicationController.h"
#include "core/WBApplication.h"
#include "core/WBSettings.h"

#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "network/WBNetworkAccessManager.h"

#include "web/WBWebPage_.h"
#include "web/WBWebKitUtils.h"
#include "web/WBWebController.h"

bool WBGraphicsWidgetItem::sInlineJavaScriptLoaded = false;
QStringList WBGraphicsWidgetItem::sInlineJavaScripts;

WBGraphicsWidgetItem::WBGraphicsWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , mInitialLoadDone(false)
    , mIsFreezable(true)
    , mIsResizable(false)
    , mLoadIsErronous(false)
    , mCanBeContent(0)
    , mCanBeTool(0)
    , mWidgetUrl(pWidgetUrl)
    , mIsFrozen(false)
    , mIsTakingSnapshot(false)
    , mShouldMoveWidget(false)
    , mUniboardAPI(0)
{
    setData(WBGraphicsItemData::ItemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
	//QWebEngineView
	//QGraphicsWidget::setGraphicsEffect
 //   QGraphicsView::setPage(new WBWebPage(this));
	//QGraphicsWidget::setAttribute(QWebEngineSettings::JavaEnabled, true); 
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    //page()->setNetworkAccessManager(WBNetworkAccessManager::defaultAccessManager());

    setAcceptDrops(true);
    setAutoFillBackground(false);

    //QPalette pagePalette = page()->palette();
    //pagePalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    //pagePalette.setBrush(QPalette::Window, QBrush(Qt::transparent));
    //page()->setPalette(pagePalette);

    QPalette viewPalette = palette();
    //pagePalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    viewPalette.setBrush(QPalette::Window, QBrush(Qt::transparent));
    setPalette(viewPalette);

    setDelegate(new WBGraphicsWidgetItemDelegate(this));

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    QGraphicsWidget::setAcceptHoverEvents(true);
}


WBGraphicsWidgetItem::~WBGraphicsWidgetItem()
{
    /* NOOP */
}

void WBGraphicsWidgetItem::initialize()
{
    setMinimumSize(nominalSize());
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); // Necessary to set if we want z value to be assigned correctly

    if (Delegate() && Delegate()->frame() && resizable())
        Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::Resizing);

   /* QPalette palette = page()->palette();
    palette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    page()->setPalette(palette);
    page()->setLinkDelegationPolicy(QWebEnginePage::DelegateAllLinks);*/

    //connect(page()->mainFrame(), &QWebChannel::javaScriptWindowObjectCleared,
    //        this, &WBGraphicsWidgetItem::javaScriptWindowObjectCleared);
    //connect(page(), SIGNAL(geometryChangeRequested(const QRect&)), this, SLOT(geometryChangeRequested(const QRect&)));
    //connect(this, SIGNAL(loadFinished(bool)), this, SLOT(mainFrameLoadFinished (bool)));
    //connect(page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(initialLayoutCompleted()));
    //connect(page(), SIGNAL(linkClicked(const QUrl&)), this, SLOT(onLinkClicked(const QUrl&)));
}

void WBGraphicsWidgetItem::onLinkClicked(const QUrl& url)
{
    //load(url);
}

void WBGraphicsWidgetItem::initialLayoutCompleted()
{
    mInitialLoadDone = true;
}

QUrl WBGraphicsWidgetItem::mainHtml()
{
    return mMainHtmlUrl;
}

void WBGraphicsWidgetItem::loadMainHtml()
{
    mInitialLoadDone = false;
    //load(mMainHtmlUrl);
}

QUrl WBGraphicsWidgetItem::widgetUrl()
{
    return mWidgetUrl;
}

QString WBGraphicsWidgetItem::mainHtmlFileName()
{
    return mMainHtmlFileName;
}

bool WBGraphicsWidgetItem::canBeContent()
{
    // if we under MAC OS
    #if defined(Q_OS_MAC)
        return mCanBeContent & WBGraphicsWidgetItem::type_MAC;
    #endif

    // if we under UNIX OS
    #if defined(Q_OS_UNIX)
        return mCanBeContent & WBGraphicsWidgetItem::type_UNIX;
    #endif

    // if we under WINDOWS OS
    #if defined(Q_OS_WIN)
        return mCanBeContent & WBGraphicsWidgetItem::type_WIN;
    #endif
}

bool WBGraphicsWidgetItem::canBeTool()
{
    // if we under MAC OS
    #if defined(Q_OS_MAC)
        return mCanBeTool & WBGraphicsWidgetItem::type_MAC;
    #endif

        // if we under UNIX OS
    #if defined(Q_OS_UNIX)
        return mCanBeTool & WBGraphicsWidgetItem::type_UNIX;
    #endif

        // if we under WINDOWS OS
    #if defined(Q_OS_WIN)
        return mCanBeTool & WBGraphicsWidgetItem::type_WIN;
    #endif
}

QString WBGraphicsWidgetItem::preference(const QString& key) const
{
    return mPreferences.value(key);
}

void WBGraphicsWidgetItem::setPreference(const QString& key, QString value)
{
    if (key == "" || (mPreferences.contains(key) && mPreferences.value(key) == value))
        return;

    mPreferences.insert(key, value);
    if (scene())
        scene()->setModified(true);
}

QMap<QString, QString> WBGraphicsWidgetItem::preferences() const
{
    return mPreferences;
}


void WBGraphicsWidgetItem::removePreference(const QString& key)
{
    mPreferences.remove(key);
}


void WBGraphicsWidgetItem::removeAllPreferences()
{
    mPreferences.clear();
}

QString WBGraphicsWidgetItem::datastoreEntry(const QString& key) const
{
    if (mDatastore.contains(key))
        return mDatastore.value(key);
    else
        return QString();
}

void WBGraphicsWidgetItem::setDatastoreEntry(const QString& key, QString value)
{
    if (key == "" || (mDatastore.contains(key) && mDatastore.value(key) == value))
        return;

    mDatastore.insert(key, value);
    if (scene())
        scene()->setModified(true);
}

QMap<QString, QString> WBGraphicsWidgetItem::datastoreEntries() const
{
    return mDatastore;
}


void WBGraphicsWidgetItem::removeDatastoreEntry(const QString& key)
{
    mDatastore.remove(key);
}


void WBGraphicsWidgetItem::removeAllDatastoreEntries()
{
    mDatastore.clear();
}

void WBGraphicsWidgetItem::removeScript()
{
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onremove) { widget.onremove();}");
}

void WBGraphicsWidgetItem::processDropEvent(QGraphicsSceneDragDropEvent *event)
{
    mUniboardAPI->ProcessDropEvent(event);
}
bool WBGraphicsWidgetItem::isDropableData(const QMimeData *data) const
{
    return mUniboardAPI->isDropableData(data);
}

QUrl WBGraphicsWidgetItem::getOwnFolder() const
{
    return ownFolder;
}

void WBGraphicsWidgetItem::setOwnFolder(const QUrl &newFolder)
{
    ownFolder = newFolder;
}

void WBGraphicsWidgetItem::setSnapshotPath(const QUrl &newFilePath)
{
    SnapshotFile = newFilePath;
}

QUrl WBGraphicsWidgetItem::getSnapshotPath()
{
    return SnapshotFile;
}

void WBGraphicsWidgetItem::clearSource()
{
    WBFileSystemUtils::deleteDir(getOwnFolder().toLocalFile());
    WBFileSystemUtils::deleteFile(getSnapshotPath().toLocalFile());
}

void WBGraphicsWidgetItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

QSize WBGraphicsWidgetItem::nominalSize() const
{
    return mNominalSize;
}

bool WBGraphicsWidgetItem::hasLoadedSuccessfully() const
{
    return (mInitialLoadDone && !mLoadIsErronous);
}

bool WBGraphicsWidgetItem::freezable()
{
    return mIsFreezable;
}

bool WBGraphicsWidgetItem::resizable()
{
    return mIsResizable;
}

bool WBGraphicsWidgetItem::isFrozen()
{
    return mIsFrozen;
}

QPixmap WBGraphicsWidgetItem::snapshot()
{
    return mSnapshot;
}

QPixmap WBGraphicsWidgetItem::takeSnapshot()
{
    mIsTakingSnapshot = true;

    QPixmap pixmap(size().toSize());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    QStyleOptionGraphicsItem options;
    paint(&painter, &options);

    mIsTakingSnapshot = false;

    mSnapshot = pixmap;

    return pixmap;
}

void WBGraphicsWidgetItem::setSnapshot(const QPixmap& pix)
{
    mSnapshot = pix;
}

WBGraphicsScene* WBGraphicsWidgetItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}

int WBGraphicsWidgetItem::widgetType(const QUrl& pUrl)
{
    QString mime = WBFileSystemUtils::mimeTypeFromFileName(pUrl.toString());

    if (mime == "application/vnd.apple-widget")
        return WBWidgetType::Apple;
    else if (mime == "application/widget")
        return WBWidgetType::W3C;
    else
        return WBWidgetType::Other;
}

QString WBGraphicsWidgetItem::widgetName(const QUrl& widgetPath)
{
    QString name;
    QString version;
    QFile w3CConfigFile(widgetPath.toLocalFile() + "/config.xml");
    QFile appleConfigFile(widgetPath.toLocalFile() + "/Info.plist");

    if (w3CConfigFile.exists() && w3CConfigFile.open(QFile::ReadOnly)) {
        QDomDocument doc;
        doc.setContent(w3CConfigFile.readAll());
        QDomElement root = doc.firstChildElement("widget");
        if (!root.isNull()) {
            QDomElement nameElement = root.firstChildElement("name");
            if (!nameElement.isNull())
                name = nameElement.text();
            version = root.attribute("version", "");
        }
        w3CConfigFile.close();
    }
    else if (appleConfigFile.exists() && appleConfigFile.open(QFile::ReadOnly)) {
        QDomDocument doc;
        doc.setContent(appleConfigFile.readAll());
        QDomElement root = doc.firstChildElement("plist");
        if (!root.isNull()) {
            QDomElement dictElement = root.firstChildElement("dict");
            if (!dictElement.isNull()) {
                QDomNodeList childNodes  = dictElement.childNodes();

                /* looking for something like
                 * ..
                 * <key>CFBundleDisplayName</key>
                 * <string>brain scans</string>
                 * ..
                 */

                for(int i = 0; i < childNodes.count() - 1; i++) {
                    if (childNodes.at(i).isElement()) {
                        QDomElement elKey = childNodes.at(i).toElement();
                        if (elKey.text() == "CFBundleDisplayName") {
                            if (childNodes.at(i + 1).isElement()) {
                               QDomElement elValue = childNodes.at(i + 1).toElement();
                               name = elValue.text();
                            }
                        }
                        else if (elKey.text() == "CFBundleShortVersionString") {
                            if (childNodes.at(i + 1).isElement()) {
                               QDomElement elValue = childNodes.at(i + 1).toElement();
                               version = elValue.text();
                            }
                        }
                    }
                }
            }
        }
        appleConfigFile.close();
    }
    QString result;

    if (name.length() > 0) {
        result = name;
        if (version.length() > 0) {
            result += " ";
            result += version;
        }
    }
    return result;
}

QString WBGraphicsWidgetItem::iconFilePath(const QUrl& pUrl)
{
    QStringList files;

    files << "icon.svg";  /* W3C widget default 1 */
    files << "icon.ico";  /* W3C widget default 2 */
    files << "icon.png";  /* W3C widget default 3 */
    files << "icon.gif";  /* W3C widget default 4 */
    files << "Icon.png";  /* Apple widget default */

    QString file = WBFileSystemUtils::getFirstExistingFileFromList(pUrl.toLocalFile(), files);
    /* default */
    if (file.length() == 0)
    {
        file = QString(":/images/defaultWidgetIcon.png");
    }
    return file;
}

void WBGraphicsWidgetItem::freeze()
{
    QPixmap pix = takeSnapshot();
    mIsFrozen = true;
    setSnapshot(pix);
}

void WBGraphicsWidgetItem::unFreeze()
{
    mIsFrozen = false;
}

bool WBGraphicsWidgetItem::event(QEvent *event)
{
    if (mShouldMoveWidget && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent*>(event);
        if (mouseMoveEvent->buttons() & Qt::LeftButton) {
            QPointF scenePos = mapToScene(mouseMoveEvent->pos());
            QPointF newPos = pos() + scenePos - mLastMousePos;
            setPos(newPos);
            mLastMousePos = scenePos;
            event->accept();
            return true;
        }
    }
    else if (event->type() == QEvent::ShortcutOverride)
        event->accept();

	return true;//QGraphicsWebView::event(event);
}

void WBGraphicsWidgetItem::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    processDropEvent(event);
    //QGraphicsWebView::dropEvent(event);
}

void WBGraphicsWidgetItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!Delegate()->mousePressEvent(event))
        setSelected(true); /* forcing selection */

    QGraphicsWidget::mousePressEvent(event);

    // did webkit consume the mouse press ?
    mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);

    mLastMousePos = mapToScene(event->pos());

    event->accept();
}

void WBGraphicsWidgetItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    mShouldMoveWidget = false;

    Delegate()->mouseReleaseEvent(event);
	QGraphicsWidget::mouseReleaseEvent(event);
}

void WBGraphicsWidgetItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    sendJSEnterEvent();
    Delegate()->hoverEnterEvent(event);
}
void WBGraphicsWidgetItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    sendJSLeaveEvent();
    Delegate()->hoverLeaveEvent(event);
}

void WBGraphicsWidgetItem::sendJSEnterEvent()
{
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onenter) { widget.onenter();}");
}

void WBGraphicsWidgetItem::sendJSLeaveEvent()
{
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onleave) { widget.onleave();}");
}

void WBGraphicsWidgetItem::injectInlineJavaScript()
{
    if (!sInlineJavaScriptLoaded) {
        sInlineJavaScripts = WBApplication::applicationController->widgetInlineJavaScripts();
        sInlineJavaScriptLoaded = true;
    }

    //foreach(QString script, sInlineJavaScripts)
    //    page()->mainFrame()->evaluateJavaScript(script);
}

void WBGraphicsWidgetItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    //QGraphicsView::paint(painter, option, widget);


    if (!mInitialLoadDone) {
        QString message;

        message = tr("Loading ...");

        painter->setFont(QFont("Arial", 12));

        QFontMetrics fm = painter->fontMetrics();
        QRect txtBoundingRect = fm.boundingRect(message);

        //txtBoundingRect.moveCenter(rect().center().toPoint());
        txtBoundingRect.adjust(-10, -5, 10, 5);

        painter->setPen(Qt::NoPen);
        painter->setBrush(WBSettings::paletteColor);
        painter->drawRoundedRect(txtBoundingRect, 3, 3);

        painter->setPen(Qt::white);
        //painter->drawText(rect(), Qt::AlignCenter, message);
    }

    Delegate()->postpaint(painter, option, widget);
}

void WBGraphicsWidgetItem::geometryChangeRequested(const QRect& geom)
{
    resize(geom.width(), geom.height());
}

void WBGraphicsWidgetItem::javaScriptWindowObjectCleared()
{
    injectInlineJavaScript();

    if(!mUniboardAPI)
        mUniboardAPI = new WBWidgetUniboardAPI(scene(), this);

    //page()->mainFrame()->addToJavaScriptWindowObject("sankore", mUniboardAPI);

}

void WBGraphicsWidgetItem::mainFrameLoadFinished (bool ok)
{
    mLoadIsErronous = !ok;
    //update(boundingRect());

    if (mInitialLoadDone && scene() && scene()->renderingContext() == WBGraphicsScene::Screen)
        takeSnapshot();
}

void WBGraphicsWidgetItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (Delegate()->wheelEvent(event))
    {
        //QGraphicsView::wheelEvent(event);
        event->accept();
    }
}

//QVariant WBGraphicsWidgetItem::itemChange(GraphicsItemChange change, const QVariant &value)
//{
//    if ((change == QGraphicsItem::ItemSelectedHasChanged) &&  scene()) {
//        if (isSelected())
//            scene()->setActiveWindow(this);
//        else
//            if(scene()->activeWindow() == this)
//                scene()->setActiveWindow(0);
//    }
//
//    QVariant newValue = Delegate()->itemChange(change, value);
//    return QGraphicsView::itemChange(change, newValue);
//}

void WBGraphicsWidgetItem::resize(qreal w, qreal h)
{
    WBGraphicsWidgetItem::resize(QSizeF(w, h));
}


void WBGraphicsWidgetItem::resize(const QSizeF & pSize)
{
    if (pSize != size()) {
		//QGraphicsWidget::setMaximumSize(pSize.width(), pSize.height());
		//QGraphicsWidget::resize(pSize.width(), pSize.height());
  //      if (Delegate())
  //          Delegate()->positionHandles();
  //      if (scene())
  //          scene()->setModified(true);
    }
}

QSizeF WBGraphicsWidgetItem::size() const
{
	QSizeF sz;
	return sz;//QGraphicsWidget::size();
}



WBGraphicsAppleWidgetItem::WBGraphicsAppleWidgetItem(const QUrl& pWidgetUrl, QGraphicsItem *parent)
    : WBGraphicsWidgetItem(pWidgetUrl, parent)
{
    QString path = pWidgetUrl.toLocalFile();

    if (!path.endsWith(".wdgt") && !path.endsWith(".wdgt/")) {
        int lastSlashIndex = path.lastIndexOf("/");
        if (lastSlashIndex > 0)
            path = path.mid(0, lastSlashIndex + 1);
    }

    QFile plistFile(path + "/Info.plist");
    plistFile.open(QFile::ReadOnly);

    QByteArray plistBin = plistFile.readAll();
    QString plist = QString::fromUtf8(plistBin);

    int mainHtmlIndex = plist.indexOf("MainHTML");
    int mainHtmlIndexStart = plist.indexOf("<string>", mainHtmlIndex);
    int mainHtmlIndexEnd = plist.indexOf("</string>", mainHtmlIndexStart);

    if (mainHtmlIndex > -1 && mainHtmlIndexStart > -1 && mainHtmlIndexEnd > -1)
        mMainHtmlFileName = plist.mid(mainHtmlIndexStart + 8, mainHtmlIndexEnd - mainHtmlIndexStart - 8);

    mMainHtmlUrl = pWidgetUrl;
    mMainHtmlUrl.setPath(pWidgetUrl.path() + "/" + mMainHtmlFileName);

    //load(mMainHtmlUrl);

    QPixmap defaultPixmap(pWidgetUrl.toLocalFile() + "/Default.png");

    //setMaximumSize(defaultPixmap.size());

    mNominalSize = defaultPixmap.size();

    initialize();
}


WBGraphicsAppleWidgetItem::~WBGraphicsAppleWidgetItem()
{
    /* NOOP */
}

void WBGraphicsAppleWidgetItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    //setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

WBItem* WBGraphicsAppleWidgetItem::deepCopy() const
{
	WBGraphicsAppleWidgetItem *appleWidget = new WBGraphicsAppleWidgetItem(QUrl(), parentItem());

    copyItemParameters(appleWidget);

    return appleWidget;

}

void WBGraphicsAppleWidgetItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsAppleWidgetItem *cp = dynamic_cast<WBGraphicsAppleWidgetItem*>(copy);
    if (cp)
    {
        foreach(QString key, mPreferences.keys())
        {
            cp->setPreference(key, mPreferences.value(key));
        }

        foreach(QString key, mDatastore.keys())
        {
            cp->setDatastoreEntry(key, mDatastore.value(key));
        }

        cp->setSourceUrl(this->sourceUrl());
        //cp->setZValue(this->zValue());
    }

}


QRectF WBGraphicsAppleWidgetItem::boundingRect() const
{
	qreal penWidth = 1;
	return QRectF(0 - penWidth / 2, 0 - penWidth / 2, 20 + penWidth, 20 + penWidth);
}



bool WBGraphicsW3CWidgetItem::sTemplateLoaded = false;
QString WBGraphicsW3CWidgetItem::sNPAPIWrappperConfigTemplate;
QMap<QString, QString> WBGraphicsW3CWidgetItem::sNPAPIWrapperTemplates;

WBGraphicsW3CWidgetItem::WBGraphicsW3CWidgetItem(const QUrl& pWidgetUrl, QGraphicsItem *parent)
    : WBGraphicsWidgetItem(pWidgetUrl, parent)
    , mW3CWidgetAPI(0)
{
    QString path = pWidgetUrl.toLocalFile();
    QDir potentialDir(path);

    if (!path.endsWith(".wgt") && !path.endsWith(".wgt/") && !potentialDir.exists()) {
        int lastSlashIndex = path.lastIndexOf("/");
        if (lastSlashIndex > 0)
            path = path.mid(0, lastSlashIndex + 1);
    }

    if (!path.endsWith("/"))
        path += "/";

    int width = 300;
    int height = 150;

    QFile configFile(path + "config.xml");
    configFile.open(QFile::ReadOnly);

    QDomDocument doc;
    doc.setContent(configFile.readAll());
    QDomNodeList widgetDomList = doc.elementsByTagName("widget");

    if (widgetDomList.count() > 0) {
        QDomElement widgetElement = widgetDomList.item(0).toElement();

        width = widgetElement.attribute("width", "300").toInt();
        height = widgetElement.attribute("height", "150").toInt();

        mMetadatas.id = widgetElement.attribute("id", "");

        if (mMetadatas.id.length() == 0)
             mMetadatas.id = widgetElement.attribute("identifier", "");

        mMetadatas.version = widgetElement.attribute("version", "");

        mIsResizable = widgetElement.attribute("ub:resizable", "false") == "true";
        mIsFreezable = widgetElement.attribute("ub:freezable", "true") == "true";

        QString roles = widgetElement.attribute("ub:roles", "content tool").trimmed().toLower();

        if (roles == "" || roles.contains("tool"))
            mCanBeTool = WBGraphicsWidgetItem::type_ALL;

        if (roles.contains("twin"))
            mCanBeTool |= WBGraphicsWidgetItem::type_WIN;

        if (roles.contains("tmac"))
            mCanBeTool |= WBGraphicsWidgetItem::type_MAC;

        if (roles.contains("tunix"))
            mCanBeTool |= WBGraphicsWidgetItem::type_UNIX;

        /* --------- */

        if (roles == "" || roles.contains("content"))
            mCanBeContent = WBGraphicsWidgetItem::type_ALL;

        if (roles.contains("cwin"))
            mCanBeContent |= WBGraphicsWidgetItem::type_WIN;

        if (roles.contains("cmac"))
            mCanBeContent |= WBGraphicsWidgetItem::type_MAC;

        if (roles.contains("cunix"))
            mCanBeContent |= WBGraphicsWidgetItem::type_UNIX;

        //------------------------------//

        QDomNodeList contentDomList = widgetElement.elementsByTagName("content");

        if (contentDomList.count() > 0) {
            QDomElement contentElement = contentDomList.item(0).toElement();
            mMainHtmlFileName = contentElement.attribute("src", "");
        }

        mMetadatas.name = textForSubElementByLocale(widgetElement, "name", QLocale::system());
        mMetadatas.description = textForSubElementByLocale(widgetElement, "description ", QLocale::system());

        QDomNodeList authorDomList = widgetElement.elementsByTagName("author");

        if (authorDomList.count() > 0) {
            QDomElement authorElement = authorDomList.item(0).toElement();

            mMetadatas.author = authorElement.text();
            mMetadatas.authorHref = authorElement.attribute("href", "");
            mMetadatas.authorEmail = authorElement.attribute("email ", "");
        }

        QDomNodeList propertiesDomList = widgetElement.elementsByTagName("preference");

        for (int i = 0; i < propertiesDomList.length(); i++) {
            QDomElement preferenceElement = propertiesDomList.at(i).toElement();
            QString prefName = preferenceElement.attribute("name", "");

            if (prefName.length() > 0) {
                QString prefValue = preferenceElement.attribute("value", "");
                bool readOnly = (preferenceElement.attribute("readonly", "false") == "true");

                mPreferences.insert(prefName, PreferenceValue(prefValue, readOnly));
            }
        }
    }

    if (mMainHtmlFileName.length() == 0) {
        QFile defaultStartFile(path + "index.htm");

        if (defaultStartFile.exists())
            mMainHtmlFileName = "index.htm";
        else {
            QFile secondDefaultStartFile(path + "index.html");

            if (secondDefaultStartFile.exists())
                mMainHtmlFileName = "index.html";
        }
    }

    mMainHtmlUrl = pWidgetUrl;
    mMainHtmlUrl.setPath(pWidgetUrl.path() + "/" + mMainHtmlFileName);
    /* is it a valid local file ? */
    QFile f(mMainHtmlUrl.toLocalFile());

    if(!f.exists())
        mMainHtmlUrl = QUrl(mMainHtmlFileName);

    //connect(page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(javaScriptWindowObjectCleared()));
    //connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), this, SLOT(javaScriptWindowObjectCleared()));

    //load(mMainHtmlUrl);

    //setMaximumSize(QSize(width, height));

    mNominalSize = QSize(width, height);

    initialize();
    setOwnFolder(pWidgetUrl);
}

WBGraphicsW3CWidgetItem::~WBGraphicsW3CWidgetItem()
{
    /* NOOP */
}

void WBGraphicsW3CWidgetItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    //setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

WBItem* WBGraphicsW3CWidgetItem::deepCopy() const
{
	WBGraphicsW3CWidgetItem *copy = new WBGraphicsW3CWidgetItem(mWidgetUrl, parentItem());
    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable
    copyItemParameters(copy);

    return copy;
}

QMap<QString, WBGraphicsW3CWidgetItem::PreferenceValue> WBGraphicsW3CWidgetItem::preferences()
{
    return mPreferences;
}

WBGraphicsW3CWidgetItem::Metadata WBGraphicsW3CWidgetItem::metadatas() const
{
    return mMetadatas;
}

QRectF WBGraphicsW3CWidgetItem::boundingRect() const
{
	qreal penWidth = 1;
	return QRectF(0 - penWidth / 2, 0 - penWidth / 2, 200 + penWidth, 200 + penWidth);
}

QString WBGraphicsW3CWidgetItem::createNPAPIWrapper(const QString& url, const QString& pMimeType, const QSize& sizeHint, const QString& pName)
{
    const QString userWidgetPath = WBSettings::settings()->userInteractiveDirectory() + "/" + tr("Web");
    QDir userWidgetDir(userWidgetPath);

    return createNPAPIWrapperInDir(url, userWidgetDir, pMimeType, sizeHint, pName);
}

QString WBGraphicsW3CWidgetItem::createNPAPIWrapperInDir(const QString& pUrl, const QDir& pDir, const QString& pMimeType, const QSize& sizeHint, const QString& pName)
{
    QString url = pUrl;
    url = WBFileSystemUtils::removeLocalFilePrefix(url);
    QString name = pName;

    QFileInfo fi(url);

    if (name.length() == 0)
        name = fi.baseName();

    if (fi.exists())
        url = fi.fileName();

    loadNPAPIWrappersTemplates();

    QString htmlTemplate;

    if (pMimeType.length() > 0 && sNPAPIWrapperTemplates.contains(pMimeType))
        htmlTemplate = sNPAPIWrapperTemplates.value(pMimeType);
    else {
        QString extension = WBFileSystemUtils::extension(url);
        if (sNPAPIWrapperTemplates.contains(extension))
            htmlTemplate = sNPAPIWrapperTemplates.value(extension);
    }

    if (htmlTemplate.length() > 0) {
        htmlTemplate = htmlTemplate.replace(QString("{in.url}"), url)
            .replace(QString("{in.width}"), QString("%1").arg(sizeHint.width()))
            .replace(QString("{in.height}"), QString("%1").arg(sizeHint.height()));

        QString configTemplate = sNPAPIWrappperConfigTemplate
            .replace(QString("{in.id}"), url)
            .replace(QString("{in.width}"), QString("%1").arg(sizeHint.width()))
            .replace(QString("{in.height}"), QString("%1").arg(sizeHint.height()))
            .replace(QString("{in.name}"), name)
            .replace(QString("{in.startFile}"), QString("index.htm"));

        QString dirPath = pDir.path();
        if (!pDir.exists())
            pDir.mkpath(dirPath);

        QString widgetLibraryPath = dirPath + "/" + name + ".wgt";
        QDir widgetLibraryDir(widgetLibraryPath);

        if (widgetLibraryDir.exists())
            if (!WBFileSystemUtils::deleteDir(widgetLibraryDir.path()))
                qWarning() << "Cannot delete old widget " << widgetLibraryDir.path();

        widgetLibraryDir.mkpath(widgetLibraryPath);
        if (fi.exists()) {
            QString target = widgetLibraryPath + "/" + fi.fileName();
            QString source = pUrl;
            source = WBFileSystemUtils::removeLocalFilePrefix(source);
            QFile::copy(source, target);
        }

        QFile configFile(widgetLibraryPath + "/config.xml");

        if (!configFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot open file " << configFile.fileName();
            return QString();
        }

        QTextStream outConfig(&configFile);
        outConfig.setCodec("UTF-8");

        outConfig << configTemplate;
        configFile.close();

        QFile indexFile(widgetLibraryPath + "/index.htm");

        if (!indexFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot open file " << indexFile.fileName();
            return QString();
        }

        QTextStream outIndex(&indexFile);
        outIndex.setCodec("UTF-8");

        outIndex << htmlTemplate;
        indexFile.close();

        return widgetLibraryPath;
    }
    else
        return QString();
}

QString WBGraphicsW3CWidgetItem::createHtmlWrapperInDir(const QString& html, const QDir& pDir, const QSize& sizeHint, const QString& pName)
{
    QString widgetPath = pDir.path() + "/" + pName + ".wgt";
    widgetPath = WBFileSystemUtils::nextAvailableFileName(widgetPath);
    QDir widgetDir(widgetPath);

    if (!widgetDir.exists())
        widgetDir.mkpath(widgetDir.path());

    QFile configFile(widgetPath + "/" + "config.xml");

    if (configFile.exists())
        configFile.remove(configFile.fileName());

    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file " << configFile.fileName();
        return "";
    }

    QTextStream outConfig(&configFile);
    outConfig.setCodec("UTF-8");
    outConfig << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    outConfig << "<widget xmlns=\"http://www.w3.org/ns/widgets\"" << endl;
    outConfig << "    xmlns:ub=\"http://uniboard.mnemis.com/widgets\"" << endl;
    outConfig << "    id=\"http://uniboard.mnemis.com/" << pName << "\"" <<endl;

    outConfig << "    version=\"1.0\"" << endl;
    outConfig << "    width=\"" << sizeHint.width() << "\"" << endl;
    outConfig << "    height=\"" << sizeHint.height() << "\"" << endl;
    outConfig << "    ub:resizable=\"true\">" << endl;

    outConfig << "  <name>" << pName << "</name>" << endl;
    outConfig << "  <content src=\"" << pName << ".html\"/>" << endl;

    outConfig << "</widget>" << endl;

    configFile.close();

    const QString fullHtmlFileName = widgetPath + "/" + pName + ".html";

    QFile widgetHtmlFile(fullHtmlFileName);
    if (widgetHtmlFile.exists())
        widgetHtmlFile.remove(widgetHtmlFile.fileName());
    if (!widgetHtmlFile.open(QIODevice::WriteOnly)) {
        qWarning() << "cannot open file " << widgetHtmlFile.fileName();
        return QString();
    }

    QTextStream outStartFile(&widgetHtmlFile);
    outStartFile.setCodec("UTF-8");

    outStartFile << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">" << endl;
    outStartFile << "<html>" << endl;
    outStartFile << "<head>" << endl;
    outStartFile << "    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">" << endl;
    outStartFile << "</head>" << endl;
    outStartFile << "  <body>" << endl;
    outStartFile << html << endl;
    outStartFile << "  </body>" << endl;
    outStartFile << "</html>" << endl;

    widgetHtmlFile.close();

    return widgetPath;
}

QString WBGraphicsW3CWidgetItem::freezedWidgetPage()
{
    static QString defaultcontent;

    if (defaultcontent.isNull()) {
        QString freezedWidgetDefaultContentFilePath = freezedWidgetFilePath();
        QFile wrapperFile(freezedWidgetDefaultContentFilePath);
        if (!wrapperFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "can't open wrapper file " + freezedWidgetDefaultContentFilePath;
            defaultcontent = "";
        }
        else {
            QByteArray arr = wrapperFile.readAll();
            if (!arr.isEmpty())
                defaultcontent = QString(arr);
            else {
                qDebug() << "content of " + freezedWidgetDefaultContentFilePath + "is empty";
                defaultcontent = QString();
            }
        }
    }

    return defaultcontent;
}

QString WBGraphicsW3CWidgetItem::freezedWidgetFilePath()
{
    return WBPlatformUtils::applicationResourcesDirectory() + "/etc/" + "freezedWidgetWrapper.html";
}

bool WBGraphicsW3CWidgetItem::hasNPAPIWrapper(const QString& pMimeType)
{
    loadNPAPIWrappersTemplates();

    return sNPAPIWrapperTemplates.contains(pMimeType);
}

void WBGraphicsW3CWidgetItem::javaScriptWindowObjectCleared()
{
    WBGraphicsWidgetItem::javaScriptWindowObjectCleared();

    if(!mW3CWidgetAPI)
        mW3CWidgetAPI = new WBW3CWidgetAPI(this);

    //page()->mainFrame()->addToJavaScriptWindowObject("widget", mW3CWidgetAPI);

}

void WBGraphicsW3CWidgetItem::loadNPAPIWrappersTemplates()
{
    if (!sTemplateLoaded) {
        sNPAPIWrapperTemplates.clear();

        QString etcPath = WBPlatformUtils::applicationResourcesDirectory() + "/etc/";

        QDir etcDir(etcPath);

        foreach(QString fileName, etcDir.entryList()) {
            if (fileName.startsWith("npapi-wrapper") && (fileName.endsWith(".htm") || fileName.endsWith(".html"))) {

                QString htmlContent = WBFileSystemUtils::readTextFile(etcPath + fileName);

                if (htmlContent.length() > 0) {
                    QStringList tokens = fileName.split(".");

                    if (tokens.length() >= 4) {
                        QString mime = tokens.at(tokens.length() - 4 );
                        mime += "/" + tokens.at(tokens.length() - 3);

                        QString fileExtension = tokens.at(tokens.length() - 2);

                        sNPAPIWrapperTemplates.insert(mime, htmlContent);
                        sNPAPIWrapperTemplates.insert(fileExtension, htmlContent);
                    }
                }
            }
        }
        sNPAPIWrappperConfigTemplate = WBFileSystemUtils::readTextFile(etcPath + "npapi-wrapper.config.xml");
        sTemplateLoaded = true;
    }
}

QString WBGraphicsW3CWidgetItem::textForSubElementByLocale(QDomElement rootElement, QString subTagName, QLocale locale)
{
    QDomNodeList subList = rootElement.elementsByTagName(subTagName);

    QString lang = locale.name();

    if (lang.length() > 2)
        lang[2] = QLatin1Char('-');

    if (subList.count() > 1) {
        for(int i = 0; i < subList.count(); i++) {
            QDomNode node = subList.at(i);
            QDomElement element = node.toElement();

            QString configLang = element.attribute("xml:lang", "");

            if(lang == configLang || (configLang.length() == 2 && configLang == lang.left(2)))
                 return element.text();
        }
    }

    if (subList.count() >= 1) {
        QDomElement element = subList.item(0).toElement();
        return element.text();
    }

    return QString();
}

void WBGraphicsW3CWidgetItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsW3CWidgetItem *cp = dynamic_cast<WBGraphicsW3CWidgetItem*>(copy);
    if (cp)
    {
        /*cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));*/
        cp->setSourceUrl(this->sourceUrl());

        cp->resize(this->size());

        foreach(QString key, this->WBGraphicsWidgetItem::preferences().keys())
        {
            cp->setPreference(key, WBGraphicsWidgetItem::preferences().value(key));
        }

        foreach(QString key, mDatastore.keys())
        {
            cp->setDatastoreEntry(key, mDatastore.value(key));
        }

        //cp->setZValue(this->zValue());
    }
}

