#ifndef WBGRAPHICSITEMSACTIONS_H
#define WBGRAPHICSITEMSACTIONS_H

#include <QObject>
#include <QtMultimedia>

enum eWBGraphicsItemMovePageAction {
    eMoveToFirstPage = 0,
    eMoveToLastPage,
    eMoveToPreviousPage,
    eMoveToNextPage,
    eMoveToPage
};

enum eWBGraphicsItemLinkType
{
    eLinkToAudio = 0,
    eLinkToPage,
    eLinkToWebUrl
};

class WBGraphicsItemAction : public QObject
{
    Q_OBJECT
public:
    WBGraphicsItemAction(eWBGraphicsItemLinkType linkType,QObject* parent = 0);
    virtual void play() = 0;
    virtual QStringList save() = 0;
    virtual void actionRemoved();
    virtual QString path() {return "";}
    eWBGraphicsItemLinkType linkType() { return mLinkType;}

private:
    eWBGraphicsItemLinkType mLinkType;
};

class WBGraphicsItemPlayAudioAction : public WBGraphicsItemAction
{
    Q_OBJECT

public:
    WBGraphicsItemPlayAudioAction(QString audioFile, bool onImport = true, QObject* parent = 0);
    WBGraphicsItemPlayAudioAction();
    ~WBGraphicsItemPlayAudioAction();
    void play();
    QStringList save();
    void actionRemoved();
    QString path() {return mAudioPath;}
    void setPath(QString audioPath);
    QString fullPath();

public slots:
    void onSourceHide();

private:
    QString mAudioPath;
	QMediaPlayer *mAudioOutput;
    bool mIsLoading;
    QString mFullPath;
};

class WBGraphicsItemMoveToPageAction : public WBGraphicsItemAction
{
    Q_OBJECT

public:
    WBGraphicsItemMoveToPageAction(eWBGraphicsItemMovePageAction actionType, int page = 0, QObject* parent = 0);
    void play();
    QStringList save();
    int page(){return mPage;}
    eWBGraphicsItemMovePageAction actionType(){return mActionType;}

private:
    eWBGraphicsItemMovePageAction mActionType;
    int mPage;
};

class WBGraphicsItemLinkToWebPageAction : public WBGraphicsItemAction
{
    Q_OBJECT

public:
    WBGraphicsItemLinkToWebPageAction(QString url, QObject* parent = 0);
    void play();
    QStringList save();
    QString url(){return mUrl;}

private:
    QString mUrl;
};

#endif // WBGRAPHICSITEMSACTIONS_H
