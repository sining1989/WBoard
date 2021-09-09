#include "WBPodcastRecordingPalette.h"

#include "WBPodcastController.h"

#include "core/WBApplication.h"

#include "gui/WBResources.h"

#include "core/WBSettings.h"

#include "gui/WBMainWindow.h"

#include "core/memcheck.h"

WBPodcastRecordingPalette::WBPodcastRecordingPalette(QWidget *parent)
     : WBActionPalette(Qt::Horizontal, parent)
{
    addAction(WBApplication::mainWindow->actionPodcastRecord);

    mTimerLabel = new QLabel(this);
    mTimerLabel->setStyleSheet(QString("QLabel {color: #1296DB; font-size: 14px; font-weight: bold; font-family: Arial; background-color: transparent; border: none}"));
    recordingProgressChanged(0);

    layout()->addWidget(mTimerLabel);

    mLevelMeter = new WBVuMeter(this);
    mLevelMeter->setMinimumSize(6, 32);

    layout()->addWidget(mLevelMeter);

    addAction(WBApplication::mainWindow->actionPodcastConfig);

    foreach(QWidget* menuWidget,  WBApplication::mainWindow->actionPodcastConfig->associatedWidgets())
    {
        QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

        tb->setIconSize(QSize(16, 16));

        if (tb && !tb->menu())
        {
            tb->setObjectName("ubButtonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu* menu = new QMenu(this);

            foreach(QAction* audioInputAction, WBPodcastController::instance()->audioRecordingDevicesActions())
            {
                menu->addAction(audioInputAction);
            }

            menu->addSeparator();

            foreach(QAction* videoSizeAction, WBPodcastController::instance()->videoSizeActions())
            {
                menu->addAction(videoSizeAction);
            }

            menu->addSeparator();

            QList<QAction*> podcastPublication = WBPodcastController::instance()->podcastPublicationActions();

            foreach(QAction* publicationAction, podcastPublication)
            {
                menu->addAction(publicationAction);
            }

            tb->setMenu(menu);
        }
    }
}


WBPodcastRecordingPalette::~WBPodcastRecordingPalette()
{
    // NOOP
}


void WBPodcastRecordingPalette::recordingStateChanged(WBPodcastController::RecordingState state)
{
    if (state == WBPodcastController::Recording)
    {
        WBApplication::mainWindow->actionPodcastRecord->setChecked(true);
        WBApplication::mainWindow->actionPodcastRecord->setEnabled(true);

        WBApplication::mainWindow->actionPodcastPause->setChecked(false);
        WBApplication::mainWindow->actionPodcastPause->setEnabled(true);

        //WBApplication::mainWindow->actionPodcastMic->setEnabled(false);

        WBApplication::mainWindow->actionPodcastConfig->setEnabled(false);
    }
    else if (state == WBPodcastController::Stopped)
    {
        WBApplication::mainWindow->actionPodcastRecord->setChecked(false);
        WBApplication::mainWindow->actionPodcastRecord->setEnabled(true);

        WBApplication::mainWindow->actionPodcastPause->setChecked(false);
        WBApplication::mainWindow->actionPodcastPause->setEnabled(false);

        //WBApplication::mainWindow->actionPodcastMic->setEnabled(true);
        WBApplication::mainWindow->actionPodcastConfig->setEnabled(true);
    }
    else if (state == WBPodcastController::Paused)
    {
        WBApplication::mainWindow->actionPodcastRecord->setChecked(true);
        WBApplication::mainWindow->actionPodcastRecord->setEnabled(true);

        WBApplication::mainWindow->actionPodcastPause->setChecked(true);
        WBApplication::mainWindow->actionPodcastPause->setEnabled(true);

        //WBApplication::mainWindow->actionPodcastMic->setEnabled(false);
        WBApplication::mainWindow->actionPodcastConfig->setEnabled(false);
    }
    else
    {
        WBApplication::mainWindow->actionPodcastRecord->setEnabled(false);
        WBApplication::mainWindow->actionPodcastPause->setEnabled(false);
        WBApplication::mainWindow->actionPodcastConfig->setEnabled(false);
    }
}


void WBPodcastRecordingPalette::recordingProgressChanged(qint64 ms)
{
    int min = ms / 60000;
    int seconds = (ms / 1000) % 60;

    mTimerLabel->setText(QString("%1:%2").arg(min, 3, 10, QChar(' ')).arg(seconds, 2, 10, QChar('0')));
}


void WBPodcastRecordingPalette::audioLevelChanged(quint8 level)
{
    mLevelMeter->setVolume(level);
}


WBVuMeter::WBVuMeter(QWidget* pParent)
    : QWidget(pParent)
    , mVolume(0)
{
    // NOOP
}


WBVuMeter::~WBVuMeter()
{
    // NOOP
}

void WBVuMeter::setVolume(quint8 pVolume)
{
    if (mVolume != pVolume)
    {
        mVolume = pVolume;
        update();
    }
}


void WBVuMeter::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter painter(this);

    int h = (height() - 8) * mVolume / 255;
    QRectF rect(0, height() - 4 - h, width(), h);

    painter.fillRect(rect, WBSettings::documentViewLightColor);
}

