#ifndef WBCOREGRAPHICSSCENE_H_
#define WBCOREGRAPHICSSCENE_H_

#include <QtWidgets>
#include <QGraphicsScene>
#include <QGraphicsItem>

class WBCoreGraphicsScene : public QGraphicsScene
{
    public:
        WBCoreGraphicsScene(QObject * parent = 0);
        virtual ~WBCoreGraphicsScene();

        virtual void addItem(QGraphicsItem* item);

        virtual void removeItem(QGraphicsItem* item, bool forceDelete = false);

        virtual bool deleteItem(QGraphicsItem* item);

        void removeItemFromDeletion(QGraphicsItem* item);
        void addItemToDeletion(QGraphicsItem *item);

        bool isModified() const
        {
            return mIsModified;
        }

        void setModified(bool pModified)
        {
            mIsModified = pModified;
        }

    private:
        QSet<QGraphicsItem*> mItemsToDelete;

        bool mIsModified;
};

#endif /* WBCOREGRAPHICSSCENE_H_ */
