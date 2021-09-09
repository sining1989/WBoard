#ifndef WBMEDIAWIDGET_H
#define WBMEDIAWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QtMultimedia>
#include <QVideoWidget>

#include "WBActionableWidget.h"

#define WBMEDIABUTTON_SIZE              32
#define TICK_INTERVAL                   1000

typedef enum{
    eMediaType_Video,
    eMediaType_Audio
}eMediaType;

class WBMediaButton : public QLabel
{
    Q_OBJECT
public:
    WBMediaButton(QWidget* parent=0, const char* name="WBMediaButton");
    ~WBMediaButton();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* ev);
    void mouseReleaseEvent(QMouseEvent* ev);

private:
    bool mPressed;
};

class WBMediaWidget : public WBActionableWidget
{
    Q_OBJECT
public:
    WBMediaWidget(eMediaType type = eMediaType_Video, QWidget* parent=0, const char* name="WBMediaWidget");
    ~WBMediaWidget();
    void setFile(const QString& filePath);
    eMediaType mediaType();
    int border();
    void setAudioCover(const QString& coverPath);
    void setUrl(const QString& url){mUrl = url;}
    QString url(){return mUrl;}

protected:
    void resizeEvent(QResizeEvent* ev);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    QString mFilePath;

private slots:
    void onPlayStopClicked();
    void onPauseClicked();
    void onStateChanged(QMediaPlayer::State state);
    void onTotalTimeChanged(qint64 total);
    void onTick(qint64 currentTime);
    void onSliderChanged(int value);

private:
    void createMediaPlayer();
    void adaptSizeToVideo();

    eMediaType mType;
	QMediaPlayer* mpMediaObject;
	QVideoWidget* mpVideoWidget;
	QMediaPlayer* mpAudioOutput;
    QVBoxLayout* mpLayout;
    QHBoxLayout* mpSeekerLayout;
    WBMediaButton* mpPlayStopButton;
    WBMediaButton* mpPauseButton;
    QSlider* mpSlider;
    bool mAutoUpdate;
    bool mGeneratingThumbnail;
    int mBorder;
    QWidget* mpMediaContainer;
    QHBoxLayout* mMediaLayout;
    QLabel* mpCover;
    QString mUrl;
};

#endif // WBMEDIAWIDGET_H
