#include <QtWidgets>
#include <QtSvg>

#include "WBGraphicsMediaItem.h"
#include "WBGraphicsMediaItemDelegate.h"
#include "WBGraphicsDelegateFrame.h"

#include "WBGraphicsScene.h"

#include "core/WBSettings.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"

#include "domain/WBGraphicsMediaItem.h"

#include "core/memcheck.h"

WBGraphicsMediaItemDelegate::WBGraphicsMediaItemDelegate(WBGraphicsMediaItem* pDelegated, QObject * parent)
    : WBGraphicsItemDelegate(pDelegated, parent, GF_COMMON
                             | GF_RESPECT_RATIO
                             | GF_TOOLBAR_USED)
    , mPlayPauseButton(NULL)
    , mToolBarShowTimer(NULL)
    , m_iToolBarShowingInterval(5000)
{
    QPalette palette;
    palette.setBrush ( QPalette::Light, Qt::darkGray );

    if (delegated()->isMuted())
        delegated()->setMute(true);

}

bool WBGraphicsMediaItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    mToolBarItem->show();
    positionHandles();

    if (mToolBarShowTimer)
        mToolBarShowTimer->start();

    return WBGraphicsItemDelegate::mousePressEvent(event);
}

/**
 * @brief Show the toolbar (play/pause, seek, mute).
 *
 * The toolbar then auto-hides after a set amount of time, if the video is currently
 * playing or is paused.
 */
void WBGraphicsMediaItemDelegate::showToolBar(bool autohide)
{
    mToolBarItem->show();
    if (mToolBarShowTimer) {

        if (delegated()->isPlaying() || delegated()->isPaused())
            mToolBarShowTimer->start();
        else
            mToolBarShowTimer->stop();

        // Don't hide the toolbar if we're at the beginning of the video
        if (delegated()->mediaPosition() == delegated()->initialPos())
            mToolBarShowTimer->stop();

        // Don't hide the toolbar if it was explicitly requested
        if (!autohide)
            mToolBarShowTimer->stop();
    }
}

void WBGraphicsMediaItemDelegate::hideToolBar()
{
    mToolBarItem->hide();
}

void WBGraphicsMediaItemDelegate::buildButtons()
{
    if(!mPlayPauseButton){
        mPlayPauseButton = new DelegateButton(":/images/play.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mPlayPauseButton, SIGNAL(clicked(bool)),
                this, SLOT(togglePlayPause()));

        mStopButton = new DelegateButton(":/images/stop.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mStopButton, SIGNAL(clicked(bool)),
                delegated(), SLOT(stop()));

        mMediaControl = new DelegateMediaControl(delegated(), mToolBarItem);
        mMediaControl->setFlag(QGraphicsItem::ItemIsSelectable, true);
        WBGraphicsItem::assignZValue(mMediaControl, delegated()->zValue());

        if (delegated()->isMuted())
            mMuteButton = new DelegateButton(":/images/soundOff.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        else
            mMuteButton = new DelegateButton(":/images/soundOn.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);

        connect(mMuteButton, SIGNAL(clicked(bool)),
                delegated(), SLOT(toggleMute()));
        connect(mMuteButton, SIGNAL(clicked(bool)),
                this, SLOT(toggleMute())); // for changing button image

        mToolBarButtons << mPlayPauseButton << mStopButton << mMuteButton;

        mToolBarItem->setItemsOnToolBar(QList<QGraphicsItem*>() << mPlayPauseButton << mStopButton << mMediaControl  << mMuteButton );
        mToolBarItem->setVisibleOnBoard(true);
        mToolBarItem->setShifting(false);

        if (!mToolBarShowTimer) {
            if (delegated()->hasLinkedImage()) {
                mToolBarShowTimer = new QTimer();
                mToolBarShowTimer->setInterval(m_iToolBarShowingInterval);
                connect(mToolBarShowTimer, SIGNAL(timeout()), this, SLOT(hideToolBar()));
            }
        }

        else {
            connect(mPlayPauseButton, SIGNAL(clicked(bool)),
                    mToolBarShowTimer, SLOT(start()));

            connect(mStopButton, SIGNAL(clicked(bool)),
                    mToolBarShowTimer, SLOT(start()));

            connect(mMediaControl, SIGNAL(used()),
                    mToolBarShowTimer, SLOT(start()));

            connect(mMuteButton, SIGNAL(clicked(bool)),
                    mToolBarShowTimer, SLOT(start()));
        }


        positionHandles();
    }
}

WBGraphicsMediaItemDelegate::~WBGraphicsMediaItemDelegate()
{
    if (mToolBarShowTimer){
        delete mToolBarShowTimer;
        mToolBarShowTimer = NULL;
    }
}

void WBGraphicsMediaItemDelegate::positionHandles()
{
    WBGraphicsItemDelegate::positionHandles();

    WBGraphicsMediaItem *mediaItem = dynamic_cast<WBGraphicsMediaItem*>(mDelegated);
    if (mediaItem)
    {
        QRectF toolBarRect = mToolBarItem->rect();

        mToolBarItem->setPos(0, mediaItem->boundingRect().height()-mToolBarItem->rect().height());

        toolBarRect.setWidth(mediaItem->boundingRect().width());
        mToolBarItem->show();

        mToolBarItem->setRect(toolBarRect);
    }

    int toolBarButtonsWidth = 0;
    foreach (DelegateButton* button, mToolBarButtons)
        toolBarButtonsWidth += button->boundingRect().width() + mToolBarItem->getElementsPadding();

    QRectF mediaItemRect = mMediaControl->rect();
    mediaItemRect.setWidth(mediaItem->boundingRect().width() - toolBarButtonsWidth);
    mediaItemRect.setHeight(mToolBarItem->boundingRect().height());
    mMediaControl->setRect(mediaItemRect);

    mToolBarItem->positionHandles();
    mMediaControl->positionHandles();

    if (mediaItem)
        mToolBarItem->show();
}

void WBGraphicsMediaItemDelegate::remove(bool canUndo)
{
    if (delegated())
        delegated()->stop();

    WBGraphicsItemDelegate::remove(canUndo);
}


void WBGraphicsMediaItemDelegate::toggleMute()
{
    if (delegated()->isMuted())
        mMuteButton->setFileName(":/images/soundOff.svg");
    else
        mMuteButton->setFileName(":/images/soundOn.svg");
}


WBGraphicsMediaItem* WBGraphicsMediaItemDelegate::delegated()
{
    return dynamic_cast<WBGraphicsMediaItem*>(mDelegated);
}

void WBGraphicsMediaItemDelegate::togglePlayPause()
{
    if (delegated())
        delegated()->togglePlayPause();
}

void WBGraphicsMediaItemDelegate::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // Possible statuses are: UnknownMediaStatus, NoMedia, LoadingMedia, LoadedMedia,
    // StalledMedia, BufferingMedia, BufferedMedia, EndOfMedia, InvalidMedia
	
    if (status == QMediaPlayer::LoadedMedia)
        mMediaControl->totalTimeChanged(delegated()->mediaDuration());

    // At the beginning of the video, play/pause to load and display the first frame
    if ((status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia)
            && delegated()->mediaPosition() == delegated()->initialPos()
            && !delegated()->isStopped()) {
        delegated()->play();
        delegated()->pause();
    }

    // At the end of the video, make sure the progress bar doesn't autohide
    if (status == QMediaPlayer::EndOfMedia)
        showToolBar(false);


    // in most cases, the only necessary action is to update the play/pause state
    updatePlayPauseState();
}

void WBGraphicsMediaItemDelegate::mediaStateChanged(QMediaPlayer::State state)
{
    Q_UNUSED(state);
    // Possible states are StoppedState, PlayingState and PausedState

    // updatePlayPauseState handles this functionality
    updatePlayPauseState();
}


void WBGraphicsMediaItemDelegate::updatePlayPauseState()
{
    if (delegated()->playerState() == QMediaPlayer::PlayingState)
        mPlayPauseButton->setFileName(":/images/pause.svg");
    else
        mPlayPauseButton->setFileName(":/images/play.svg");
}


void WBGraphicsMediaItemDelegate::updateTicker(qint64 time)
{
    mMediaControl->totalTimeChanged(delegated()->mediaDuration());
    mMediaControl->updateTicker(time);
}


void WBGraphicsMediaItemDelegate::totalTimeChanged(qint64 newTotalTime)
{
    mMediaControl->totalTimeChanged(newTotalTime);
}

void WBGraphicsMediaItemDelegate::showHide(bool show)
{
    QVariant showFlag = QVariant(show ? WBItemLayerType::Object : WBItemLayerType::Control);
    showHideRecurs(showFlag, mDelegated);
    mDelegated->update();

    emit showOnDisplayChanged(show);
}
