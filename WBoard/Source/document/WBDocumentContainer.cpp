#include "WBDocumentContainer.h"
#include "adaptors/WBThumbnailAdaptor.h"
#include "core/WBPersistenceManager.h"
#include "core/memcheck.h"


WBDocumentContainer::WBDocumentContainer(QObject * parent)
    :QObject(parent)
    ,mCurrentDocument(NULL)
{}

WBDocumentContainer::~WBDocumentContainer()
{
    foreach(const QPixmap* pm, mDocumentThumbs){
        delete pm;
        pm = NULL;
    }
}

void WBDocumentContainer::setDocument(WBDocumentProxy* document, bool forceReload)
{
    if (mCurrentDocument != document || forceReload)
    {
        mCurrentDocument = document;

        reloadThumbnails();
        emit documentSet(mCurrentDocument);
    }
}

void WBDocumentContainer::duplicatePages(QList<int>& pageIndexes)
{
    int offset = 0;
    foreach(int sceneIndex, pageIndexes)
    {
        WBPersistenceManager::persistenceManager()->duplicateDocumentScene(mCurrentDocument, sceneIndex + offset);
        offset++;
    }
}

bool WBDocumentContainer::movePageToIndex(int source, int target)
{
    //on document view
    WBPersistenceManager::persistenceManager()->moveSceneToIndex(mCurrentDocument, source, target);
    deleteThumbPage(source);
    insertThumbPage(target);
    emit documentThumbnailsUpdated(this);
    //on board thumbnails view
    emit moveThumbnailRequired(source, target);
    return true;
}

void WBDocumentContainer::deletePages(QList<int>& pageIndexes)
{
    WBPersistenceManager::persistenceManager()->deleteDocumentScenes(mCurrentDocument, pageIndexes);
    int offset = 0;
    foreach(int index, pageIndexes)
    {
        deleteThumbPage(index - offset);
        emit removeThumbnailRequired(index - offset);
        offset++;

    }
    emit documentThumbnailsUpdated(this);
}

void WBDocumentContainer::addPage(int index)
{
    WBPersistenceManager::persistenceManager()->createDocumentSceneAt(mCurrentDocument, index);
    insertThumbPage(index);

    emit documentThumbnailsUpdated(this);
    emit addThumbnailRequired(this, index);
}


void WBDocumentContainer::addPixmapAt(const QPixmap *pix, int index)
{
    mDocumentThumbs.insert(index, pix);
    emit documentThumbnailsUpdated(this);
}


void WBDocumentContainer::clearThumbPage()
{
    qDeleteAll(mDocumentThumbs);
    mDocumentThumbs.clear();
}

void WBDocumentContainer::initThumbPage()
{
    clearThumbPage();

    for (int i=0; i < selectedDocument()->pageCount(); i++)
        insertThumbPage(i);
}

void WBDocumentContainer::updatePage(int index)
{
    updateThumbPage(index);
    emit documentThumbnailsUpdated(this);
}

void WBDocumentContainer::deleteThumbPage(int index)
{
    mDocumentThumbs.removeAt(index);
}

void WBDocumentContainer::updateThumbPage(int index)
{
    if (mDocumentThumbs.size() > index)
    {
        mDocumentThumbs[index] = WBThumbnailAdaptor::get(mCurrentDocument, index);
        emit documentPageUpdated(index);
    }
    else
    {
        qDebug() << "error [updateThumbPage] : index > mDocumentThumbs' size.";
    }
}

void WBDocumentContainer::insertThumbPage(int index)
{
    mDocumentThumbs.insert(index, WBThumbnailAdaptor::get(mCurrentDocument, index));
}

void WBDocumentContainer::reloadThumbnails()
{
    if (mCurrentDocument)
    {
        WBThumbnailAdaptor::load(mCurrentDocument, mDocumentThumbs);
    }
    emit documentThumbnailsUpdated(this);
}

int WBDocumentContainer::pageFromSceneIndex(int sceneIndex)
{
    return sceneIndex+1;
}

int WBDocumentContainer::sceneIndexFromPage(int page)
{
    return page-1;
}

void WBDocumentContainer::addEmptyThumbPage()
{
    const QPixmap* pThumb = new QPixmap();
    mDocumentThumbs.append(pThumb);
}
