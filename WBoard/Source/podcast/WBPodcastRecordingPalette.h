#ifndef WBPODCASTRECORDINGPALETTE_H_
#define WBPODCASTRECORDINGPALETTE_H_

#include "gui/WBActionPalette.h"
#include "WBPodcastController.h"

#include <QtGui>
#include <QLabel>

class WBVuMeter;

class WBPodcastRecordingPalette : public WBActionPalette
{
    Q_OBJECT;

    public:
        WBPodcastRecordingPalette(QWidget *parent = 0);
        virtual ~WBPodcastRecordingPalette();

    public slots:
        void recordingStateChanged(WBPodcastController::RecordingState);
        void recordingProgressChanged(qint64 ms);
        void audioLevelChanged(quint8 level);

    private:
        QLabel *mTimerLabel;
        WBVuMeter *mLevelMeter;
};


class WBVuMeter : public QWidget
{
    public:
        WBVuMeter(QWidget* pParent);
        virtual ~WBVuMeter();

        void setVolume(quint8 pVolume);

    protected:
        virtual void paintEvent(QPaintEvent* e);

    private:
        quint8 mVolume;

};

#endif /* WBPODCASTRECORDINGPALETTE_H_ */
