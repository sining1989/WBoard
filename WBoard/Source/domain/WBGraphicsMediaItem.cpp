#include "WBGraphicsGroupContainerItem.h"
#include "WBGraphicsMediaItem.h"
#include "WBGraphicsMediaItemDelegate.h"
#include "WBGraphicsScene.h"
#include "WBGraphicsDelegateFrame.h"
#include "document/WBDocumentProxy.h"
#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "core/memcheck.h"

#include <QGraphicsVideoItem>

bool WBGraphicsMediaItem::sIsMutedByDefault = false;

WBGraphicsMediaItem* WBGraphicsMediaItem::createMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem* parent)
{
    WBGraphicsMediaItem * mediaItem;

    QString mediaPath = pMediaFileUrl.toString();
    if ("" == mediaPath)
        mediaPath = pMediaFileUrl.toLocalFile();

    if (mediaPath.toLower().contains("videos"))
        mediaItem = new WBGraphicsVideoItem(pMediaFileUrl, parent);
    else if (mediaPath.toLower().contains("audios"))
        mediaItem = new WBGraphicsAudioItem(pMediaFileUrl, parent);

    return mediaItem;
}

WBGraphicsMediaItem::WBGraphicsMediaItem(const QUrl& pMediaFileUrl, QGraphicsItem *parent)
        : QGraphicsRectItem(parent)
        , mMuted(sIsMutedByDefault)
        , mMutedByUserAction(sIsMutedByDefault)
        , mStopped(false)
        , mMediaFileUrl(pMediaFileUrl)
        , mLinkedImage(NULL)
        , mInitialPos(0)
{

    mErrorString = "";

    mMediaObject = new QMediaPlayer(this);
    mMediaObject->setMedia(pMediaFileUrl);

    setDelegate(new WBGraphicsMediaItemDelegate(this));

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem));
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);

    connect(mMediaObject, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            Delegate(), SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

    connect(mMediaObject, SIGNAL(stateChanged(QMediaPlayer::State)),
            Delegate(), SLOT(mediaStateChanged(QMediaPlayer::State)));

    connect(mMediaObject, SIGNAL(positionChanged(qint64)),
            Delegate(), SLOT(updateTicker(qint64)));

    connect(mMediaObject, SIGNAL(durationChanged(qint64)),
            Delegate(), SLOT(totalTimeChanged(qint64)));

    connect(Delegate(), SIGNAL(showOnDisplayChanged(bool)),
            this, SLOT(showOnDisplayChanged(bool)));

    connect(mMediaObject, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, &WBGraphicsMediaItem::mediaError);
}

WBGraphicsAudioItem::WBGraphicsAudioItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
    :WBGraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = false;

    Delegate()->createControls();
    Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::ResizingHorizontally);

    this->setSize(320, 26);
    this->setMinimumSize(QSize(150, 26));

    mMediaObject->setNotifyInterval(1000);

}

WBGraphicsVideoItem::WBGraphicsVideoItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
    :WBGraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = true;
    setPlaceholderVisible(true);
    Delegate()->createControls();

    mVideoItem = new QGraphicsVideoItem(this);

    mVideoItem->setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);
    mVideoItem->setFlag(ItemStacksBehindParent, true);

    /* setVideoOutput has to be called only when the video item is visible on the screen,
     * due to a Qt bug (QTBUG-32522). So instead of calling it here, it is called when the
     * active scene has changed, or when the item is first created.
     * If and when Qt fix this issue, this should be changed back.
     * */
    //mMediaObject->setVideoOutput(mVideoItem);
    mHasVideoOutput = false;

    mMediaObject->setNotifyInterval(50);

    setMinimumSize(QSize(320, 240));
    setSize(320, 240);

    connect(mVideoItem, SIGNAL(nativeSizeChanged(QSizeF)),
            this, SLOT(videoSizeChanged(QSizeF)));

    connect(mMediaObject, SIGNAL(videoAvailableChanged(bool)),
            this, SLOT(hasVideoChanged(bool)));

    connect(mMediaObject, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(mediaStateChanged(QMediaPlayer::State)));

    connect(mMediaObject, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, &WBGraphicsVideoItem::mediaError);

    setAcceptHoverEvents(true);

    update();
}

WBGraphicsMediaItem::~WBGraphicsMediaItem()
{
    if (mMediaObject) {
        mMediaObject->stop();
        delete mMediaObject;
    }
}

QVariant WBGraphicsMediaItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if ((change == QGraphicsItem::ItemEnabledChange)
            || (change == QGraphicsItem::ItemSceneChange)
            || (change == QGraphicsItem::ItemVisibleChange))
    {
        if (mMediaObject && (!isEnabled() || !isVisible() || !scene()))
            mMediaObject->pause();
    }
    else if (change == QGraphicsItem::ItemSceneHasChanged)
    {
        if (!scene())
            mMediaObject->stop();
        else {
            QString absoluteMediaFilename;

            if(mMediaFileUrl.toLocalFile().startsWith("audios/") || mMediaFileUrl.toLocalFile().startsWith("videos/"))
                absoluteMediaFilename = scene()->document()->persistencePath() + "/"  + mMediaFileUrl.toLocalFile();
            else
                absoluteMediaFilename = mMediaFileUrl.toLocalFile();

            if (absoluteMediaFilename.length() > 0)
                  mMediaObject->setMedia(QUrl::fromLocalFile(absoluteMediaFilename));

        }
    }

    if (Delegate()) {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsRectItem::itemChange(change, newValue);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void WBGraphicsMediaItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    //painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}


QMediaPlayer::State WBGraphicsMediaItem::playerState() const
{
    return mMediaObject->state();
}

/**
 * @brief Returns true if the video was manually stopped, false otherwise.
 */
bool WBGraphicsMediaItem::isStopped() const
{
    return mStopped;
}

qint64 WBGraphicsMediaItem::mediaDuration() const
{
    return mMediaObject->duration();
}

qint64 WBGraphicsMediaItem::mediaPosition() const
{
    return mMediaObject->position();
}

bool WBGraphicsMediaItem::isMediaSeekable() const
{
    return mMediaObject->isSeekable();
}

/**
 * @brief Set the item's minimum size. If the current size is smaller, it will be resized.
 * @param size The new minimum size
 */
void WBGraphicsMediaItem::setMinimumSize(const QSize& size)
{
    mMinimumSize = size;

    QSizeF newSize = rect().size();
    int width = newSize.width();
    int height = newSize.height();

    if (rect().width() < mMinimumSize.width())
        width = mMinimumSize.width();

    if (rect().height() < mMinimumSize.height())
        height = mMinimumSize.height();

    this->setSize(width, height);
}

void WBGraphicsMediaItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid));
}

void WBGraphicsMediaItem::setMediaFileUrl(QUrl url)
{
    mMediaFileUrl = url;
}

void WBGraphicsMediaItem::setInitialPos(qint64 p)
{
    mInitialPos = p;
}

void WBGraphicsMediaItem::setMediaPos(qint64 p)
{
    mMediaObject->setPosition(p);
}

void WBGraphicsMediaItem::setSelected(bool selected)
{
    if(selected){
        Delegate()->createControls();
        if (this->getMediaType() == mediaType_Audio)
            Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::ResizingHorizontally);
        else
            Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::Resizing);
    }
    QGraphicsRectItem::setSelected(selected);
}

void WBGraphicsMediaItem::setSourceUrl(const QUrl &pSourceUrl)
{
    WBItem::setSourceUrl(pSourceUrl);
}

void WBGraphicsMediaItem::clearSource()
{
    QString path = mediaFileUrl().toLocalFile();
    //if path is absolute clean duplicated path string
    if (!path.contains(WBApplication::boardController->selectedDocument()->persistencePath()))
        path = WBApplication::boardController->selectedDocument()->persistencePath() + "/" + path;

    if (!WBFileSystemUtils::deleteFile(path))
        qDebug() << "cannot delete file: " << path;
}

void WBGraphicsMediaItem::toggleMute()
{
    mMuted = !mMuted;
    setMute(mMuted);
}

void WBGraphicsMediaItem::setMute(bool bMute)
{
    mMuted = bMute;
    mMediaObject->setMuted(mMuted);
    mMutedByUserAction = mMuted;
    sIsMutedByDefault = mMuted;
}

WBGraphicsScene* WBGraphicsMediaItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}


void WBGraphicsMediaItem::activeSceneChanged()
{
    if (WBApplication::boardController->activeScene() != scene())
        mMediaObject->pause();
}


void WBGraphicsMediaItem::showOnDisplayChanged(bool shown)
{
    if (!shown) {
        mMuted = true;
        mMediaObject->setMuted(mMuted);
    }
    else if (!mMutedByUserAction) {
        mMuted = false;
        mMediaObject->setMuted(mMuted);
    }
}
void WBGraphicsMediaItem::play()
{
    mMediaObject->play();
    mStopped = false;
}

void WBGraphicsMediaItem::pause()
{
    mMediaObject->pause();
    mStopped = false;
}

void WBGraphicsMediaItem::stop()
{
    mMediaObject->stop();
    mStopped = true;
}

void WBGraphicsMediaItem::togglePlayPause()
{
    if (!mErrorString.isEmpty()) {
        WBApplication::showMessage("Can't play media: " + mErrorString);
        return;
    }

    if (mMediaObject->state() == QMediaPlayer::StoppedState)
        mMediaObject->play();

    else if (mMediaObject->state() == QMediaPlayer::PlayingState) {

        if ((mMediaObject->duration() - mMediaObject->position()) <= 0) {
            mMediaObject->stop();
            mMediaObject->play();
        }

        else {
            mMediaObject->pause();
            if(scene())
                scene()->setModified(true);
        }
    }

    else if (mMediaObject->state() == QMediaPlayer::PausedState) {
        if ((mMediaObject->duration() - mMediaObject->position()) <= 0)
            mMediaObject->stop();

        mMediaObject->play();
    }

    else  if ( mMediaObject->mediaStatus() == QMediaPlayer::LoadingMedia) {
        mMediaObject->setMedia(mediaFileUrl());
        mMediaObject->play();
    }
}

void WBGraphicsMediaItem::mediaError(QMediaPlayer::Error errorCode)
{
    // QMediaPlayer::errorString() isn't very descriptive, so we generate our own message

    switch (errorCode) {
        case QMediaPlayer::NoError:
            mErrorString = "";
            break;
        case QMediaPlayer::ResourceError:
            mErrorString = tr("Media resource couldn't be resolved");
            break;
        case QMediaPlayer::FormatError:
            mErrorString = tr("Unsupported media format");
            break;
        case QMediaPlayer::ServiceMissingError:
            mErrorString = tr("Media playback service not found");
            break;
        default:
            mErrorString = tr("Media error: ") + QString(errorCode) + " (" + mMediaObject->errorString() + ")";
    }

    if (!mErrorString.isEmpty() ) {
        WBApplication::showMessage(mErrorString);
        qDebug() << mErrorString;
    }
}

void WBGraphicsMediaItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsMediaItem *cp = dynamic_cast<WBGraphicsMediaItem*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));
        cp->setSourceUrl(this->sourceUrl());
        cp->setSize(rect().width(), rect().height());

        cp->setZValue(this->zValue());

        connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), cp, SLOT(activeSceneChanged()));

    }
}

void WBGraphicsMediaItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()) {
        Delegate()->mousePressEvent(event);
        if (parentItem() && WBGraphicsGroupContainerItem::Type == parentItem()->type()) {
            WBGraphicsGroupContainerItem *group = qgraphicsitem_cast<WBGraphicsGroupContainerItem*>(parentItem());
            if (group) {
                QGraphicsItem *curItem = group->getCurrentItem();
                if (curItem && this != curItem)
                    group->deselectCurrentItem();
                group->setCurrentItem(this);
                this->setSelected(true);
                Delegate()->positionHandles();
            }
        }
    }

    if (parentItem() && parentItem()->type() == WBGraphicsGroupContainerItem::Type) {
        mShouldMove = false;
        if (!Delegate()->mousePressEvent(event))
            event->accept();
    }
    else {
        mShouldMove = (event->buttons() & Qt::LeftButton);
        mMousePressPos = event->scenePos();
        mMouseMovePos = mMousePressPos;

        event->accept();
        setSelected(true);
    }
    QGraphicsRectItem::mousePressEvent(event);
}

QRectF WBGraphicsMediaItem::boundingRect() const
{
    return rect();
}

void WBGraphicsMediaItem::setSize(int width, int height)
{
    QRectF r = rect();
    r.setWidth(width);
    r.setHeight(height);
    setRect(r);

    if (Delegate())
        Delegate()->positionHandles();
    if (scene())
        scene()->setModified(true);
}

WBItem* WBGraphicsAudioItem::deepCopy() const
{
    QUrl url = this->mediaFileUrl();
    WBGraphicsMediaItem *copy = new WBGraphicsAudioItem(url, parentItem());

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;
}

WBItem* WBGraphicsVideoItem::deepCopy() const
{
    QUrl url = this->mediaFileUrl();
    WBGraphicsMediaItem *copy = new WBGraphicsVideoItem(url, parentItem());

    copy->setUuid(this->uuid());
    copyItemParameters(copy);

    return copy;
}

void WBGraphicsVideoItem::setSize(int width, int height)
{
    // Resize the video, then the rest of the Item

    int sizeX = 0;
    int sizeY = 0;

    if (mMinimumSize.width() > width)
        sizeX = mMinimumSize.width();
    else
        sizeX = width;

    if (mMinimumSize.height() > height)
        sizeY = mMinimumSize.height();
    else
        sizeY = height;

    mVideoItem->setSize(QSize(sizeX, sizeY));


    WBGraphicsMediaItem::setSize(sizeX, sizeY);
}

void WBGraphicsVideoItem::videoSizeChanged(QSizeF newSize)
{
    /* Depending on the platform/video backend, video size information becomes
     * available at different times (either when the file is loaded, or when
     * playback begins), so this slot is needed to resize the video item as
     * soon as the information is available.
     */


    // We don't want the video item to resize when the video is stopped or finished;
    // and in those cases, the new size is reported as (0, 0).

    if (newSize != QSizeF(0,0))
        this->setSize(newSize.width(), newSize.height());

    else // Make sure the toolbar doesn't disappear
        Delegate()->showToolBar(false);
}

void WBGraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // When selected, a QGraphicsRectItem is drawn with a dashed line border. We don't want this
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    QGraphicsRectItem::paint(painter, &styleOption, widget);
    WBGraphicsMediaItem::paint(painter, option, widget);

}

QVariant WBGraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemVisibleChange
            && value.toBool()
            && !mHasVideoOutput
            && WBApplication::app()->boardController
            && WBApplication::app()->boardController->activeScene() == scene())
    {
        //qDebug() << "Item change, setting video output";

        mMediaObject->setVideoOutput(mVideoItem);
        mHasVideoOutput = true;
    }

    return WBGraphicsMediaItem::itemChange(change, value);
}

void WBGraphicsVideoItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // Display the seek bar
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void WBGraphicsVideoItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverMoveEvent(event);
}

void WBGraphicsVideoItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void WBGraphicsVideoItem::hasVideoChanged(bool hasVideo)
{
    // On Linux, this is called (with hasVideo == true) when the video is first played
    // and when it finishes (hasVideo == false). But on Windows and OS X, it isn't called when
    // the video finishes, so those platforms require another solution to showing/hiding the
    // placeholder black rectangle.

    setPlaceholderVisible(!hasVideo);
}

void WBGraphicsVideoItem::mediaStateChanged(QMediaPlayer::State state)
{

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    setPlaceholderVisible((state == QMediaPlayer::StoppedState));
#else
    Q_UNUSED(state);
#endif

}

void WBGraphicsVideoItem::activeSceneChanged()
{
    //qDebug() << "Active scene changed";

    // Update the visibility of the placeholder, to prevent it being hidden when switching pages
    setPlaceholderVisible(!mErrorString.isEmpty());

    // Call setVideoOutput, if the video is visible and if it hasn't been called already
    if (!mHasVideoOutput && WBApplication::boardController->activeScene() == scene()) {
        //qDebug() << "setting video output";
        mMediaObject->setMedia(mMediaFileUrl);
        mMediaObject->setVideoOutput(mVideoItem);
        mHasVideoOutput = true;
    }

    WBGraphicsMediaItem::activeSceneChanged();
}

void WBGraphicsVideoItem::mediaError(QMediaPlayer::Error errorCode)
{
    setPlaceholderVisible(errorCode != QMediaPlayer::NoError);
}

/**
 * @brief Set the brush and fill to display a black rectangle
 * @param visible If true, a black rectangle is displayed in place of the video
 *
 * Depending on platforms, when a video is finished or stopped, the video might be
 * removed altogether. To avoid just having the controls bar visible at that point,
 * we can display a "fake" black video frame in its place using this method.
 */
void WBGraphicsVideoItem::setPlaceholderVisible(bool visible)
{
    if (visible) {
        setBrush(QColor(Qt::black));
        setPen(QColor(Qt::white));
    }
    else {
        setBrush(QColor(Qt::transparent));
        setPen(QColor(Qt::transparent));
    }

}
