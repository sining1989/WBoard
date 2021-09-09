#include <QDomDocument>
#include <QWebEngineView>
#include <QtWebChannel/QWebChannel>
#include <QWidget>

#include "WBFeaturesWidget.h"
#include "gui/WBThumbnailWidget.h"
#include "frameworks/WBFileSystemUtils.h"
#include "core/WBApplication.h"
#include "core/WBDownloadManager.h"
#include "globals/WBGlobals.h"
#include "board/WBBoardController.h"

const char *WBFeaturesWidget::objNamePathList = "PathList";
const char *WBFeaturesWidget::objNameFeatureList = "FeatureList";

const QMargins FeatureListMargins(0, 0, 0, 30);
const int FeatureListBorderOffset = 10;
const char featureTypeSplitter = ':';
static const QString mimeSankoreFeatureTypes = "Sankore/featureTypes";

WBFeaturesWidget::WBFeaturesWidget(QWidget *parent, const char *name)
    : WBDockPaletteWidget(parent)
    , imageGatherer(NULL)
{
    setObjectName(name);
    mName = "FeaturesWidget";
    mVisibleState = true;

    SET_STYLE_SHEET();

    mIconToLeft = QPixmap(":images/general_open.png");
    mIconToRight = QPixmap(":images/general_close.png");
    setAcceptDrops(true);

    //Main WBFeature functionality
    controller = new WBFeaturesController(this);

    //Main layout including all the widgets in palette
    layout = new QVBoxLayout(this);

    //Path icon view on the top of the palette
    pathListView = new WBFeaturesListView(this, objNamePathList);
    controller->assignPathListView(pathListView);

    centralWidget = new WBFeaturesCentralWidget(this);
    controller->assignFeaturesListView(centralWidget->listView());
    centralWidget->setSliderPosition(WBSettings::settings()->featureSliderPosition->get().toInt());

    //Bottom actionbar for DnD, quick search etc
    mActionBar = new WBFeaturesActionBar(controller, this);

    //Filling main layout
    layout->addWidget(pathListView);
    layout->addWidget(centralWidget);
    layout->addWidget(mActionBar);

    connect(centralWidget->listView(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(currentSelected(const QModelIndex &)));
    connect(this, SIGNAL(sendFileNameList(QStringList)), centralWidget, SIGNAL(sendFileNameList(QStringList)));
    connect(mActionBar, SIGNAL(searchElement(const QString &)), this, SLOT( searchStarted(const QString &)));
    connect(mActionBar, SIGNAL(newFolderToCreate()), this, SLOT(createNewFolder()));
    connect(mActionBar, SIGNAL(deleteElements(const WBFeaturesMimeData *)), this, SLOT(deleteElements(const WBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(deleteSelectedElements()), this, SLOT(deleteSelectedElements()));
    connect(mActionBar, SIGNAL(addToFavorite(const WBFeaturesMimeData *)), this, SLOT(addToFavorite(const WBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(removeFromFavorite(const WBFeaturesMimeData *)), this, SLOT(removeFromFavorite(const WBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(addElementsToFavorite() ), this, SLOT ( addElementsToFavorite()) );
    connect(mActionBar, SIGNAL(removeElementsFromFavorite()), this, SLOT (removeElementsFromFavorite()));

    connect(mActionBar, SIGNAL(rescanModel()), this, SLOT(rescanModel()));
    connect(pathListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(currentSelected(const QModelIndex &)));
    connect(WBApplication::boardController, SIGNAL(displayMetadata(QMap<QString,QString>)), this, SLOT(onDisplayMetadata( QMap<QString,QString>)));
    connect(WBDownloadManager::downloadManager(), SIGNAL( addDownloadedFileToLibrary( bool, QUrl, QString, QByteArray, QString))
             , this, SLOT(onAddDownloadedFileToLibrary(bool, QUrl, QString,QByteArray, QString)));
    connect(centralWidget, SIGNAL(lockMainWidget(bool)), this, SLOT(lockIt(bool)));
    connect(centralWidget, SIGNAL(createNewFolderSignal(QString)), controller, SLOT(addNewFolder(QString)));
    connect(controller, SIGNAL(scanStarted()), centralWidget, SLOT(scanStarted()));
    connect(controller, SIGNAL(scanFinished()), centralWidget, SLOT(scanFinished()));
    connect(controller, SIGNAL(scanStarted()), mActionBar, SLOT(lockIt()));
    connect(controller, SIGNAL(scanFinished()), mActionBar, SLOT(unlockIt()));
    connect(controller, SIGNAL(maxFilesCountEvaluated(int)), centralWidget, SIGNAL(maxFilesCountEvaluated(int)));
    connect(controller, SIGNAL(featureAddedFromThread()), centralWidget, SIGNAL(increaseStatusBarValue()));
    connect(controller, SIGNAL(scanCategory(QString)), centralWidget, SIGNAL(scanCategory(QString)));
    connect(controller, SIGNAL(scanPath(QString)), centralWidget, SIGNAL(scanPath(QString)));
}

WBFeaturesWidget::~WBFeaturesWidget()
{
    if (NULL != imageGatherer)
        delete imageGatherer;
}

void WBFeaturesWidget::searchStarted(const QString &pattern)
{
    controller->searchStarted(pattern, centralWidget->listView());
}

void WBFeaturesWidget::currentSelected(const QModelIndex &current)
{
    if (!current.isValid()) {
        qWarning() << "SLOT:currentSelected, invalid index catched";
        return;
    }

    QString objName = sender()->objectName();

    if (objName.isEmpty()) {
        qWarning() << "incorrect sender";
    } else if (objName == objNamePathList) {
        //Calling to reset the model for listView. Maybe separate function needed
        controller->searchStarted("", centralWidget->listView());
    }

    WBFeature feature = controller->getFeature(current, objName);

    if ( feature.isFolder() ) {
        QString newPath = feature.getFullVirtualPath();

        controller->setCurrentElement(feature);
        controller->siftElements(newPath);

        centralWidget->switchTo(WBFeaturesCentralWidget::MainList);

        if ( feature.getType() == FEATURE_FAVORITE ) {
            mActionBar->setCurrentState( IN_FAVORITE );

        }  else if ( feature.getType() == FEATURE_CATEGORY && feature.getName() == "root" ) {
            mActionBar->setCurrentState( IN_ROOT );

        } else if (feature.getType() == FEATURE_TRASH) {
            mActionBar->setCurrentState(IN_TRASH);

        } else if (feature.getType() == FEATURE_SEARCH) {
            //The search feature behavior is not standard. If features list clicked - show empty element
            //else show existing saved features search QWebEngineView
            if (sender()->objectName() == objNameFeatureList) {
                centralWidget->showElement(feature, WBFeaturesCentralWidget::FeaturesWebView);
            } else if (sender()->objectName() == objNamePathList) {
                centralWidget->switchTo(WBFeaturesCentralWidget::FeaturesWebView);
            }

        } else  {
            mActionBar->setCurrentState(IN_FOLDER);
        }

//    } else if (feature.getType() == FEATURE_SEARCH) {
//        centralWidget->showElement(feature, WBFeaturesCentralWidget::FeaturesWebView);

    }

    else if (WBSettings::settings()->libraryShowDetailsForLocalItems->get().toBool() == true) {
        centralWidget->showElement(feature, WBFeaturesCentralWidget::FeaturePropertiesList);
        mActionBar->setCurrentState( IN_PROPERTIES );
    }
    mActionBar->cleanText();
}

void WBFeaturesWidget::createNewFolder()
{
    centralWidget->showAdditionalData(WBFeaturesCentralWidget::NewFolderDialog, WBFeaturesCentralWidget::Modal);
    emit sendFileNameList(controller->getFileNamesInFolders());
}

void WBFeaturesWidget::deleteElements( const WBFeaturesMimeData * mimeData )
{
    if (!mimeData->features().count() )
        return;

    QList<WBFeature> featuresList = mimeData->features();

    foreach ( WBFeature curFeature, featuresList ) {
        if ( curFeature.inTrash()) {
            controller->deleteItem(curFeature.getFullPath());

        } else {
           controller->moveToTrash(curFeature);
        }
    }

    controller->refreshModels();//zhusizhi
}

void WBFeaturesWidget::deleteSelectedElements()
{
    QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();

    QList<WBFeature> featureasToMove;
    for (int i = 0; i < selected.count(); i++)
    {
        featureasToMove.append(controller->getFeature(selected.at(i), objNameFeatureList));
    }

    foreach (WBFeature feature, featureasToMove)
    {
        if (feature.isDeletable()) {
            if (feature.inTrash()) {
                controller->deleteItem(feature);
            } else {
                controller->moveToTrash(feature, true);
            }
        }
    }

    controller->refreshModels();
}

void WBFeaturesWidget::rescanModel()
{
    controller->rescanModel();
}

void WBFeaturesWidget::lockIt(bool pLock)
{
    mActionBar->setEnabled(!pLock);
    pathListView->setEnabled(!pLock);
    centralWidget->setLockedExcludingAdditional(pLock);
}

void WBFeaturesWidget::addToFavorite( const WBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();
    foreach ( QUrl url, urls ) {
        controller->addToFavorite(url);
    }

    controller->refreshModels();
}

void WBFeaturesWidget::removeFromFavorite( const WBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();

    foreach( QUrl url, urls ) {
        controller->removeFromFavorite(url);
    }
}

void WBFeaturesWidget::onDisplayMetadata( QMap<QString,QString> metadata )
{
    QString previewImageUrl = ":images/libpalette/notFound.png";

    QString widgetsUrl = QUrl::fromEncoded(metadata["Url"].toLatin1()).toString()/*metadata.value("Url", QString())*/;
    QString widgetsThumbsUrl = QUrl::fromEncoded(metadata["thumbnailUrl"].toLatin1()).toString();

    QString strType = WBFileSystemUtils::mimeTypeFromFileName(widgetsUrl);
    WBMimeType::Enum thumbType = WBFileSystemUtils::mimeTypeFromString(strType);

    switch (static_cast<int>(thumbType)) {
    case WBMimeType::Audio:
        previewImageUrl = ":images/libpalette/soundIcon.svg";
        break;

    case WBMimeType::Video:
        previewImageUrl = ":images/libpalette/movieIcon.svg";
        break;

    case WBMimeType::RasterImage:
    case WBMimeType::VectorImage:
        previewImageUrl = widgetsUrl;
        break;
    }

    if (!widgetsThumbsUrl.isNull()) {
        previewImageUrl = ":/images/libpalette/loading.png";
        if (!imageGatherer)
            imageGatherer = new WBDownloadHttpFile(0, this);

        connect(imageGatherer, SIGNAL(downloadFinished(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)), this, SLOT(onPreviewLoaded(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)));

        // We send here the request and store its reply in order to be able to cancel it if needed
        imageGatherer->get(QUrl(widgetsThumbsUrl), QPoint(0,0), QSize(), false);
    }

    WBFeature feature( "/root", QImage(previewImageUrl), QString(), widgetsUrl, FEATURE_ITEM );
    feature.setMetadata( metadata );

    centralWidget->showElement(feature, WBFeaturesCentralWidget::FeaturePropertiesList);
    mActionBar->setCurrentState( IN_PROPERTIES );
}


void WBFeaturesWidget::onPreviewLoaded(int id, bool pSuccess, QUrl sourceUrl, QUrl originalUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
{
    Q_UNUSED(id);
    Q_UNUSED(pSuccess);
    Q_UNUSED(originalUrl);
    Q_UNUSED(isBackground);
    Q_UNUSED(pSize);
    Q_UNUSED(pPos);
    Q_UNUSED(sourceUrl);
    Q_UNUSED(pContentTypeHeader)

    QImage img;
    img.loadFromData(pData);
    QPixmap pix = QPixmap::fromImage(img);
    centralWidget->setPropertiesPixmap(pix);
    centralWidget->setPropertiesThumbnail(pix);
}

void WBFeaturesWidget::onAddDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentHeader, QByteArray pData, QString pTitle)
{
    if (pSuccess) {
        qDebug() << pData.length();
        controller->addDownloadedFile(sourceUrl, pData, pContentHeader, pTitle);
        controller->refreshModels();
    }
}

void WBFeaturesWidget::addElementsToFavorite()
{
    if ( centralWidget->currentView() == WBFeaturesCentralWidget::FeaturePropertiesList ) {
        WBFeature feature = centralWidget->getCurElementFromProperties();
        if ( feature != WBFeature() && !WBApplication::isFromWeb(feature.getFullPath().toString())) {
            controller->addToFavorite( feature.getFullPath() );
        }

    } else if ( centralWidget->currentView() == WBFeaturesCentralWidget::MainList ) {
        QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();
        for ( int i = 0; i < selected.size(); ++i ) {
            WBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<WBFeature>();
            controller->addToFavorite(feature.getFullPath());
       }
    }

    controller->refreshModels();
}

void WBFeaturesWidget::removeElementsFromFavorite()
{
    QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();
    QList <QUrl> items;
    for ( int i = 0; i < selected.size(); ++i )  {
        WBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<WBFeature>();
        items.append( feature.getFullPath() );
    }

    foreach ( QUrl url, items )  {
        controller->removeFromFavorite(url, true);
    }

    controller->refreshModels();
}

void WBFeaturesWidget::switchToListView()
{
//    stackedWidget->setCurrentIndex(ID_LISTVIEW);
//    currentStackedWidget = ID_LISTVIEW;
}

void WBFeaturesWidget::switchToProperties()
{
//    stackedWidget->setCurrentIndex(ID_PROPERTIES);
//    currentStackedWidget = ID_PROPERTIES;
}

void WBFeaturesWidget::switchToWebView()
{
//    stackedWidget->setCurrentIndex(ID_WEBVIEW);
//    currentStackedWidget = ID_WEBVIEW;
}

QStringList WBFeaturesMimeData::formats() const
{
    return QMimeData::formats();
}

void WBFeaturesWidget::importImage(const QImage &image, const QString &fileName)
{
    controller->importImage(image, fileName);
}

WBFeaturesListView::WBFeaturesListView( QWidget* parent, const char* name )
    : QListView(parent)
{
    setObjectName(name);
}

void WBFeaturesListView::dragEnterEvent( QDragEnterEvent *event )
{
    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() )
        event->acceptProposedAction();
}

void WBFeaturesListView::dragMoveEvent( QDragMoveEvent *event )
{
    const WBFeaturesMimeData *fMimeData = qobject_cast<const WBFeaturesMimeData*>(event->mimeData());
    QModelIndex index = indexAt(event->pos());
    WBFeature onFeature = model()->data(index, Qt::UserRole + 1).value<WBFeature>();
    if (fMimeData) {
        if (!index.isValid() || !onFeature.isFolder()) {
            event->ignore();
            return;
        }
        foreach (WBFeature curFeature, fMimeData->features()) {
            if (curFeature == onFeature) {
                event->ignore();
                return;
            }
        }
     }

    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() ) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void WBFeaturesListView::dropEvent( QDropEvent *event )
{
    QObject *eventSource = event->source();
    if (eventSource && eventSource->objectName() == WBFeaturesWidget::objNameFeatureList) {
        event->setDropAction( Qt::MoveAction );
    }

    QListView::dropEvent( event );
}

void WBFeaturesListView::thumbnailSizeChanged( int value )
{
    setIconSize(QSize(value, value));
    setGridSize(QSize(value + 20, value + 20 ));

    WBSettings::settings()->featureSliderPosition->set(value);
}

WBFeaturesNavigatorWidget::WBFeaturesNavigatorWidget(QWidget *parent, const char *name) :
    QWidget(parent), mListView(0), mListSlider(0)

{
    name = "WBFeaturesNavigatorWidget";

    setObjectName(name);
//    SET_STYLE_SHEET()

    mListView = new WBFeaturesListView(this, WBFeaturesWidget::objNameFeatureList);

    mListSlider = new QSlider(Qt::Horizontal, this);

    mListSlider->setMinimum(WBFeaturesWidget::minThumbnailSize);
    mListSlider->setMaximum(WBFeaturesWidget::maxThumbnailSize);
    mListSlider->setValue(WBFeaturesWidget::minThumbnailSize);
    mListSlider->setMinimumHeight(20);

    mListView->setParent(this);
    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mainLayer->addWidget(mListView, 1);
    mainLayer->addWidget(mListSlider, 0);
    mainLayer->setMargin(0);

    connect(mListSlider, SIGNAL(valueChanged(int)), mListView, SLOT(thumbnailSizeChanged(int)));
}

void WBFeaturesNavigatorWidget::setSliderPosition(int pValue)
{
    mListSlider->setValue(pValue);
}

WBFeaturesCentralWidget::WBFeaturesCentralWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("WBFeaturesCentralWidget");
    SET_STYLE_SHEET();

    QVBoxLayout *mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    //Maintains the view of the main part of the palette. Consists of
    //mNavigator
    //featureProperties
    //webVeiw
    mStackedWidget = new QStackedWidget(this);

    //Main features icon view with QSlider on the bottom
    mNavigator = new WBFeaturesNavigatorWidget(this);

    //Specifies the properties of a standalone element
    mFeatureProperties = new WBFeatureProperties(this);

    //Used to show search bar on the search widget
    webView = new WBFeaturesWebView(this);

    //filling stackwidget
    mStackedWidget->addWidget(mNavigator);
    mStackedWidget->addWidget(mFeatureProperties);
    mStackedWidget->addWidget(webView);
    mStackedWidget->setCurrentIndex(MainList);
    mStackedWidget->setContentsMargins(0, 0, 0, 0);


    mAdditionalDataContainer = new QStackedWidget(this);
    mAdditionalDataContainer->setObjectName("mAdditionalDataContainer");

    //New folder dialog
    WBFeaturesNewFolderDialog *dlg = new WBFeaturesNewFolderDialog(mAdditionalDataContainer);
    mAdditionalDataContainer->addWidget(dlg);
    mAdditionalDataContainer->setCurrentIndex(NewFolderDialog);

    connect(dlg, SIGNAL(createNewFolder(QString)), this, SLOT(createNewFolderSlot(QString)));
    connect(dlg, SIGNAL(closeDialog()), this, SLOT(hideAdditionalData()));
    connect(this, SIGNAL(sendFileNameList(QStringList)), dlg, SLOT(setFileNameList(QStringList)));

    //Progress bar to show scanning progress
    WBFeaturesProgressInfo *progressBar = new WBFeaturesProgressInfo();
    mAdditionalDataContainer->addWidget(progressBar);
    mAdditionalDataContainer->setCurrentIndex(ProgressBarWidget);

    connect(this, SIGNAL(maxFilesCountEvaluated(int)), progressBar, SLOT(setProgressMax(int)));
    connect(this, SIGNAL(increaseStatusBarValue()), progressBar, SLOT(increaseProgressValue()));
    connect(this, SIGNAL(scanCategory(QString)), progressBar, SLOT(setCommmonInfoText(QString)));
    connect(this, SIGNAL(scanPath(QString)), progressBar, SLOT(setDetailedInfoText(QString)));

    mLayout->addWidget(mStackedWidget, 1);
    mLayout->addWidget(mAdditionalDataContainer, 0);

    mAdditionalDataContainer->hide();
}

void WBFeaturesCentralWidget::showElement(const WBFeature &feature, StackElement pView)
{
    if (pView == FeaturesWebView) {
        webView->showElement(feature);
        mStackedWidget->setCurrentIndex(FeaturesWebView);
    } else if (pView == FeaturePropertiesList) {
        mFeatureProperties->showElement(feature);
        mStackedWidget->setCurrentIndex(FeaturePropertiesList);
    }
}

void WBFeaturesCentralWidget::switchTo(StackElement pView)
{
    mStackedWidget->setCurrentIndex(pView);
}

void WBFeaturesCentralWidget::setPropertiesPixmap(const QPixmap &pix)
{
    mFeatureProperties->setOrigPixmap(pix);
}

void WBFeaturesCentralWidget::setPropertiesThumbnail(const QPixmap &pix)
{
    mFeatureProperties->setThumbnail(pix);
}

WBFeature WBFeaturesCentralWidget::getCurElementFromProperties()
{
    return mFeatureProperties->getCurrentElement();
}

void WBFeaturesCentralWidget::showAdditionalData(AddWidget pWidgetType, AddWidgetState pState)
{
    if (!mAdditionalDataContainer->widget(pWidgetType)) {
        qDebug() << "can't find widget specified by WBFeaturesCentralWidget::showAdditionalData(AddWidget pWidgetType, AddWidgetState pState)";
        return;
    }

    mAdditionalDataContainer->setMaximumHeight(mAdditionalDataContainer->widget(pWidgetType)->sizeHint().height());

    mAdditionalDataContainer->setCurrentIndex(pWidgetType);
    mAdditionalDataContainer->show();
    emit lockMainWidget(pState == Modal ? true : false);
}

void WBFeaturesCentralWidget::setLockedExcludingAdditional(bool pLock)
{
//    Lock all the members excluding mAdditionalDataContainer
    mStackedWidget->setEnabled(!pLock);
}

void WBFeaturesCentralWidget::createNewFolderSlot(QString pStr)
{
    emit createNewFolderSignal(pStr);
    hideAdditionalData();
}

void WBFeaturesCentralWidget::hideAdditionalData()
{
    emit lockMainWidget(false);
    mAdditionalDataContainer->hide();
}

void WBFeaturesCentralWidget::scanStarted()
{
    showAdditionalData(ProgressBarWidget);
}

void WBFeaturesCentralWidget::scanFinished()
{
    hideAdditionalData();
}

WBFeaturesNewFolderDialog::WBFeaturesNewFolderDialog(QWidget *parent) : QWidget(parent)
  , acceptText(tr("Accept"))
  , cancelText(tr("Cancel"))
  , labelText(tr("Enter a new folder name"))
{
    this->setStyleSheet("QPushButton { background:white; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();
    labelLayout->setSizeConstraint(QLayout::SetMinimumSize);

    QLabel *mLabel = new QLabel(labelText, this);
    mLabel->setWordWrap(true);
    mLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    mLineEdit = new QLineEdit(this);

    mValidator = new QRegExpValidator(QRegExp("[^\\/\\:\\?\\*\\|\\<\\>\\\"]{2,}"), this);
    mLineEdit->setValidator(mValidator);
    labelLayout->addWidget(mLabel);
    labelLayout->addWidget(mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton = new QPushButton(acceptText, this);
    QPushButton *cancelButton = new QPushButton(cancelText, this);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    acceptButton->setEnabled(false);

    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mLineEdit, SIGNAL(textEdited(QString)), this, SLOT(reactOnTextChanged(QString)));

    reactOnTextChanged(QString());
}

void WBFeaturesNewFolderDialog::setRegexp(const QRegExp pRegExp)
{
    mValidator->setRegExp(pRegExp);
}
bool WBFeaturesNewFolderDialog::validString(const QString &pStr)
{
    return mLineEdit->hasAcceptableInput() && !mFileNameList.contains(pStr, Qt::CaseSensitive);
}

void WBFeaturesNewFolderDialog::accept()
{
//     Setting all the constraints we need
    emit createNewFolder(mLineEdit->text());
    mLineEdit->clear();
}
void WBFeaturesNewFolderDialog::reject()
{
    mLineEdit->clear();
    emit closeDialog();
}
void WBFeaturesNewFolderDialog::setFileNameList(const QStringList &pLst)
{
    mFileNameList = pLst;
}
void WBFeaturesNewFolderDialog::reactOnTextChanged(const QString &pStr)
{
    if (validString(pStr)) {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:white;");
    } else {
        acceptButton->setEnabled(false);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
    }
}

WBFeaturesProgressInfo::WBFeaturesProgressInfo(QWidget *parent) :
    QWidget(parent),
    mProgressBar(0),
    mCommonInfoLabel(0),
    mDetailedInfoLabel(0)
{
    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mProgressBar = new QProgressBar(this);
//    setting defaults
    mProgressBar->setMinimum(0);
    mProgressBar->setMaximum(100000);
    mProgressBar->setValue(0);

    mProgressBar->setStyleSheet("background:white");

    mCommonInfoLabel = new QLabel(this);
    mDetailedInfoLabel = new QLabel(this);
    mDetailedInfoLabel->setAlignment(Qt::AlignRight);
    mCommonInfoLabel->hide();
    mDetailedInfoLabel->hide();

    mainLayer->addWidget(mCommonInfoLabel);
    mainLayer->addWidget(mDetailedInfoLabel);
    mainLayer->addWidget(mProgressBar);
}

void WBFeaturesProgressInfo::setCommmonInfoText(const QString &str)
{
    mProgressBar->setFormat(tr("Loading ") + str + " (%p%)");
}

void WBFeaturesProgressInfo::setDetailedInfoText(const QString &str)
{
    mDetailedInfoLabel->setText(str);
}

void WBFeaturesProgressInfo::setProgressMax(int pValue)
{
    mProgressBar->setMaximum(pValue);
}

void WBFeaturesProgressInfo::setProgressMin(int pValue)
{
    mProgressBar->setMinimum(pValue);
}

void WBFeaturesProgressInfo::increaseProgressValue()
{
    mProgressBar->setValue(mProgressBar->value() + 1);
}

void WBFeaturesProgressInfo::sendFeature(WBFeature pFeature)
{
    Q_UNUSED(pFeature);
}


WBFeaturesWebView::WBFeaturesWebView(QWidget* parent, const char* name):QWidget(parent)
    , mpView(NULL)
    , mpWebSettings(NULL)
    , mpLayout(NULL)
    , mpSankoreAPI(NULL)
{
    setObjectName(name);

    SET_STYLE_SHEET();

    mpLayout = new QVBoxLayout();
    setLayout(mpLayout);

    mpView = new QWebEngineView(this);
    mpView->setObjectName("SearchEngineView");
    mpSankoreAPI = new WBWidgetUniboardAPI(WBApplication::boardController->activeScene());
    //mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
    //connect(mpView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(javaScriptWindowObjectCleared()));
    mpWebSettings = QWebEngineSettings::globalSettings();
    mpWebSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    mpWebSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    mpWebSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    //mpWebSettings->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, true);
    //mpWebSettings->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled, true);
    mpWebSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    //mpWebSettings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    mpWebSettings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    mpLayout->addWidget(mpView);
    mpLayout->setMargin(0);

    connect(mpView, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
}

WBFeaturesWebView::~WBFeaturesWebView()
{
    if( NULL != mpSankoreAPI )
    {
        delete mpSankoreAPI;
        mpSankoreAPI = NULL;
    }
    if( NULL != mpView )
    {
        delete mpView;
        mpView = NULL;
    }
    if( NULL != mpLayout )
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}

void WBFeaturesWebView::javaScriptWindowObjectCleared()
{
   // mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
}

void WBFeaturesWebView::showElement(const WBFeature &elem)
{
    QString qsWidgetName;
    QString path = elem.getFullPath().toLocalFile();

    QString qsConfigPath = QString("%0/config.xml").arg(path);

    if(QFile::exists(qsConfigPath))
    {
        QFile f(qsConfigPath);
        if(f.open(QIODevice::ReadOnly))
        {
            QDomDocument domDoc;
            domDoc.setContent(QString(f.readAll()));
            QDomElement root = domDoc.documentElement();

            QDomNode node = root.firstChild();
            while(!node.isNull())
            {
                if(node.toElement().tagName() == "content")
                {
                    QDomAttr srcAttr = node.toElement().attributeNode("src");
                    qsWidgetName = srcAttr.value();
                    break;
                }
                node = node.nextSibling();
            }
            f.close();
        }
    }

    mpView->load(QUrl::fromLocalFile(QString("%0/%1").arg(path).arg(qsWidgetName)));
}

void WBFeaturesWebView::onLoadFinished(bool ok)
{
    if(ok && NULL != mpSankoreAPI){
       // mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
    }
}


WBFeatureProperties::WBFeatureProperties( QWidget *parent, const char *name ) : QWidget(parent)
    , mpLayout(NULL)
    , mpButtonLayout(NULL)
    , mpAddPageButton(NULL)
    , mpAddToLibButton(NULL)
    , mpObjInfoLabel(NULL)
    , mpObjInfos(NULL)
    , mpThumbnail(NULL)
    , mpOrigPixmap(NULL)
    , mpElement(NULL)
{
    setObjectName(name);

    // Create the GUI
    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    maxThumbHeight = height() / 4;

    mpThumbnail = new QLabel();
    QPixmap icon(":images/libpalette/notFound.png");
    icon.scaledToWidth(THUMBNAIL_WIDTH);

    mpThumbnail->setPixmap(icon);
    mpThumbnail->setObjectName("DockPaletteWidgetBox");
    mpThumbnail->setStyleSheet("background:white;");
    mpThumbnail->setAlignment(Qt::AlignHCenter);
    mpLayout->addWidget(mpThumbnail, 0);

    mpButtonLayout = new QHBoxLayout();
    mpLayout->addLayout(mpButtonLayout, 0);

    mpAddPageButton = new WBFeatureItemButton();
    mpAddPageButton->setText(tr("Add to page"));
    mpButtonLayout->addWidget(mpAddPageButton);

    mpAddToLibButton = new WBFeatureItemButton();
    mpAddToLibButton->setText(tr("Add to library"));
    mpButtonLayout->addWidget(mpAddToLibButton);

    mpButtonLayout->addStretch(1);

    mpObjInfoLabel = new QLabel(tr("Object informations"));
    mpObjInfoLabel->setStyleSheet(QString("color: #888888; font-size : 18px; font-weight:bold;"));
    mpLayout->addWidget(mpObjInfoLabel, 0);

    mpObjInfos = new QTreeWidget(this);
    mpObjInfos->setColumnCount(2);
    mpObjInfos->header()->hide();
    mpObjInfos->setAlternatingRowColors(true);
    mpObjInfos->setRootIsDecorated(false);
    mpObjInfos->setObjectName("DockPaletteWidgetBox");
    mpObjInfos->setStyleSheet("background:white;");
    mpLayout->addWidget(mpObjInfos, 1);
    mpLayout->setMargin(0);

    connect( mpAddPageButton, SIGNAL(clicked()), this, SLOT(onAddToPage()) );
    connect( mpAddToLibButton, SIGNAL( clicked() ), this, SLOT(onAddToLib() ) );
}

WBFeatureProperties::~WBFeatureProperties()
{
    if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
    if ( mpElement )
    {
        delete mpElement;
        mpElement = NULL;
    }
    if ( mpThumbnail )
    {
        delete mpThumbnail;
        mpThumbnail = NULL;
    }
    if ( mpButtonLayout )
    {
        delete mpButtonLayout;
        mpButtonLayout = NULL;
    }
    if ( mpAddPageButton )
    {
        delete mpAddPageButton;
        mpAddPageButton = NULL;
    }
    if ( mpAddToLibButton )
    {
        delete mpAddToLibButton;
        mpAddToLibButton = NULL;
    }
    if ( mpObjInfoLabel )
    {
        delete mpObjInfoLabel;
        mpObjInfoLabel = NULL;
    }
    if ( mpObjInfos )
    {
        delete mpObjInfos;
        mpObjInfos = NULL;
    }
}

void WBFeatureProperties::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

void WBFeatureProperties::showEvent (QShowEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

WBFeature WBFeatureProperties::getCurrentElement() const
{
    if ( mpElement )
        return *mpElement;

    return WBFeature();
}

void WBFeatureProperties::setOrigPixmap(const QPixmap &pix)
{

    if (mpOrigPixmap)
        delete mpOrigPixmap;

    mpOrigPixmap = new QPixmap(pix);
}

void WBFeatureProperties::setThumbnail(const QPixmap &pix)
{
    mpThumbnail->setPixmap(pix.scaledToWidth(THUMBNAIL_WIDTH));
    adaptSize();
}

void WBFeatureProperties::adaptSize()
{
    if( NULL != mpOrigPixmap )
    {
        if( width() < THUMBNAIL_WIDTH + 40 )
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( width() - 40 ) );
        }
        else
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( THUMBNAIL_WIDTH ) );
        }
    }
}

void WBFeatureProperties::showElement(const WBFeature &elem)
{
    if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
    if ( mpElement )
    {
        delete mpElement;
        mpElement = NULL;
    }
    mpElement = new WBFeature(elem);
    mpOrigPixmap = new QPixmap(QPixmap::fromImage(elem.getThumbnail()));
    mpThumbnail->setPixmap(QPixmap::fromImage(elem.getThumbnail()).scaledToWidth(THUMBNAIL_WIDTH));
    populateMetadata();

    if ( WBApplication::isFromWeb( elem.getFullPath().toString() ) )
    {
        mpAddToLibButton->show();
    }
    else
    {
        mpAddToLibButton->hide();
    }
}

void WBFeatureProperties::populateMetadata()
{
    if(NULL != mpObjInfos){
        mpObjInfos->clear();
        QMap<QString, QString> metas = mpElement->getMetadata();
        QList<QString> lKeys = metas.keys();
        QList<QString> lValues = metas.values();

        for(int i=0; i< metas.size(); i++){
            QStringList values;
            values << lKeys.at(i);
            values << lValues.at(i);
            mpItem = new QTreeWidgetItem(values);
            mpObjInfos->addTopLevelItem(mpItem);
        }
        mpObjInfos->resizeColumnToContents(0);
    }
}

void WBFeatureProperties::onAddToPage()
{
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    WBFeaturesWidget* featuresWidget = qobject_cast<WBFeaturesWidget*>( w );
    if (featuresWidget)
        featuresWidget->getFeaturesController()->addItemToPage( *mpElement );
}

void WBFeatureProperties::onAddToLib()
{
    if ( WBApplication::isFromWeb(  mpElement->getFullPath().toString() ) )
    {
        sDownloadFileDesc desc;
        desc.isBackground = false;
        desc.modal = false;
        desc.dest = sDownloadFileDesc::library;
        desc.name = mpElement->getMetadata().value("Title", QString());
        qDebug() << desc.name;
        desc.srcUrl = mpElement->getFullPath().toString();
        QString str1 = mpElement->getFullPath().toString().normalized(QString::NormalizationForm_C);
        QString str2 = mpElement->getFullPath().toString().normalized(QString::NormalizationForm_D);
        QString str3 = mpElement->getFullPath().toString().normalized(QString::NormalizationForm_KC);
        QString str4 = mpElement->getFullPath().toString().normalized(QString::NormalizationForm_KD);
        qDebug() << desc.srcUrl << endl
                    << "str1" << str1 << endl
                    << "str2" << str2 << endl
                    << "str3" << str3 << endl
                    << "str4" << str4 << endl;
        WBDownloadManager::downloadManager()->addFileToDownload(desc);
    }
}


void WBFeatureProperties::onSetAsBackground()
{
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    WBFeaturesWidget* featuresWidget = qobject_cast<WBFeaturesWidget*>( w );
    featuresWidget->getFeaturesController()->addItemAsBackground( *mpElement );
}



WBFeatureItemButton::WBFeatureItemButton(QWidget *parent, const char *name):QPushButton(parent)
{
    setObjectName(name);
    setStyleSheet(QString("background-color : #DDDDDD; color : #555555; border-radius : 6px; padding : 5px; font-weight : bold; font-size : 12px;"));
}

WBFeatureItemButton::~WBFeatureItemButton()
{
}

QVariant WBFeaturesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return featuresList->at(index.row()).getDisplayName();
    }

    else if (role == Qt::DecorationRole) {
        return QIcon( QPixmap::fromImage(featuresList->at(index.row()).getThumbnail()));

    } else if (role == Qt::UserRole) {
        return featuresList->at(index.row()).getVirtualPath();

    }    else if (role == Qt::UserRole + 1) {
        //return featuresList->at(index.row()).getType();
        WBFeature f = featuresList->at(index.row());
        return QVariant::fromValue( f );
    }

    return QVariant();
}

QMimeData* WBFeaturesModel::mimeData(const QModelIndexList &indexes) const
{
    WBFeaturesMimeData *mimeData = new WBFeaturesMimeData();
    QList <QUrl> urlList;
    QList <WBFeature> featuresList;
    QByteArray typeData;

    foreach (QModelIndex index, indexes) {

        if (index.isValid()) {
            WBFeature element = data(index, Qt::UserRole + 1).value<WBFeature>();
            urlList.push_back( element.getFullPath() );
            QString curPath = element.getFullPath().toLocalFile();
            featuresList.append(element);

            if (!typeData.isNull()) {
                typeData += WBFeaturesController::featureTypeSplitter();
            }
            typeData += QString::number(element.getType()).toLatin1();
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setFeatures(featuresList);
    mimeData->setData(mimeSankoreFeatureTypes, typeData);

    return mimeData;
}

bool WBFeaturesModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row)

    const WBFeaturesMimeData *fMimeData = qobject_cast<const WBFeaturesMimeData*>(mimeData);
    WBFeaturesController *curController = qobject_cast<WBFeaturesController *>(QObject::parent());

    bool dataFromSameModel = false;

    if (fMimeData)
        dataFromSameModel = true;

    if ((!mimeData->hasUrls() && !mimeData->hasImage()) )
        return false;
    if ( action == Qt::IgnoreAction )
        return true;
    if ( column > 0 )
        return false;

    WBFeature parentFeature;
    if (!parent.isValid()) {
        parentFeature = curController->getCurrentElement();
    } else {
        parentFeature = parent.data( Qt::UserRole + 1).value<WBFeature>();
    }

    if (dataFromSameModel) {
        QList<WBFeature> featList = fMimeData->features();
        for (int i = 0; i < featList.count(); i++) {
            WBFeature sourceElement;
            if (dataFromSameModel) {
                sourceElement = featList.at(i);
                moveData(sourceElement, parentFeature, Qt::MoveAction, true);
            }
        }
    } else if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        foreach (QUrl curUrl, urlList) {
            qDebug() << "URl catched is " << curUrl.toLocalFile();
            curController->moveExternalData(curUrl, parentFeature);
        }
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>( mimeData->imageData() );
        curController->importImage( image, parentFeature );

    }

    return true;
}

void WBFeaturesModel::addItem( const WBFeature &item )
{
    beginInsertRows( QModelIndex(), featuresList->size(), featuresList->size() );
    featuresList->append( item );
    endInsertRows();
}

void WBFeaturesModel::deleteFavoriteItem( const QString &path )
{
    for ( int i = 0; i < featuresList->size(); ++i )
    {
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) &&
            !QString::compare( featuresList->at(i).getVirtualPath(), "/root/favorites", Qt::CaseInsensitive ) )
        {
            removeRow( i, QModelIndex() );
            return;
        }
    }
}

void WBFeaturesModel::deleteItem( const QString &path )
{
    for ( int i = 0; i < featuresList->size(); ++i )
    {
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) )
        {
            removeRow( i, QModelIndex() );
            return;
        }
    }
}

void WBFeaturesModel::deleteItem(const WBFeature &feature)
{
    int i = featuresList->indexOf(feature);
    if (i == -1) {
        qDebug() << "no matches in deleting item from UBFEaturesModel";
        return;
    }
    removeRow(i, QModelIndex());
}

bool WBFeaturesModel::removeRows( int row, int count, const QModelIndex & parent )
{
    if ( row < 0 )
        return false;
    if ( row + count > featuresList->size() )
        return false;
    beginRemoveRows( parent, row, row + count - 1 );
    //featuresList->remove( row, count );
    featuresList->erase( featuresList->begin() + row, featuresList->begin() + row + count );
    endRemoveRows();
    return true;
}

bool WBFeaturesModel::removeRow(  int row, const QModelIndex & parent )
{
    if ( row < 0 )
        return false;
    if ( row >= featuresList->size() )
        return false;
    beginRemoveRows( parent, row, row );
    //featuresList->remove( row );
    featuresList->erase( featuresList->begin() + row );
    endRemoveRows();
    return true;
}

void WBFeaturesModel::moveData(const WBFeature &source, const WBFeature &destination
                               , Qt::DropAction action = Qt::CopyAction, bool deleteManualy)
{
    WBFeaturesController *curController = qobject_cast<WBFeaturesController *>(QObject::parent());
    if (!curController)
        return;

    QString sourcePath = source.getFullPath().toLocalFile();
    QString sourceVirtualPath = source.getVirtualPath();

    WBFeatureElementType sourceType = source.getType();
    QImage sourceIcon = source.getThumbnail();

    if (sourceType == FEATURE_INTERNAL) {
        qWarning() << "Built-in tools cannot be moved";
        return;
    }

    Q_ASSERT( QFileInfo( sourcePath ).exists() );

    QString name = QFileInfo( sourcePath ).fileName();
    QString destPath = destination.getFullPath().toLocalFile();

    QString destVirtualPath = destination.getFullVirtualPath();
    QString destFullPath = destPath + "/" + name;

    if ( sourcePath.compare(destFullPath, Qt::CaseInsensitive ) || destination.getType() != FEATURE_TRASH)
    {
        WBFileSystemUtils::copy(sourcePath, destFullPath);
        if (action == Qt::MoveAction) {
            curController->deleteItem( source.getFullPath() );
        }
    }

    //Passing all the source container ubdating dependancy pathes
    if (sourceType == FEATURE_FOLDER) {
        for (int i = 0; i < featuresList->count(); i++) {

            WBFeature &curFeature = (*featuresList)[i];

            QString curFeatureFullPath = curFeature.getFullPath().toLocalFile();
            QString curFeatureVirtualPath = curFeature.getVirtualPath();

            if (curFeatureFullPath.contains(sourcePath) && curFeatureFullPath != sourcePath) {

                WBFeature copyFeature = curFeature;
                QUrl newPath = QUrl::fromLocalFile(curFeatureFullPath.replace(sourcePath, destFullPath));
                QString newVirtualPath = curFeatureVirtualPath.replace(sourceVirtualPath, destVirtualPath);
                //when copying to trash don't change the real path
                if (destination.getType() != FEATURE_TRASH) {
                    // processing copy or move action for real FS
                    if (action == Qt::CopyAction) {
                        copyFeature.setFullPath(newPath);
                    } else {
                        curFeature.setFullPath(newPath);
                    }
                }
                // processing copy or move action for virtual FS
                if (action == Qt::CopyAction) {
                    copyFeature.setFullVirtualPath(newVirtualPath);
                } else {
                    curFeature.setFullVirtualPath(newVirtualPath);
                }

                if (action == Qt::CopyAction) {
                    addItem(copyFeature);
                }
            }
        }
    }

    WBFeature newElement( destVirtualPath + "/" + name, sourceIcon, name, QUrl::fromLocalFile(destFullPath), sourceType );
    addItem(newElement);

    if (deleteManualy) {
        deleteItem(source);
    }

// Commented because of crashes on mac. But works fine. It is not predictable behavior. 
// Please uncomment it if model will not refreshes
//   emit dataRestructured();. 
}

Qt::ItemFlags WBFeaturesModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
    if ( index.isValid() )
    {
        WBFeature item = index.data( Qt::UserRole + 1 ).value<WBFeature>();
        if ( item.getType() == FEATURE_INTERACTIVE
             || item.getType() == FEATURE_ITEM
             || item.getType() == FEATURE_AUDIO
             || item.getType() == FEATURE_VIDEO
             || item.getType() == FEATURE_IMAGE
             || item.getType() == FEATURE_FLASH
             || item.getType() == FEATURE_INTERNAL
             || item.getType() == FEATURE_FOLDER)

            resultFlags |= Qt::ItemIsDragEnabled;

        if ( item.isFolder() && !item.getVirtualPath().isNull() )
            resultFlags |= Qt::ItemIsDropEnabled;
    }

    return resultFlags;
}


QStringList WBFeaturesModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

int WBFeaturesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !featuresList)
        return 0;
    else
        return featuresList->size();
}

bool WBFeaturesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString path = index.data( Qt::UserRole ).toString();

    return filterRegExp().exactMatch(path);
}

bool WBFeaturesSearchProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    /*QString name = sourceModel()->data(index, Qt::DisplayRole).toString();
    eUBLibElementType type = (eUBLibElementType)sourceModel()->data(index, Qt::UserRole + 1).toInt();*/

    WBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<WBFeature>();
    bool isFile = feature.getType() == FEATURE_INTERACTIVE
            || feature.getType() == FEATURE_INTERNAL
            || feature.getType() == FEATURE_ITEM
            || feature.getType() == FEATURE_AUDIO
            || feature.getType() == FEATURE_VIDEO
            || feature.getType() == FEATURE_IMAGE;

    return isFile
            && feature.getFullVirtualPath().contains(mFilterPrefix)
            && filterRegExp().exactMatch( feature.getName() );
}

bool WBFeaturesPathProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    WBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<WBFeature>();

    // We want to display parent folders up to and including the current one
    return (feature.isFolder()
            && ( path.startsWith(feature.getFullVirtualPath() + "/")
                 || path == feature.getFullVirtualPath()));

}

QString    WBFeaturesItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(locale)

    QString text = value.toString();
    text = text.replace(".wgt", "");
    text = text.replace(".wgs", "");
    text = text.replace(".swf","");
    if (listView)
    {
        const QFontMetrics fm = listView->fontMetrics();
        const QSize iSize = listView->gridSize();
        return elidedText( fm, iSize.width(), Qt::ElideRight, text );
    }
    return text;
}

WBFeaturesPathItemDelegate::WBFeaturesPathItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    arrowPixmap = new QPixmap(":images/navig_arrow.png");
}

QString WBFeaturesPathItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)

    return QString();
}

void WBFeaturesPathItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    WBFeature feature = index.data( Qt::UserRole + 1 ).value<WBFeature>();
    QRect rect = option.rect;
    if ( !feature.getFullPath().isEmpty() )
    {
        painter->drawPixmap( rect.left() - 10, rect.center().y() - 5, *arrowPixmap );
    }
    painter->drawImage( rect.left() + 5, rect.center().y() - 5, feature.getThumbnail().scaledToHeight( 30, Qt::SmoothTransformation ) );
}

WBFeaturesPathItemDelegate::~WBFeaturesPathItemDelegate()
{
    if ( arrowPixmap )
    {
        delete arrowPixmap;
        arrowPixmap = NULL;
    }
}
