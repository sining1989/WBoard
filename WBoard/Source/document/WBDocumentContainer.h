#ifndef WBDOCUMENTCONTAINER_H_
#define WBDOCUMENTCONTAINER_H_

#include <QtWidgets>
#include "WBDocumentProxy.h"

class WBDocumentContainer : public QObject
{
    Q_OBJECT

    public:
        WBDocumentContainer(QObject * parent = 0);
        virtual ~WBDocumentContainer();

        void setDocument(WBDocumentProxy* document, bool forceReload = false);
        void pureSetDocument(WBDocumentProxy *document) {mCurrentDocument = document;}

        WBDocumentProxy* selectedDocument(){return mCurrentDocument;}
        int pageCount(){return mCurrentDocument->pageCount();}
        const QPixmap* pageAt(int index)
        {
            if (index < mDocumentThumbs.size())
                return mDocumentThumbs[index];
            else
            {
                return NULL;
            }
        }

        static int pageFromSceneIndex(int sceneIndex);
        static int sceneIndexFromPage(int sceneIndex);

        void duplicatePages(QList<int>& pageIndexes);
        bool movePageToIndex(int source, int target);
        void deletePages(QList<int>& pageIndexes);
        void clearThumbPage();
        void initThumbPage();
        void addPage(int index);
        void addPixmapAt(const QPixmap *pix, int index);
        void updatePage(int index);
        void addEmptyThumbPage();
        void reloadThumbnails();

        void insertThumbPage(int index);

    private:
        WBDocumentProxy* mCurrentDocument;
        QList<const QPixmap*>  mDocumentThumbs;

    protected:
        void deleteThumbPage(int index);
        void updateThumbPage(int index);

    signals:
        void documentSet(WBDocumentProxy* document);
        void documentPageUpdated(int index);

        void initThumbnailsRequired(WBDocumentContainer* source);
        void addThumbnailRequired(WBDocumentContainer* source, int index);
        void removeThumbnailRequired(int index);
        void moveThumbnailRequired(int from, int to);
        void updateThumbnailsRequired();

        void documentThumbnailsUpdated(WBDocumentContainer* source);
};

#endif /* WBDOCUMENTPROXY_H_ */
