#include "WBWindowsMediaVideoEncoder.h"

#include <QtGui>

#include "frameworks/WBFileSystemUtils.h"

#include "core/WBApplication.h"
#include "core/memcheck.h"

WBWindowsMediaVideoEncoder::WBWindowsMediaVideoEncoder(QObject* pParent)
    : WBAbstractVideoEncoder(pParent)
    , mWMVideo(0)
    , mWaveRecorder(0)
    , mRecordAudio(true)
    , mLastAudioLevel(0)
    , mIsPaused(false)
{
    // NOOP
}


WBWindowsMediaVideoEncoder::~WBWindowsMediaVideoEncoder()
{
    // NOOP
}


bool WBWindowsMediaVideoEncoder::start()
{
    QString profile = WBFileSystemUtils::readTextFile(":/podcast/WBoard.prx");

    profile.replace("{in.videoWidth}", QString("%1").arg(videoSize().width()));
    profile.replace("{in.videoHeight}", QString("%1").arg(videoSize().height()));
    profile.replace("{in.bitsPerSecond}", QString("%1").arg(videoBitsPerSecond()));
    profile.replace("{in.nanoSecondsPerFrame}", QString("%1").arg(10000000 / framesPerSecond()));

    qDebug() << profile;

    if(mRecordAudio)
    {
        mWaveRecorder = new WBWaveRecorder(this);

        bool audioAvailable = mWaveRecorder->init(audioRecordingDevice()) && mWaveRecorder->start();

        if (!audioAvailable)
        {
            mWaveRecorder->deleteLater();
            mWaveRecorder = 0;
            mRecordAudio = false;
        }
    }

    mWMVideo = new WBWindowsMediaFile(this);

    if(!mWMVideo->init(videoFileName().replace("/", "\\"), profile, framesPerSecond(),
        videoSize().width(), videoSize().height(), 32))
    {
        mWMVideo->deleteLater();
        return false;
    }

    if (mRecordAudio)
    {
        connect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), mWMVideo
                        , SLOT(appendAudioBuffer(WAVEHDR*, long)), Qt::DirectConnection);
        connect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), this
                        , SLOT(processAudioBuffer(WAVEHDR*, long)), Qt::DirectConnection);
    }

    mIsRecording = true;

    return true;
}


bool WBWindowsMediaVideoEncoder::stop()
{
    bool audioOk = true;

    if (mWaveRecorder)
    {
        disconnect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), mWMVideo
                , SLOT(appendAudioBuffer(WAVEHDR*, long)));
        disconnect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), this
                , SLOT(processAudioBuffer(WAVEHDR*, long)));

        mWaveRecorder->stop();
        audioOk = mWaveRecorder->close();
        mLastErrorMessage = mWaveRecorder->lastErrorMessage();
        mWaveRecorder->deleteLater();
        mWaveRecorder = 0;
        emit audioLevelChanged(0);
    }

    bool videoOk = true;

    if (mWMVideo)
    {
        videoOk = mWMVideo->close();
        mLastErrorMessage = mWMVideo->lastErrorMessage();
        mWMVideo->deleteLater();
        mWMVideo = 0;
    }

    bool ok = audioOk && videoOk;

    emit encodingFinished(ok);

    mIsRecording = false;

    return ok;
}


void WBWindowsMediaVideoEncoder::newPixmap(const QImage& pPix, long timestamp)
{
    if(mWMVideo && !mIsPaused)
    {
        if(!mWMVideo->appendVideoFrame(pPix, timestamp))
        {
            qWarning() << "Error adding new video frame" << mWMVideo->lastErrorMessage();
        }
    }
}


void WBWindowsMediaVideoEncoder::newChapter(const QString& pLabel, long timestamp)
{
    if(mWMVideo)
       mWMVideo->startNewChapter(pLabel, timestamp);
}


void WBWindowsMediaVideoEncoder::setRecordAudio(bool pRecordAudio)
{
    if (mRecordAudio != pRecordAudio)
    {
        mRecordAudio = pRecordAudio;

        if (mRecordAudio)
        {
            connect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), mWMVideo, SLOT(appendAudioBuffer(WAVEHDR*, long)), Qt::DirectConnection);
        }
        else
        {
            disconnect(mWaveRecorder, SIGNAL(newWaveBuffer(WAVEHDR*, long)), mWMVideo, SLOT(appendAudioBuffer(WAVEHDR*, long)));
            emit audioLevelChanged(0);
        }
    }
}


void WBWindowsMediaVideoEncoder::processAudioBuffer(WAVEHDR* waveBuffer, long timestamp)
{
    Q_UNUSED(timestamp);

    if(mWaveRecorder && mRecordAudio)
    {
        long samplesCount = waveBuffer->dwBytesRecorded / 2;
        qint16* samples = (qint16*)waveBuffer->lpData;
        quint16 maxRMS = 0;

        for(long i = 0; i < samplesCount; i++)
        {
            quint8 current = qAbs(samples[i] / 128);

            quint16 currentRMS = current * current;
            maxRMS = qMax(maxRMS, currentRMS);
        }

        quint8 max = sqrt((qreal)maxRMS);

        if (max != mLastAudioLevel)
        {
            mLastAudioLevel = max;
            emit audioLevelChanged(mLastAudioLevel);
        }
    }

}


bool WBWindowsMediaVideoEncoder::pause()
{
    bool result = true;

    if(!mIsPaused && mIsRecording)
    {
        if(mWaveRecorder)
        {
            result = mWaveRecorder->stop();
            emit audioLevelChanged(0);
        }

        mIsPaused = true;
    }

    return result;
}


bool WBWindowsMediaVideoEncoder::unpause()
{
    bool result = true;

    if (mIsPaused && mIsRecording)
    {
        if(mWaveRecorder)
        {
            result = mWaveRecorder->start();
        }

        mIsPaused = false;
    }

    return result;
}




