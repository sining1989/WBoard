#ifndef WBGRAPHICSMEDIAITEMDELEGATE_H_
#define WBGRAPHICSMEDIAITEMDELEGATE_H_

#include <QtWidgets>
#include <QTimer>
#include <QtMultimedia>

#include "core/WB.h"
#include "WBGraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;

class WBGraphicsMediaItemDelegate :  public WBGraphicsItemDelegate
{
    Q_OBJECT

public:
    WBGraphicsMediaItemDelegate(WBGraphicsMediaItem* pDelegated, QObject * parent = 0);
    virtual ~WBGraphicsMediaItemDelegate();

    virtual void positionHandles();

    bool mousePressEvent(QGraphicsSceneMouseEvent* event);

    void showToolBar(bool autohide = true);

public slots:
    void toggleMute();
    void updateTicker(qint64 time);
    virtual void showHide(bool show);

    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void mediaStateChanged(QMediaPlayer::State state);

protected slots:
    virtual void remove(bool canUndo = true);

    void togglePlayPause();

    void updatePlayPauseState();

    void totalTimeChanged(qint64 newTotalTime);

    void hideToolBar();


protected:
    virtual void buildButtons();

    WBGraphicsMediaItem* delegated();

    DelegateButton* mPlayPauseButton;
    DelegateButton* mStopButton;
    DelegateButton* mMuteButton;
    DelegateMediaControl* mMediaControl;

    QTimer* mToolBarShowTimer;
    int m_iToolBarShowingInterval;
};

#endif /* WBGRAPHICSMEDIAITEMDELEGATE_H_ */
