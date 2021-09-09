#ifndef WBMIMEDATA_H_
#define WBMIMEDATA_H_

#include <QtWidgets>

class WBDocumentProxy;
class WBItem;

struct WBMimeDataItem
{
    public:
        WBMimeDataItem(WBDocumentProxy* proxy, int sceneIndex);
        virtual ~WBMimeDataItem();

        WBDocumentProxy* documentProxy() const { return mProxy; }
        int sceneIndex() const { return mSceneIndex; }

    private:
        WBDocumentProxy* mProxy;
        int mSceneIndex;
};

class WBMimeDataGraphicsItem : public QMimeData
{
    Q_OBJECT

    public:
        WBMimeDataGraphicsItem(QList<WBItem*> pItems);
        virtual ~WBMimeDataGraphicsItem();

        QList<WBItem*> items() const { return mItems; }

    private:
        QList<WBItem*> mItems;

};

class WBMimeData : public QMimeData
{
    Q_OBJECT

    public:
        WBMimeData(const QList<WBMimeDataItem> &items);
        virtual ~WBMimeData();

        QList<WBMimeDataItem> items() const { return mItems; }

    private:
        QList<WBMimeDataItem> mItems;
};

#endif /* WBMIMEDATA_H_ */
