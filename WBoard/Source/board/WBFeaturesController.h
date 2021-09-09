#ifndef WBFEATURESCONTROLLER_H
#define WBFEATURESCONTROLLER_H

#include <QMetaType>
#include <QObject>
#include <QWidget>
#include <QSet>
#include <QVector>
#include <QString>
#include <QPixmap>
#include <QMap>
#include <QUrl>
#include <QByteArray>
#include <QtWidgets>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QListView>

class WBFeaturesModel;
class WBFeaturesItemDelegate;
class WBFeaturesPathItemDelegate;
class WBFeaturesProxyModel;
class WBFeaturesSearchProxyModel;
class WBFeaturesPathProxyModel;
class WBFeaturesListView;
class WBFeature;

class WBFeaturesComputingThread : public QThread
{
    Q_OBJECT
public:
    explicit WBFeaturesComputingThread(QObject *parent = 0);
    virtual ~WBFeaturesComputingThread();
        void compute(const QList<QPair<QUrl, WBFeature> > &pScanningData, QSet<QUrl> *pFavoritesSet);

protected:
    void run();

signals:
    void sendFeature(WBFeature pFeature);
    void featureSent();
    void scanStarted();
    void scanFinished();
    void maxFilesCountEvaluated(int max);
    void scanCategory(const QString &str);
    void scanPath(const QString &str);

private:
    void scanFS(const QUrl & currentPath, const QString & currVirtualPath, const QSet<QUrl> &pFavoriteSet);
    void scanAll(QList<QPair<QUrl, WBFeature> > pScanningData, const QSet<QUrl> &pFavoriteSet);
    int featuresCount(const QUrl &pPath);
    int featuresCountAll(QList<QPair<QUrl, WBFeature> > pScanningData);

private:
    QMutex mMutex;
    QWaitCondition mWaitCondition;
    QUrl mScanningPath;
    QString mScanningVirtualPath;
    QList<QPair<QUrl, WBFeature> > mScanningData;
    QSet<QUrl> mFavoriteSet;
    bool restart;
    bool abort;
};


enum WBFeatureElementType
{
    FEATURE_CATEGORY,
    FEATURE_VIRTUALFOLDER,
    FEATURE_FOLDER,
    FEATURE_INTERACTIVE,
    FEATURE_INTERNAL,
    FEATURE_ITEM,
    FEATURE_AUDIO,
    FEATURE_VIDEO,
    FEATURE_IMAGE,
    FEATURE_FLASH,
    FEATURE_TRASH,
    FEATURE_FAVORITE,
    FEATURE_SEARCH,
    FEATURE_INVALID
};

class WBFeature
{
public:
    WBFeature() {;}
    WBFeature(const QString &url, const QImage &icon, const QString &name, const QUrl &realPath, WBFeatureElementType type = FEATURE_CATEGORY);
    virtual ~WBFeature();
    QString getName() const { return mName; }
    QString getDisplayName() const {return mDisplayName;}
    QImage getThumbnail() const {return mThumbnail;}
    QString getVirtualPath() const { return virtualDir; }
    QUrl getFullPath() const { return mPath; }
    QString getFullVirtualPath() const { return  virtualDir + "/" + mName; }
    QString getUrl() const;
    void setFullPath(const QUrl &newPath) {mPath = newPath;}
    void setFullVirtualPath(const QString &newVirtualPath) {virtualDir = newVirtualPath;}
    WBFeatureElementType getType() const { return elementType; }

    bool isFolder() const;
    bool allowedCopy() const;
    bool isDeletable() const;
    bool inTrash() const;
    bool operator ==( const WBFeature &f )const;
    bool operator !=( const WBFeature &f )const;
    const QMap<QString,QString> & getMetadata() const { return metadata; }
    void setMetadata( const QMap<QString,QString> &data ) { metadata = data; }

private:
    QString getNameFromVirtualPath(const QString &pVirtPath);
    QString getVirtualDirFromVirtualPath(const QString &pVirtPath);

private:
    QString virtualDir;
    QString virtualPath;
    QImage mThumbnail;
    QString mName;
    QString mDisplayName;
    QUrl mPath;
    WBFeatureElementType elementType;
    QMap<QString,QString> metadata;
};
Q_DECLARE_METATYPE( WBFeature )

class WBFeaturesController : public QObject
{
friend class WBFeaturesWidget;

Q_OBJECT

public:
    WBFeaturesController(QWidget *parentWidget);
    virtual ~WBFeaturesController();

    QList <WBFeature>* getFeatures() const {return featuresList;}

    const QString& getRootPath()const {return rootPath;}
    void scanFS();

    void addItemToPage(const WBFeature &item);
    void addItemAsBackground(const WBFeature &item);
    const WBFeature& getCurrentElement()const {return currentElement;}
    void setCurrentElement( const WBFeature &elem ) {currentElement = elem;}
    const WBFeature & getTrashElement () const { return trashElement; }

    void addDownloadedFile( const QUrl &sourceUrl, const QByteArray &pData, const QString pContentSource, const QString pTitle );

    WBFeature moveItemToFolder( const QUrl &url, const WBFeature &destination );
    WBFeature copyItemToFolder( const QUrl &url, const WBFeature &destination );
    void moveExternalData(const QUrl &url, const WBFeature &destination);

    void rescanModel();
    void siftElements(const QString &pSiftValue);
    //TODO make less complicated for betteer maintainence
    WBFeature getFeature(const QModelIndex &index, const QString &listName);
    void searchStarted(const QString &pattern, QListView *pOnView);
    void refreshModels();

    void deleteItem( const QUrl &url );
    void deleteItem(const WBFeature &pFeature);
    bool isTrash( const QUrl &url );
    void moveToTrash(WBFeature feature, bool deleteManualy = false);
    void addToFavorite( const QUrl &path );
    void removeFromFavorite(const QUrl &path, bool deleteManualy = false);
    void importImage(const QImage &image, const QString &fileName = QString());
    void importImage( const QImage &image, const WBFeature &destination, const QString &fileName = QString() );
    QStringList getFileNamesInFolders();

    void fileSystemScan(const QUrl &currPath, const QString & currVirtualPath);
    int featuresCount(const QUrl &currPath);
    static WBFeatureElementType fileTypeFromUrl( const QString &path );

    static QString fileNameFromUrl( const QUrl &url );
    static QImage getIcon( const QString &path, WBFeatureElementType pFType );
    static bool isDeletable( const QUrl &url );
    static char featureTypeSplitter() {return ':';}
    static QString categoryNameForVirtualPath(const QString &str);

    static const QString virtualRootName;

    void assignFeaturesListView(WBFeaturesListView *pList);
    void assignPathListView(WBFeaturesListView *pList);

public:
    static const QString rootPath;
    static const QString audiosPath;
    static const QString moviesPath;
    static const QString picturesPath;
    static const QString appPath;
    static const QString flashPath;
    static const QString shapesPath;
    static const QString interactPath;
    static const QString trashPath;
    static const QString favoritePath;

signals:
    void maxFilesCountEvaluated(int pLimit);
    void scanStarted();
    void scanFinished();
    void featureAddedFromThread();
    void scanCategory(const QString &);
    void scanPath(const QString &);

private slots:
    void addNewFolder(QString name);
    void startThread();
    void createNpApiFeature(const QString &str);

private:
    WBFeaturesItemDelegate *itemDelegate;
    WBFeaturesPathItemDelegate *pathItemDelegate;

    WBFeaturesModel *featuresModel;
    WBFeaturesProxyModel *featuresProxyModel;
    WBFeaturesSearchProxyModel *featuresSearchModel;
    WBFeaturesPathProxyModel *featuresPathModel;

    QAbstractItemModel *curListModel;
    WBFeaturesComputingThread mCThread;

private:
    static QImage createThumbnail(const QString &path);
    //void addImageToCurrentPage( const QString &path );
    void loadFavoriteList();
    void saveFavoriteList();
    QString uniqNameForFeature(const WBFeature &feature, const QString &pName = "Imported", const QString &pExtention = "") const;
    QString adjustName(const QString &str);

    QList <WBFeature> *featuresList;

    QUrl mUserAudioDirectoryPath;
    QUrl mUserVideoDirectoryPath;
    QUrl mUserPicturesDirectoryPath;
    QUrl mUserInteractiveDirectoryPath;
    QUrl mUserAnimationDirectoryPath;

    QString libraryPath;
    QUrl mLibPicturesDirectoryPath;
    QUrl mLibAudiosDirectoryPath;
    QUrl mLibVideosDirectoryPath;
    //QUrl mLibInteractiveDirectoryPath;//zhusizhi
    //QUrl mLibAnimationsDirectoryPath;
    //QUrl mLibApplicationsDirectoryPath;
    QUrl mLibShapesDirectoryPath;

    QUrl trashDirectoryPath;

    int mLastItemOffsetIndex;
    WBFeature currentElement;

    WBFeature rootElement;
    WBFeature favoriteElement;
    WBFeature audiosElement;
    WBFeature moviesElement;
    WBFeature picturesElement;
    //WBFeature interactElement;
    WBFeature applicationsElement;
    WBFeature shapesElement;

    QSet <QUrl> *favoriteSet;

public:
    WBFeature trashElement;
    WBFeature getDestinationFeatureForUrl( const QUrl &url );
    WBFeature getDestinationFeatureForMimeType(const QString &pMmimeType);

};
#endif
