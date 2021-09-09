#ifndef WBBOARDTHUMBNAILSVIEW_H
#define WBBOARDTHUMBNAILSVIEW_H

#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>

#include "document/WBDocumentContainer.h"
#include "WBThumbnailWidget.h"

class WBBoardThumbnailsView : public QGraphicsView
{
    Q_OBJECT
public:
    WBBoardThumbnailsView(QWidget* parent=0, const char* name="WBBoardThumbnailsView");

public slots:
    void ensureVisibleThumbnail(int index);
    void centerOnThumbnail(int index);

    void clearThumbnails();
    void initThumbnails(WBDocumentContainer* source);
    void addThumbnail(WBDocumentContainer* source, int i);
    void moveThumbnail(int from, int to);
    void removeThumbnail(int i);
    void updateThumbnails();

    void longPressTimeout();
    void mousePressAndHoldEvent(QPoint pos);

protected:
    virtual void resizeEvent(QResizeEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

signals:
    void mousePressAndHoldEventRequired(QPoint pos);
    void moveThumbnailRequired(int from, int to);

private:
    WBDraggableThumbnailView* createThumbnail(WBDocumentContainer* source, int i);
    void updateThumbnailsPos();

    QList<WBDraggableThumbnailView*> mThumbnails;

    int mThumbnailWidth;
    const int mThumbnailMinWidth;
    const int mMargin;

    WBDraggableThumbnailView* mDropSource;
    WBDraggableThumbnailView* mDropTarget;
    QGraphicsRectItem *mDropBar;

    int mLongPressInterval;
    QTimer mLongPressTimer;
    QPoint mLastPressedMousePos;
};

#endif // WBBOARDTHUMBNAILSVIEW_H
