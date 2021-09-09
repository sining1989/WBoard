#include "WBPodcastController.h"

#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBStringUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBDisplayManager.h"

#include "board/WBBoardController.h"
#include "board/WBBoardView.h"
#include "board/WBBoardPaletteManager.h"

#include "gui/WBMainWindow.h"

#include "web/WBWebController.h"
#include "web/browser/WBWebView.h"

#include "domain/WBGraphicsScene.h"

#include "WBAbstractVideoEncoder.h"

#include "WBPodcastRecordingPalette.h"




#ifdef Q_OS_WIN
    #include "windowsmedia/WBWindowsMediaVideoEncoder.h"
    #include "windowsmedia/WBWaveRecorder.h"
#elif defined(Q_OS_OSX)
    #include "ffmpeg/UBFFmpegVideoEncoder.h"
    #include "ffmpeg/UBMicrophoneInput.h"
#elif defined(Q_OS_LINUX)
    #include "ffmpeg/UBFFmpegVideoEncoder.h"
    #include "ffmpeg/UBMicrophoneInput.h"
#endif

#include "core/memcheck.h"

WBPodcastController* WBPodcastController::sInstance = 0;

unsigned int WBPodcastController::sBackgroundColor = 0x00000000;  // BBGGRRAA


WBPodcastController::WBPodcastController(QObject* pParent)
    : QObject(pParent)
    , mVideoEncoder(0)
    , mIsGrabbing(false)
    , mInitialized(false)
    , mVideoFramesPerSecondAtStart(10)
    , mVideoFrameSizeAtStart(1024, 768)
    , mVideoBitsPerSecondAtStart(1700000)
    , mSourceWidget(0)
    , mSourceScene(0)
    , mScreenGrabingTimerEventID(0)
    , mRecordingProgressTimerEventID(0)
    , mRecordingPalette(0)
    , mRecordingState(Stopped)
    , mApplicationIsClosing(false)
    , mRecordingTimestampOffset(0)
    , mDefaultAudioInputDeviceAction(0)
    , mNoAudioInputDeviceAction(0)
    , mSmallVideoSizeAction(0)
    , mMediumVideoSizeAction(0)
    , mFullVideoSizeAction(0)
{
    connect(WBApplication::applicationController, SIGNAL(mainModeChanged(WBApplicationController::MainMode)),
            this, SLOT(applicationMainModeChanged(WBApplicationController::MainMode)));

    connect(WBApplication::applicationController, SIGNAL(desktopMode(bool)),
            this, SLOT(applicationDesktopMode(bool)));

    connect(WBApplication::webController, SIGNAL(activeWebPageChanged(WBWebView*)),
            this, SLOT(webActiveWebPageChanged(WBWebView*)));

    connect(WBApplication::app(), SIGNAL(lastWindowClosed()),
            this, SLOT(applicationAboutToQuit()));

}


WBPodcastController::~WBPodcastController()
{
    // NOOP
}


void WBPodcastController::applicationAboutToQuit()
{
    mApplicationIsClosing = true;

    if(mRecordingState == Recording || mRecordingState == Paused)
    {
        stop();
    }
}


void WBPodcastController::groupActionTriggered(QAction* action)
{
    Q_UNUSED(action);
    updateActionState();
}


void WBPodcastController::actionToggled(bool checked)
{
    Q_UNUSED(checked);
    updateActionState();
}


void WBPodcastController::updateActionState()
{
    if (mSmallVideoSizeAction && mSmallVideoSizeAction->isChecked())
        WBSettings::settings()->podcastVideoSize->set("Small");
    else if (mFullVideoSizeAction && mFullVideoSizeAction->isChecked())
        WBSettings::settings()->podcastVideoSize->set("Full");
    else
        WBSettings::settings()->podcastVideoSize->reset();

    WBSettings::settings()->podcastAudioRecordingDevice->reset();

    if (mDefaultAudioInputDeviceAction && mDefaultAudioInputDeviceAction->isChecked())
         WBSettings::settings()->podcastAudioRecordingDevice->set("Default");
    else if (mNoAudioInputDeviceAction && mNoAudioInputDeviceAction->isChecked())
         WBSettings::settings()->podcastAudioRecordingDevice->set("None");
    else
    {
        foreach(QAction* action, mAudioInputDevicesActions)
        {
            if (action->isChecked())
            {
                WBSettings::settings()->podcastAudioRecordingDevice->set(action->text());
                break;
            }
        }
    }

}


void WBPodcastController::setSourceWidget(QWidget* pWidget)
{
    if (mSourceWidget != pWidget)
    {
        if (mSourceWidget == qApp->desktop())
        {
            killTimer(mScreenGrabingTimerEventID);
            mScreenGrabingTimerEventID = 0;
        }
        else if (mSourceWidget)
        {
            mSourceWidget->removeEventFilter(this);
        }

        mSourceWidget = pWidget;
        mInitialized = false;
        mViewToVideoTransform.reset();
        mLatestCapture.fill(sBackgroundColor);

        if (mSourceWidget)
        {
            QSizeF sourceWidgetSize(mSourceWidget->size());

            if (mSourceWidget == qApp->desktop())
                sourceWidgetSize = qApp->desktop()->availableGeometry(WBApplication::applicationController->displayManager()->controleScreenIndex()).size();

            QSizeF videoFrameSize(mVideoFrameSizeAtStart);

            qreal scaleHorizontal = videoFrameSize.width() / sourceWidgetSize.width();
            qreal scaleVertical = videoFrameSize.height() / sourceWidgetSize.height();

            qreal scale = qMin(scaleHorizontal, scaleVertical);

            mViewToVideoTransform.scale(scale, scale);

            WBBoardView *bv = qobject_cast<WBBoardView *>(mSourceWidget);

            if (bv)
            {
                connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), this, SLOT(activeSceneChanged()));
                connect(WBApplication::boardController, SIGNAL(backgroundChanged()), this, SLOT(sceneBackgroundChanged()));
                connect(WBApplication::boardController, SIGNAL(controlViewportChanged()), this, SLOT(activeSceneChanged()));

                activeSceneChanged();
            }
            else
            {
                QSizeF scaledWidgetSize = sourceWidgetSize * scale;

                int offsetX = (videoFrameSize.width() - scaledWidgetSize.width()) / 2;
                int offsetY = (videoFrameSize.height() - scaledWidgetSize.height()) / 2;

                mViewToVideoTransform.translate(offsetX / scale, offsetY / scale);

                disconnect(WBApplication::boardController, SIGNAL(activeSceneChanged()), this, SLOT(activeSceneChanged()));
                disconnect(WBApplication::boardController, SIGNAL(backgroundChanged()), this, SLOT(sceneBackgroundChanged()));
                disconnect(WBApplication::boardController, SIGNAL(controlViewportChanged()), this, SLOT(activeSceneChanged()));

                mSourceScene = 0;

                startNextChapter();

                if(mSourceWidget == qApp->desktop())
                {
                    mScreenGrabingTimerEventID  = startTimer(1000 / mVideoFramesPerSecondAtStart);
                }
                else
                {
                    mSourceWidget->installEventFilter(this);
                }
            }
        }
    }
}


WBPodcastController* WBPodcastController::instance()
{
    if(!sInstance)
        sInstance = new WBPodcastController(WBApplication::staticMemoryCleaner);

    return sInstance;
}


void WBPodcastController::start()
{
    if (mRecordingState == Stopped)
    {
        mInitialized = false;

        QSize recommendedSize(1024, 768);

        int fullBitRate = WBSettings::settings()->podcastWindowsMediaBitsPerSecond->get().toInt();

        if (mSmallVideoSizeAction && mSmallVideoSizeAction->isChecked())
        {
            recommendedSize = QSize(640, 480);
            mVideoBitsPerSecondAtStart = fullBitRate / 4;
        }
        else if (mMediumVideoSizeAction && mMediumVideoSizeAction->isChecked())
        {
            recommendedSize = QSize(1024, 768);
            mVideoBitsPerSecondAtStart = fullBitRate / 2;
        }
        else if (mFullVideoSizeAction && mFullVideoSizeAction->isChecked())
        {
            recommendedSize = WBApplication::boardController->controlView()->size();
            mVideoBitsPerSecondAtStart = fullBitRate;
        }

        QSize scaledboardSize = WBApplication::boardController->controlView()->size();
        scaledboardSize.scale(recommendedSize, Qt::KeepAspectRatio);

        // Video width/height should be a multiple of 4

        int width = scaledboardSize.width();
        int height = scaledboardSize.height();

        if (width % 4 != 0)
                width = ((width / 4) * 4);

        if (height % 4 != 0)
                height = ((height / 4) * 4);

        mVideoFrameSizeAtStart = QSize(width, height);

        applicationMainModeChanged(WBApplication::applicationController->displayMode());

#ifdef Q_OS_WIN
		mVideoEncoder = new WBWindowsMediaVideoEncoder(this);  //deleted on stop
#elif defined(Q_OS_OSX)
        mVideoEncoder = new UBFFmpegVideoEncoder(this);
#elif defined(Q_OS_LINUX)
        mVideoEncoder = new UBFFmpegVideoEncoder(this);
#endif

        if (mVideoEncoder)
        {
            connect(mVideoEncoder, SIGNAL(encodingStatus(const QString&)), this, SLOT(encodingStatus(const QString&)));
            connect(mVideoEncoder, SIGNAL(encodingFinished(bool)), this, SLOT(encodingFinished(bool)));

            if(mRecordingPalette)
            {
                connect(mVideoEncoder, SIGNAL(audioLevelChanged(quint8))
                        , mRecordingPalette, SLOT(audioLevelChanged(quint8)));
            }

            mVideoEncoder->setRecordAudio(!mNoAudioInputDeviceAction->isChecked());

            QString recordingDevice = "";

            if (!mNoAudioInputDeviceAction->isChecked() && !mDefaultAudioInputDeviceAction->isChecked())
            {
                foreach(QAction* audioDevice, mAudioInputDevicesActions)
                {
                    if (audioDevice->isChecked())
                    {
                        recordingDevice = audioDevice->text();
                        break;
                    }
                }
            }

            mVideoEncoder->setAudioRecordingDevice(recordingDevice);

            mVideoEncoder->setFramesPerSecond(mVideoFramesPerSecondAtStart);
            mVideoEncoder->setVideoSize(mVideoFrameSizeAtStart);
            mVideoEncoder->setVideoBitsPerSecond(mVideoBitsPerSecondAtStart);

            mPartNumber = 0;

            mPodcastRecordingPath = WBSettings::settings()->userPodcastRecordingDirectory();

            qDebug() << "mPodcastRecordingPath: " << mPodcastRecordingPath;

            QString videoFileName;

			//zhusizhi
            //if (mIntranetPublicationAction && mIntranetPublicationAction->isChecked())
            //{
                videoFileName = mPodcastRecordingPath + "/" + "Podcast-"
                        + QDateTime::currentDateTime().toString("yyyyMMddhhmmss")
                        + "-" + WBPlatformUtils::computerName() + "." + mVideoEncoder->videoFileExtension();
            //}
            //else
            //{
            //    videoFileName = mPodcastRecordingPath + "/" + tr("WBoard Cast") + "." + mVideoEncoder->videoFileExtension();
            //}

            videoFileName = WBFileSystemUtils::nextAvailableFileName(videoFileName, " ");

            mVideoEncoder->setVideoFileName(videoFileName);

            mLatestCapture = QImage(mVideoFrameSizeAtStart, QImage::Format_RGB32); //0xffRRGGBB

            mRecordStartTime = QTime::currentTime();

            mRecordingProgressTimerEventID = startTimer(100);

            if(mVideoEncoder->start())
            {
                applicationMainModeChanged(WBApplication::applicationController->displayMode());

                setRecordingState(Recording);

                if (mSourceScene)
                {
                    processScenePaintEvent();
                }
                else if (mSourceWidget)
                {
                    processWidgetPaintEvent();
                }
            }
            else
            {
                WBApplication::showMessage(tr("Failed to start encoder ..."), false);
            }
        }
        else
        {
            WBApplication::showMessage(tr("No Podcast encoder available ..."), false);
        }
    }
}

void WBPodcastController::pause()
{
    if (mVideoEncoder && mRecordingState == Recording && mVideoEncoder->canPause())
    {
        sendLatestPixmapToEncoder();

        mTimeAtPaused = QTime::currentTime();

        if (mVideoEncoder->pause())
        {
            setRecordingState(Paused);
        }
    }
}


void WBPodcastController::unpause()
{
    if (mVideoEncoder && mRecordingState == Paused && mVideoEncoder->canPause())
    {
        if (mVideoEncoder->unpause())
        {
             mRecordingTimestampOffset += mTimeAtPaused.msecsTo(QTime::currentTime());
             sendLatestPixmapToEncoder();

             setRecordingState(Recording);
        }
    }
}


void WBPodcastController::stop()
{
    if ((mRecordingState == Recording || mRecordingState == Paused) && mVideoEncoder)
    {
        if (mScreenGrabingTimerEventID != 0)
        {
            killTimer(mScreenGrabingTimerEventID);
            mScreenGrabingTimerEventID = 0;
        }

        if (mRecordingProgressTimerEventID != 0)
            killTimer(mRecordingProgressTimerEventID);

        sendLatestPixmapToEncoder();

        setRecordingState(Stopping);

        mVideoEncoder->stop();
    }
}


bool WBPodcastController::eventFilter(QObject *obj, QEvent *event)
{
    if (mRecordingState == Recording && !mIsGrabbing && event->type() == QEvent::Paint)
    {
        QPaintEvent *paintEvent = static_cast<QPaintEvent*>(event);

        mWidgetRepaintRectQueue.enqueue(paintEvent->rect());

        if (mWidgetRepaintRectQueue.length() == 1)
            QTimer::singleShot(1000.0 / mVideoFramesPerSecondAtStart, this, SLOT(processWidgetPaintEvent()));
     }

    return QObject::eventFilter(obj, event);
}


void WBPodcastController::processWidgetPaintEvent()
{
    if(mRecordingState != Recording)
        return;

    QRect repaintRect;

    if (!mInitialized)
    {
        mWidgetRepaintRectQueue.clear();
        repaintRect = mSourceWidget->geometry();

        mLatestCapture.fill(sBackgroundColor);

        mInitialized = true;
    }
    else
    {
        while(mWidgetRepaintRectQueue.size() > 0)
        {
            repaintRect = repaintRect.united(mWidgetRepaintRectQueue.dequeue());
        }
    }

    if (!repaintRect.isNull())
    {
        mIsGrabbing = true;

        QPainter p(&mLatestCapture);
        p.setTransform(mViewToVideoTransform);
        p.setRenderHints(QPainter::Antialiasing);
        p.setRenderHints(QPainter::SmoothPixmapTransform);

        mSourceWidget->render(&p, repaintRect.topLeft(), QRegion(repaintRect), QWidget::DrawChildren);

        mIsGrabbing = false;

        sendLatestPixmapToEncoder();
    }
}


void WBPodcastController::activeSceneChanged()
{
    if (mSourceScene)
    {
        disconnect(mSourceScene, SIGNAL(changed(const QList<QRectF>&)),
                this, SLOT(sceneChanged(const QList<QRectF> &)));
    }

    mSourceScene = WBApplication::boardController->activeScene();

    connect(mSourceScene, SIGNAL(changed(const QList<QRectF>&)),
        this, SLOT(sceneChanged(const QList<QRectF> &)));

    mInitialized = false;

    startNextChapter();

    WBBoardView *bv = qobject_cast<WBBoardView*>(mSourceWidget);
    if (bv)
    {
        QRectF viewportRect = bv->mapToScene(bv->geometry()).boundingRect();
        mSceneRepaintRectQueue.enqueue(viewportRect);
    }

    processScenePaintEvent();
}

void WBPodcastController::sceneBackgroundChanged()
{
    WBBoardView *bv = qobject_cast<WBBoardView*>(mSourceWidget);

    if (bv)
    {
        mInitialized = false;
    }

    processScenePaintEvent();
}


long WBPodcastController::elapsedRecordingMs()
{
    QTime now = QTime::currentTime();
    long msFromStart = mRecordStartTime.msecsTo(now);

    return msFromStart - mRecordingTimestampOffset;
}


void WBPodcastController::startNextChapter()
{
    if (mVideoEncoder)
    {
        //punch chapter in
        ++mPartNumber;
        mVideoEncoder->newChapter(tr("Part %1").arg(mPartNumber), elapsedRecordingMs());
    }
}


void WBPodcastController::sceneChanged(const QList<QRectF> & region)
{
    if(mRecordingState != Recording)
        return;

    bool shouldRepaint = (mSceneRepaintRectQueue.length() == 0);

    WBBoardView *bv = qobject_cast<WBBoardView *>(mSourceWidget);
    if (bv)
    {
        QRectF viewportRect = bv->mapToScene(QRect(0, 0, bv->width(), bv->height())).boundingRect();
        foreach(const QRectF rect, region)
        {
            QRectF maxRect = rect.intersected(viewportRect);
            mSceneRepaintRectQueue.enqueue(maxRect);
        }

        if (shouldRepaint)
            QTimer::singleShot(1000.0 / mVideoFramesPerSecondAtStart, this, SLOT(processScenePaintEvent()));

    }
}


void WBPodcastController::processScenePaintEvent()
{
    if(mRecordingState != Recording)
        return;

    WBBoardView *bv = qobject_cast<WBBoardView *>(mSourceWidget);

    if(!bv)
        return;

    QRectF repaintRect;

    if (!mInitialized)
    {
        mSceneRepaintRectQueue.clear();
        repaintRect = bv->mapToScene(QRect(0, 0, bv->width(), bv->height())).boundingRect();

        if (bv->scene()->isDarkBackground())
                mLatestCapture.fill(Qt::black);
        else
                mLatestCapture.fill(Qt::white);

        mLatestCapture.fill(Qt::white);

        mInitialized = true;
    }
    else
    {
        while(mSceneRepaintRectQueue.size() > 0)
        {
            repaintRect = repaintRect.united(mSceneRepaintRectQueue.dequeue());
        }
    }

    if (!repaintRect.isNull())
    {
        WBGraphicsScene *scene = bv->scene();

        QPainter p(&mLatestCapture);

        p.setTransform(mViewToVideoTransform);
        p.setTransform(bv->viewportTransform(), true);

        p.setRenderHints(QPainter::Antialiasing);
        p.setRenderHints(QPainter::SmoothPixmapTransform);

        repaintRect.adjust(-1, -1, 1, 1);

        p.setClipRect(repaintRect);

        if (scene->isDarkBackground())
            p.fillRect(repaintRect, Qt::black);
        else
            p.fillRect(repaintRect, Qt::white);

        scene->setRenderingContext(WBGraphicsScene::Podcast);

        scene->render(&p, repaintRect, repaintRect);

        scene->setRenderingContext(WBGraphicsScene::Screen);

        sendLatestPixmapToEncoder();
    }
}


void WBPodcastController::applicationMainModeChanged(WBApplicationController::MainMode pMode)
{
    if (pMode == WBApplicationController::Internet)
    {
        setSourceWidget(WBApplication::webController->controlView());
    }
    else
    {
        setSourceWidget(WBApplication::boardController->controlView());
    }
}


void WBPodcastController::applicationDesktopMode(bool displayed)
{
    Q_UNUSED(displayed);

    if (displayed)
    {
        setSourceWidget(qApp->desktop());
    }
    else
    {
        applicationMainModeChanged(WBApplication::applicationController->displayMode());
    }
}


void WBPodcastController::webActiveWebPageChanged(WBWebView* pWebView)
{
    if(WBApplication::applicationController->displayMode() == WBApplicationController::Internet)
    {
         setSourceWidget(pWebView);
    }
}


void WBPodcastController::encodingStatus(const QString& pStatus)
{
    WBApplication::showMessage(pStatus, true);
}


void WBPodcastController::encodingFinished(bool ok)
{
    if (mVideoEncoder)
    {
        if (ok)
        {
            if (!mApplicationIsClosing)
            {
                QString location;

                if (mPodcastRecordingPath == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
                    location = tr("on your desktop ...");
                else
                {
                    QDir dir(mPodcastRecordingPath);
                    location = tr("in folder %1").arg(mPodcastRecordingPath);
                }

                WBApplication::showMessage(tr("Podcast created %1").arg(location), false);

            }
        }
        else
        {
            qWarning() << mVideoEncoder->lastErrorMessage();

            WBApplication::showMessage(tr("Podcast recording error (%1)").arg(mVideoEncoder->lastErrorMessage()), false);
        }

        mVideoEncoder->deleteLater();

        setRecordingState(Stopped);
    }
}


void WBPodcastController::sendLatestPixmapToEncoder()
{
    if (mVideoEncoder)
        mVideoEncoder->newPixmap(mLatestCapture, elapsedRecordingMs());
}

void WBPodcastController::timerEvent(QTimerEvent *event)
{
    if (mRecordingState == Recording
            && event->timerId() == mScreenGrabingTimerEventID
            && mSourceWidget == qApp->desktop())
    {
        QDesktopWidget * dtop = QApplication::desktop();
        QRect dtopRect = dtop->screenGeometry(WBApplication::controlScreenIndex());
        QScreen * screen = WBApplication::controlScreen();

        QPixmap desktop = screen->grabWindow(dtop->effectiveWinId(),
                                             dtopRect.x(), dtopRect.y(), dtopRect.width(), dtopRect.height());

        {
            QPainter p(&mLatestCapture);

            if (!mInitialized)
            {
                mLatestCapture.fill(sBackgroundColor);
                mInitialized = true;
            }

            QRectF targetRect = mViewToVideoTransform.mapRect(QRectF(0, 0, desktop.width(), desktop.height()));

            p.setRenderHints(QPainter::Antialiasing);
            p.setRenderHints(QPainter::SmoothPixmapTransform);
            p.drawPixmap(targetRect.left(), targetRect.top(), desktop.scaled(targetRect.width(), targetRect.height(),  Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        sendLatestPixmapToEncoder();
    }

    if (mRecordingProgressTimerEventID == event->timerId() && mRecordingState == Recording)
    {
        emit recordingProgressChanged(elapsedRecordingMs());
    }
}


QStringList WBPodcastController::audioRecordingDevices()
{
    QStringList devices;

#ifdef Q_OS_WIN
    //devices = WBWaveRecorder::waveInDevices();
#elif defined(Q_OS_OSX)
    devices = UBMicrophoneInput::availableDevicesNames();
#elif defined(Q_OS_LINUX)
    devices = UBMicrophoneInput::availableDevicesNames();
#endif

    return devices;
}


void WBPodcastController::recordToggled(bool record)
{
    if ((mRecordingState == Stopped) && record)
        start();
    else
        stop();
}

void WBPodcastController::pauseToggled(bool paused)
{
    if ((mRecordingState == Recording) && paused)
        pause();
    else
        unpause();
}


void WBPodcastController::toggleRecordingPalette(bool visible)
{
    if(!mRecordingPalette)
    {
        mRecordingPalette = new WBPodcastRecordingPalette(WBApplication::mainWindow);

        mRecordingPalette->adjustSizeAndPosition();
        mRecordingPalette->setCustomPosition(true);

        int left = WBApplication::boardController->controlView()->width() * 0.75
                   - mRecordingPalette->width() / 2;

        int top = WBApplication::boardController->controlView()->height()
                   - mRecordingPalette->height() - WBSettings::boardMargin;

        QPoint controlViewPoint(left, top);
        QPoint mainWindowsPoint = WBApplication::boardController->controlView()->mapTo(WBApplication::mainWindow, controlViewPoint);

        mRecordingPalette->move(mainWindowsPoint);

        connect(WBApplication::mainWindow->actionPodcastRecord, SIGNAL(triggered(bool))
             , this, SLOT(recordToggled(bool)));

        connect(WBApplication::mainWindow->actionPodcastPause, SIGNAL(toggled(bool))
             , this, SLOT(pauseToggled(bool)));

        connect(this, SIGNAL(recordingStateChanged(WBPodcastController::RecordingState))
                , mRecordingPalette, SLOT(recordingStateChanged(WBPodcastController::RecordingState)));
        connect(this, SIGNAL(recordingProgressChanged(qint64))
                , mRecordingPalette, SLOT(recordingProgressChanged(qint64)));
    }

    mRecordingPalette->setVisible(visible);
}


void WBPodcastController::setRecordingState(RecordingState pRecordingState)
{
    if(mRecordingState != pRecordingState)
    {
        mRecordingState = pRecordingState;
        emit recordingStateChanged(mRecordingState);
    }
}


QList<QAction*> WBPodcastController::audioRecordingDevicesActions()
{
    if (mAudioInputDevicesActions.length() == 0)
    {
        QString settingsDevice = WBSettings::settings()->podcastAudioRecordingDevice->get().toString();

        mDefaultAudioInputDeviceAction = new QAction(tr("Default Audio Input"), this);
        QAction *checkedAction = mDefaultAudioInputDeviceAction;

        mNoAudioInputDeviceAction = new QAction(tr("No Audio Recording"), this);

        if (settingsDevice == "None")
            checkedAction = mNoAudioInputDeviceAction;

        mAudioInputDevicesActions << mNoAudioInputDeviceAction;
        mAudioInputDevicesActions << mDefaultAudioInputDeviceAction;

        foreach(QString audioDevice, audioRecordingDevices())
        {
            QAction* act = new QAction(audioDevice, this);
            act->setCheckable(true);
            mAudioInputDevicesActions << act;
            if (settingsDevice == audioDevice)
                checkedAction = act;
        }

        QActionGroup* audioInputActionGroup = new QActionGroup(this);
        audioInputActionGroup->setExclusive(true);

        foreach(QAction* action, mAudioInputDevicesActions)
        {
            audioInputActionGroup->addAction(action);
            action->setCheckable(true);
        }
        checkedAction->setChecked(true);

        connect(audioInputActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(groupActionTriggered(QAction*)));
    }

    return mAudioInputDevicesActions;

}


QList<QAction*> WBPodcastController::videoSizeActions()
{
    if (mVideoSizesActions.length() == 0)
    {
        mSmallVideoSizeAction = new QAction(tr("Small"), this);
        mMediumVideoSizeAction = new QAction(tr("Medium"), this);
        mFullVideoSizeAction = new QAction(tr("Full"), this);

        mVideoSizesActions << mSmallVideoSizeAction;
        mVideoSizesActions << mMediumVideoSizeAction;
        mVideoSizesActions << mFullVideoSizeAction;

        QActionGroup* videoSizeActionGroup = new QActionGroup(this);
        videoSizeActionGroup->setExclusive(true);

        foreach(QAction* videoSizeAction, mVideoSizesActions)
        {
            videoSizeAction->setCheckable(true);
            videoSizeActionGroup->addAction(videoSizeAction);
        }

        QString videoSize = WBSettings::settings()->podcastVideoSize->get().toString();

        if (videoSize == "Small")
            mSmallVideoSizeAction->setChecked(true);
        else if (videoSize == "Full")
            mFullVideoSizeAction->setChecked(true);
        else
            mMediumVideoSizeAction->setChecked(true);

        connect(videoSizeActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(groupActionTriggered(QAction*)));
    }

    return mVideoSizesActions;
}


QList<QAction*> WBPodcastController::podcastPublicationActions()
{
    if (mPodcastPublicationActions.length() == 0)
    {
        foreach(QAction* publicationAction, mPodcastPublicationActions)
        {
            connect(publicationAction, SIGNAL(toggled(bool)), this, SLOT(actionToggled(bool)));
        }
    }

    return mPodcastPublicationActions;
}



