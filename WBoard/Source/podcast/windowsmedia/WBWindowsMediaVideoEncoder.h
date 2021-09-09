#ifndef WBWINDOWSMEDIAVIDEOENCODER_H_
#define WBWINDOWSMEDIAVIDEOENCODER_H_

#include <QtGui>
#include "podcast/WBAbstractVideoEncoder.h"

#include "WBWindowsMediaFile.h"
#include "WBWaveRecorder.h"

class WBWindowsMediaVideoEncoder : public WBAbstractVideoEncoder
{
    Q_OBJECT;

    public:
        WBWindowsMediaVideoEncoder(QObject* pParent = 0);

        virtual ~WBWindowsMediaVideoEncoder();

        virtual bool start();
        virtual bool pause();
        virtual bool unpause();
        virtual bool stop();

        virtual bool canPause() { return true;};

        virtual void newPixmap(const QImage& pPix, long timestamp);
        virtual void newChapter(const QString& pLabel, long timestamp);

        virtual QString videoFileExtension() const
        {
            return "wmv";
        }

        virtual QString lastErrorMessage()
        {
            return mLastErrorMessage;
        }

        virtual void setRecordAudio(bool pRecordAudio);

    private slots:
        void processAudioBuffer(WAVEHDR*, long);

    private:
        QPointer<WBWindowsMediaFile> mWMVideo;
        QPointer<WBWaveRecorder> mWaveRecorder;

        QString mLastErrorMessage;

        bool mRecordAudio;
        bool mIsPaused;

        quint8 mLastAudioLevel;
        bool mIsRecording;

};

#endif /* WBWINDOWSMEDIAVIDEOENCODER_H_ */
