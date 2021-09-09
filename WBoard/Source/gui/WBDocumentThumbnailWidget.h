#ifndef WBDOCUMENTTHUMBNAILWIDGET_H_
#define WBDOCUMENTTHUMBNAILWIDGET_H_

#include "WBThumbnailWidget.h"

class WBGraphicsScene;

class WBDocumentThumbnailWidget: public WBThumbnailWidget
{
    Q_OBJECT

    public:
        WBDocumentThumbnailWidget(QWidget* parent);
        virtual ~WBDocumentThumbnailWidget();

        void setDragEnabled(bool enabled);
        bool dragEnabled() const;

        void hightlightItem(int index);

    public slots:
        virtual void setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems,
            const QList<QUrl>& pItemPaths, const QStringList pLabels = QStringList(),
            const QString& pMimeType = QString(""));

	signals:
        void sceneDropped(WBDocumentProxy* proxy, int source, int target);

    private slots:
        void autoScroll();

    protected:
        virtual void mouseMoveEvent(QMouseEvent *event);

        virtual void dragEnterEvent(QDragEnterEvent *event);
        virtual void dragLeaveEvent(QDragLeaveEvent *event);
        virtual void dragMoveEvent(QDragMoveEvent *event);
        virtual void dropEvent(QDropEvent *event);

    private:
        void deleteDropCaret();

        QGraphicsRectItem *mDropCaretRectItem;
        WBThumbnailPixmap *mClosestDropItem;
        bool mDropIsRight;
        bool mDragEnabled;
        QTimer* mScrollTimer;
        int mScrollMagnitude;
};

#endif /* WBDOCUMENTTHUMBNAILWIDGET_H_ */
