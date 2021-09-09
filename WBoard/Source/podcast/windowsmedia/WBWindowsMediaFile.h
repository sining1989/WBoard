#ifndef WBWINDOWSMEDIAFILE_H_
#define WBWINDOWSMEDIAFILE_H_

#include <QtCore>

#ifndef inteface
#define interface struct
#endif
#include <wmsdk.h>
#pragma comment(lib, "wmvcore.lib")

class WBWindowsMediaFile : public QObject
{
    Q_OBJECT;

    public:
        WBWindowsMediaFile(QObject * pParent = 0);
        virtual ~WBWindowsMediaFile();

        bool init(const QString& videoFileName, const QString& profileData
                , int pFramesPerSecond, int pixelWidth, int pixelHeight, int bitsPerPixel);

        bool close();

        bool appendVideoFrame(const QImage& pPix, long msTimeStamp);

        void startNewChapter(const QString& pLabel, long timestamp);

        QString lastErrorMessage() const
        {
            return mLastErrorMessage;
        }

    public slots:
       void appendAudioBuffer(WAVEHDR*, long);

    private:
        bool initVideoStream(int pixelWidth, int pixelHeight, int bitsPerPixel);

        void setLastErrorMessage(const QString& error);

        void releaseWMObjects();

        QWORD msToSampleTime(long ms)
        {
            QWORD qwordMs = ms;
            return (qwordMs * 10000);
        }

        IWMProfile *mWMProfile;
        IWMWriter *mWMWriter;
        IWMWriterPushSink *mPushSink;

        IWMInputMediaProps *mWMInputVideoProps;
        IWMInputMediaProps *mWMInputAudioProps;
        IWMProfileManager *mWMProfileManager;

        HDC mWMhDC;
        DWORD mVideoInputIndex;
        DWORD mAudioInputIndex;
        DWORD mFramesPerSecond;

        QString mLastErrorMessage;

        struct MarkerInfo
        {
            QString label;
            long timestamp;
        };

        QList<MarkerInfo> mChapters;

        QString mVideoFileName;

};

#endif /* WBWINDOWSMEDIAFILE_H_ */
