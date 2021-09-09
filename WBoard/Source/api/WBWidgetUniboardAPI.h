#ifndef WBWIDGETAPI_H
#define WBWIDGETAPI_H

#include <QtCore>
#include <QGraphicsSceneDragDropEvent>

#include "WBW3CWidgetAPI.h"
#include "core/WBDownloadManager.h"

class WBGraphicsScene;
class WBGraphicsWidgetItem;
class WBGraphicsW3CWidgetItem;

class WBWidgetMessageAPI;
class WBDatastoreAPI;
class WBDocumentDatastoreAPI;

class WBWidgetUniboardAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pageCount READ pageCount SCRIPTABLE true)

    Q_PROPERTY(int currentPageNumber READ currentPageNumber SCRIPTABLE true)

    Q_PROPERTY(QString uuid READ uuid SCRIPTABLE true)
    
    Q_PROPERTY(QString lang READ lang SCRIPTABLE true)

    Q_PROPERTY(QObject* messages READ messages SCRIPTABLE true)

    Q_PROPERTY(QObject* datastore READ datastore SCRIPTABLE true)

    public:
        WBWidgetUniboardAPI(WBGraphicsScene *pScene, WBGraphicsWidgetItem *widget = 0);
        ~WBWidgetUniboardAPI();

        QObject* messages();

        QObject* datastore();

    public slots:
        void setScene(WBGraphicsScene* pScene)
        {
            mScene = pScene;
        }

        void setTool(const QString& toolString);

        void setPenColor(const QString& penColor);

        void setMarkerColor(const QString& penColor);

        QString pageThumbnail(const int pageNumber);

        void zoom(const qreal factor, const qreal x, const qreal y);

        void move(const qreal x, const qreal y);

        void moveTo(const qreal x, const qreal y);

        void drawLineTo(const qreal x, const qreal y, const qreal pWidth);

        void eraseLineTo(const qreal x, const qreal y, const qreal pWidth);

        void clear();

        void setBackground(bool pIsDark, bool pIsCrossed);

        void addObject(QString pUrl, int width = 0, int height = 0, int x = 0, int y = 0, bool background = false);

        void resize(qreal width, qreal height);

        QString locale();

        void setPreference(const QString& key, QString value);

        QString preference(const QString& key, const QString& pDefault = QString());

        QStringList preferenceKeys();

        void showMessage(const QString& message);

        void centerOn(const qreal x, const qreal y);

        void addText(const QString& text, const qreal x, const qreal y, const int height = -1, const QString& font = ""
                , bool bold = false, bool italic = false);

        void returnStatus(const QString& method, const QString& status);
        void usedMethods(QStringList methods);
        void response(bool correct);

        void sendFileMetadata(QString metaData);

        void enableDropOnWidget (bool enable = true);

        void ProcessDropEvent(QGraphicsSceneDragDropEvent *);
        bool isDropableData(const QMimeData *pMimeData) const;

	private slots:
        void onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);

	private:
        inline void registerIDWidget(int id){webDownloadIds.append(id);}
        inline bool takeIDWidget(int id);

    private:
        QString uuid();

        QString lang();

        int pageCount();

        int currentPageNumber();
        QString getObjDir();
        QString createMimeText(bool downloaded, const QString &mimeType, const QString &fileName);
        bool supportedTypeHeader(const QString &) const;
        QString boolToStr(bool value) const {return value ? "true" : "false";}

        WBGraphicsScene* mScene;

        WBGraphicsWidgetItem* mGraphicsWidget;

        bool mIsVisible;

        WBWidgetMessageAPI* mMessagesAPI;

        WBDatastoreAPI* mDatastoreAPI;
        QList<int> webDownloadIds;
};

class WBDatastoreAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject* document READ document SCRIPTABLE true)

    public:
        WBDatastoreAPI(WBGraphicsW3CWidgetItem *widget);
        virtual ~WBDatastoreAPI(){;}

        QObject* document();

    private:
        WBDocumentDatastoreAPI* mDocumentDatastore;

};

class WBDocumentDatastoreAPI : public WBW3CWebStorage
{
    Q_OBJECT

    public:
        WBDocumentDatastoreAPI(WBGraphicsW3CWidgetItem *graphicsWidget);

        virtual ~WBDocumentDatastoreAPI();

    public slots:
        virtual QString key(int index);
        virtual QString getItem(const QString& key);
        virtual void setItem(const QString& key, const QString& value);
        virtual void removeItem(const QString& key);
        virtual void clear();

    protected:
        virtual int length();

    private:
        WBGraphicsW3CWidgetItem* mGraphicsW3CWidget;

};

#endif // WBWIDGETAPI_H
