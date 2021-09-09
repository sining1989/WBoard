#include "core/WBApplication.h"
#include "globals/WBGlobals.h"
#include "WBMediaWidget.h"

#include "core/memcheck.h"


WBMediaWidget::WBMediaWidget(eMediaType type, QWidget *parent, const char *name):WBActionableWidget(parent, name)
  , mpMediaObject(NULL)
  , mpVideoWidget(NULL)
  , mpAudioOutput(NULL)
  , mpLayout(NULL)
  , mpSeekerLayout(NULL)
  , mpPlayStopButton(NULL)
  , mpPauseButton(NULL)
  , mpSlider(NULL)
  , mAutoUpdate(false)
  , mGeneratingThumbnail(false)
  , mBorder(5)
  , mpMediaContainer(NULL)
  , mMediaLayout(NULL)
  , mpCover(NULL)
{
    SET_STYLE_SHEET();

    addAction(eAction_Close);
    mType = type;
    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    mpPlayStopButton = new WBMediaButton(this);
    mpPlayStopButton->setPixmap(QPixmap(":images/play.svg"));
    mpPauseButton = new WBMediaButton(this);
    mpPauseButton->setPixmap(QPixmap(":images/pause.svg"));
    mpPauseButton->setEnabled(false);
    mpSlider = new QSlider(this);
    mpSlider->setOrientation(Qt::Horizontal);
    mpSlider->setMinimum(0);
    mpSlider->setMaximum(0);

    mpSeekerLayout = new QHBoxLayout();
    mpSeekerLayout->addWidget(mpPlayStopButton, 0);
    mpSeekerLayout->addWidget(mpPauseButton, 0);
    mpSeekerLayout->addWidget(mpSlider, 1);
    mpSeekerLayout->setContentsMargins(0, 0, 0, 0);

    connect(mpPlayStopButton, SIGNAL(clicked()), this, SLOT(onPlayStopClicked()));
    connect(mpPauseButton, SIGNAL(clicked()), this, SLOT(onPauseClicked()));
    connect(mpSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
}

/**
  * \brief Destructor
  */
WBMediaWidget::~WBMediaWidget()
{
    unsetActionsParent();
    DELETEPTR(mpMediaObject);
    DELETEPTR(mpSlider);
    DELETEPTR(mpPauseButton);
    DELETEPTR(mpPlayStopButton);
    DELETEPTR(mpAudioOutput);
    DELETEPTR(mpVideoWidget);
    DELETEPTR(mpCover);
    DELETEPTR(mpMediaContainer);
    DELETEPTR(mpSeekerLayout);
    DELETEPTR(mpLayout);
}

/**
  * \brief Set the media file
  * @param filePath as the media file path
  */
void WBMediaWidget::setFile(const QString &filePath)
{
    Q_ASSERT("" != filePath);
    mFilePath = filePath;
    mpMediaObject = new QMediaPlayer(this);
    //mpMediaObject->setTickInterval(TICK_INTERVAL);
    connect(mpMediaObject, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(onStateChanged(QMediaPlayer::State)));
    connect(mpMediaObject, SIGNAL(totalTimeChanged(qint64)), this, SLOT(onTotalTimeChanged(qint64)));
    connect(mpMediaObject, SIGNAL(tick(qint64)), this, SLOT(onTick(qint64)));
    mpMediaObject->setMedia(QUrl::fromLocalFile(filePath));
    createMediaPlayer();
}

/**
  * \brief Get the media type
  * @returns the media type
  */
eMediaType WBMediaWidget::mediaType()
{
    return mType;
}

void WBMediaWidget::showEvent(QShowEvent* event)
{
	if(mType == eMediaType_Audio){
		return;
	}else{
		if(!mpVideoWidget){
			mpVideoWidget = new QVideoWidget(this);
			mMediaLayout->addStretch(1);
			mMediaLayout->addWidget(mpVideoWidget);
			mMediaLayout->addStretch(1);
			mpMediaObject->setVideoOutput(mpVideoWidget);
			adaptSizeToVideo();
			mpMediaObject->play();
			mpMediaObject->stop();
		}
		QWidget::showEvent(event);
	}
}

void WBMediaWidget::hideEvent(QHideEvent* event)
{
    if(mpMediaObject->state() == QMediaPlayer::PlayingState)
        mpMediaObject->stop();
    WBActionableWidget::hideEvent(event);
}

/**
  * \brief Create the media player
  */
void WBMediaWidget::createMediaPlayer()
{
    mpMediaContainer = new QWidget();
    mpMediaContainer->setObjectName("UBMediaVideoContainer");
    mMediaLayout = new QHBoxLayout();
    mpMediaContainer->setLayout(mMediaLayout);

    if(eMediaType_Video == mType){
        mMediaLayout->setContentsMargins(10, 10, 10, 10);
        if(isVisible()){
            mpVideoWidget = new QVideoWidget(this);
            mMediaLayout->addStretch(1);
            mMediaLayout->addWidget(mpVideoWidget);
            mMediaLayout->addStretch(1);
			mpMediaObject->setVideoOutput(mpVideoWidget);
            adaptSizeToVideo();
        }
        mpAudioOutput = new QMediaPlayer(this);
        //Phonon::createPath(mpMediaObject, mpAudioOutput);
    }else if(eMediaType_Audio == mType){
        mMediaLayout->setContentsMargins(10, 10, 10, 10);
        mpCover = new QLabel(mpMediaContainer);
        //mpMediaContainer->setStyleSheet(QString("background: none;"));
        setAudioCover(":images/libpalette/soundIcon.svg");
        mpCover->setScaledContents(true);
        mMediaLayout->addStretch(1);
        mMediaLayout->addWidget(mpCover);
        mMediaLayout->addStretch(1);
        mpAudioOutput = new QMediaPlayer(this);
        //Phonon::createPath(mpMediaObject, mpAudioOutput);
    }
    mpLayout->addWidget(mpMediaContainer, 1);
    mpLayout->addLayout(mpSeekerLayout, 0);
    setActionsParent(mpMediaContainer);
}

/**
  * \brief Adapt the widget size to the video in order to keep the good aspect ratio
  */
void WBMediaWidget::adaptSizeToVideo()
{
    if(NULL != mpMediaContainer){
        int origW = mpMediaContainer->width();
        int origH = mpMediaContainer->height();
        int newW = width();
        float scaleFactor = (float)origW/(float)newW;
        int newH = origH/scaleFactor;
        resize(newW, height() + newH);
    }
}

/**
  * \brief Handle the media state change notification
  * @param newState as the new state
  * @param oldState as the old state
  */
void WBMediaWidget::onStateChanged(QMediaPlayer::State state)
{
    if(!mGeneratingThumbnail){
        //if(QMediaPlayer::LoadingMedia == oldState && QMediaPlayer::StoppedState == newState){
        //    if(eMediaType_Video == mType){
        //        // We do that here to generate the thumbnail of the video
        //        mGeneratingThumbnail = true;
        //        mpMediaObject->play();
        //        mpMediaObject->pause();
        //        mGeneratingThumbnail = false;
        //    }
        //}else if(QMediaPlayer::PlayingState == oldState && QMediaPlayer::PausedState == newState){
        //    mpPlayStopButton->setPixmap(QPixmap(":images/play.svg"));
        //    mpPauseButton->setEnabled(false);
        //}else if((QMediaPlayer::PausedState == oldState && QMediaPlayer::PlayingState == newState) ||
        //         (QMediaPlayer::StoppedState == oldState && QMediaPlayer::PlayingState == newState)){
        //    mpPlayStopButton->setPixmap(QPixmap(":images/stop.svg"));
        //    mpPauseButton->setEnabled(true);
        //}else if(QMediaPlayer::PlayingState == oldState && QMediaPlayer::StoppedState == newState){
        //    mpPlayStopButton->setPixmap(QPixmap(":images/play.svg"));
        //    mpPauseButton->setEnabled(false);
        //    mpSlider->setValue(0);
        //}

    }
    //    if(mType == eMediaType_Video)
    //        updateView(newState);
}

/**
  * \brief Handles the total time change notification
  * @param total as the new total time
  */
void WBMediaWidget::onTotalTimeChanged(qint64 total)
{
    mpSlider->setMaximum(total);
}

/**
  * \brief Handles the tick notification
  * @param currentTime as the current time
  */
void WBMediaWidget::onTick(qint64 currentTime)
{
    mAutoUpdate = true;
    mpSlider->setValue((int)currentTime);
    mAutoUpdate = false;
}

/**
  * \brief Handles the seeker value change notification
  * @param value as the new seeker value
  */
void WBMediaWidget::onSliderChanged(int value)
{
    if(!mAutoUpdate){
        mpMediaObject->setVolume(value);
    }
}

/**
  * \brief Toggle Play-Stop
  */
void WBMediaWidget::onPlayStopClicked()
{
    switch(mpMediaObject->state()){
    case QMediaPlayer::PlayingState:
        mpMediaObject->stop();
        break;

    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        mpMediaObject->play();
        break;
    default:
        break;
    }
}

/**
  * \brief Pause the media
  */
void WBMediaWidget::onPauseClicked()
{
    mpMediaObject->pause();
}

/**
  * Get the border
  * @returns the actual border
  */
int WBMediaWidget::border()
{
    return mBorder;
}

/**
  * \brief Handles the resize event
  * @param ev as the resize event
  */
void WBMediaWidget::resizeEvent(QResizeEvent* ev)
{
    Q_UNUSED(ev);
}

/**
  * \brief Set the audio cover
  * @param coverPath as the cover image file path
  */
void WBMediaWidget::setAudioCover(const QString &coverPath)
{
    if(NULL != mpCover){
        mpCover->setPixmap(QPixmap(coverPath));
    }
}

// -----------------------------------------------------------------------------------------------------------
/**
  * \brief Constructor
  * @param parent as the parent widget
  * @param name as the object name
  */
WBMediaButton::WBMediaButton(QWidget *parent, const char *name):QLabel(parent)
  , mPressed(false)
{
    setObjectName(name);
    resize(WBMEDIABUTTON_SIZE, WBMEDIABUTTON_SIZE);
    setStyleSheet(QString("padding:0px 0px 0px 0px; margin:0px 0px 0px 0px;"));
}

/**
  * \brief Destructor
  */
WBMediaButton::~WBMediaButton()
{

}

/**
  * \brief Handles the mouse press notification
  * @param ev as the mouse press event
  */
void WBMediaButton::mousePressEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);
    mPressed = true;
}

/**
  * \brief Handles the mouse release notification
  * @param ev as the mouse release event
  */
void WBMediaButton::mouseReleaseEvent(QMouseEvent* ev)
{
    Q_UNUSED(ev);
    if(mPressed){
        mPressed = false;
        emit clicked();
    }
}
