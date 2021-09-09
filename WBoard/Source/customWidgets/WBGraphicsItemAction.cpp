#include "WBGraphicsItemAction.h"
#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "board/WBBoardController.h"
#include "web/WBWebController.h"
#include "document/WBDocumentController.h"
#include "document/WBDocumentProxy.h"
#include "document/WBDocumentContainer.h"

#include "board/WBBoardController.h"


WBGraphicsItemAction::WBGraphicsItemAction(eWBGraphicsItemLinkType linkType, QObject *parent) :
    QObject(parent)
{
    mLinkType = linkType;
}

 void WBGraphicsItemAction::actionRemoved()
 {
    //NOOP
 }


WBGraphicsItemPlayAudioAction::WBGraphicsItemPlayAudioAction(QString audioFile, bool onImport, QObject *parent) :
    WBGraphicsItemAction(eLinkToAudio,parent)
  , mAudioOutput(NULL)
  , mIsLoading(true)
{
    Q_ASSERT(audioFile.length() > 0);

    if(onImport){
        QString extension = QFileInfo(audioFile).completeSuffix();
        QString destDir = WBApplication::boardController->selectedDocument()->persistencePath() + "/" + WBPersistenceManager::audioDirectory;
        QString destFile = destDir + "/" + QUuid::createUuid().toString() + "." + extension;
        if(!QDir(destDir).exists())
            QDir(WBApplication::boardController->selectedDocument()->persistencePath()).mkdir(destDir);
        //explanation : the audioFile could be relative. The method copy will return false so a second try is done adding
        // the document file path
        if(!QFile(audioFile).copy(destFile))
            QFile(WBApplication::boardController->selectedDocument()->persistencePath() + "/" + audioFile).copy(destFile);
        mAudioPath = destFile;
        mFullPath = destFile;
    }
    else
    {
        //On import don't recreate the file
        mAudioPath = audioFile;
        mFullPath = mAudioPath;
    }

    mAudioOutput = new QMediaPlayer(this);
	mAudioOutput->setMedia(QUrl::fromLocalFile(mAudioPath));
}


WBGraphicsItemPlayAudioAction::WBGraphicsItemPlayAudioAction() :
    WBGraphicsItemAction(eLinkToAudio,NULL)
  , mAudioOutput(0)
  , mIsLoading(true)
{
}


void WBGraphicsItemPlayAudioAction::setPath(QString audioPath)
{
    Q_ASSERT(audioPath.length() > 0);
    mAudioPath = audioPath;
    mFullPath = mAudioPath;
    mAudioOutput = new QMediaPlayer(this);
	mAudioOutput->setMedia(QUrl::fromLocalFile(mAudioPath)); 
}

QString WBGraphicsItemPlayAudioAction::fullPath()
{
    return mFullPath;
}

WBGraphicsItemPlayAudioAction::~WBGraphicsItemPlayAudioAction()
{
    if(!mAudioOutput && mAudioOutput->state() == QMediaPlayer::PlayingState)
		mAudioOutput->stop();
}

void WBGraphicsItemPlayAudioAction::onSourceHide()
{
    if(mAudioOutput && mAudioOutput->state() == QMediaPlayer::PlayingState){
		mAudioOutput->stop();
    }
}

void WBGraphicsItemPlayAudioAction::play()
{
    if(mAudioOutput->state() == QMediaPlayer::PlayingState){
		mAudioOutput->stop();
    }
	mAudioOutput->setPosition(0);
	mAudioOutput->play();
}


QStringList WBGraphicsItemPlayAudioAction::save()
{
    //Another hack
    if(WBApplication::documentController && WBApplication::documentController->selectedDocument()){
        QString documentPath = WBApplication::documentController->selectedDocument()->persistencePath() + "/";
        return QStringList() << QString("%1").arg(eLinkToAudio) <<  mAudioPath.replace(documentPath,"");
    }
    else{
        int index = mAudioPath.indexOf("/audios/");
        QString relativePath = mAudioPath.remove(0,index + 1);
        return QStringList() << QString("%1").arg(eLinkToAudio) <<  relativePath;
    }
}

void WBGraphicsItemPlayAudioAction::actionRemoved()
{
    QFile(mAudioPath).remove();
}


WBGraphicsItemMoveToPageAction::WBGraphicsItemMoveToPageAction(eWBGraphicsItemMovePageAction actionType, int page, QObject* parent) :
    WBGraphicsItemAction(eLinkToPage,parent)
{
    mActionType = actionType;
    mPage = page;
}

void WBGraphicsItemMoveToPageAction::play()
{
    WBBoardController* boardController = WBApplication::boardController;

    switch (mActionType) {
    case eMoveToFirstPage:
        boardController->firstScene();
        break;
    case eMoveToLastPage:
        boardController->lastScene();
        break;
    case eMoveToPreviousPage:
        boardController->previousScene();
        break;
    case eMoveToNextPage:
        boardController->nextScene();
        break;
    case eMoveToPage:
        if(mPage >= 0 && mPage < boardController->pageCount())
            boardController->setActiveDocumentScene(mPage);
        else
            qWarning() << "scene number " << mPage << "ins't accessible anymore";
        break;
    default:
        break;
    }
}

QStringList WBGraphicsItemMoveToPageAction::save()
{
    return QStringList() << QString("%1").arg(eLinkToPage) << QString("%1").arg(mActionType) << QString("%1").arg(mPage);
}


WBGraphicsItemLinkToWebPageAction::WBGraphicsItemLinkToWebPageAction(QString url, QObject *parent) :
    WBGraphicsItemAction(eLinkToWebUrl,parent)
{
    if (url.length() > 0)
    {
        if(!url.startsWith("http://"))
            url = "http://" + url;
        mUrl = url;
    }
    else
        mUrl = QString();
}

void WBGraphicsItemLinkToWebPageAction::play()
{
    if (mUrl.length() > 0)
        WBApplication::webController->loadUrl(QUrl(mUrl));
}

QStringList WBGraphicsItemLinkToWebPageAction::save()
{
    return QStringList() << QString("%1").arg(eLinkToWebUrl) << mUrl;
}
