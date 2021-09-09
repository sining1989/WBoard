#ifndef WBDOCUMENTNAVIGATOR_H
#define WBDOCUMENTNAVIGATOR_H

#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QThread>

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentContainer.h"
#include "WBThumbnailWidget.h"

#define NO_PAGESELECTED -1

class WBDocumentNavigator : public QGraphicsView
{
    Q_OBJECT
public:
    WBDocumentNavigator(QWidget* parent=0, const char* name="documentNavigator");
    ~WBDocumentNavigator();

    void setNbColumns(int nbColumns);
    int nbColumns();
    void setThumbnailMinWidth(int width);
    int thumbnailMinWidth();
    WBSceneThumbnailNavigPixmap* clickedThumbnail(const QPoint pos) const;

public slots:
    void onScrollToSelectedPage(int index);
    void generateThumbnails(WBDocumentContainer* source);
    void updateSpecificThumbnail(int iPage);    

    void longPressTimeout();
    void mousePressAndHoldEvent();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);

signals:
    void mousePressAndHoldEventRequired();
    void moveThumbnailRequired(int from, int to);

private:
    void refreshScene();
    int border();

    /** The scene */
    QGraphicsScene* mScene;
    /** The list of current thumbnails with labels*/
    QList<WBImgTextThumbnailElement> mThumbsWithLabels;
    /** The current number of columns */
    int mNbColumns;
    /** The current thumbnails width */
    int mThumbnailWidth;
    /** The current thumbnails minimum width */
    int mThumbnailMinWidth;
    /** The selected thumbnail */
    WBSceneThumbnailNavigPixmap* mSelectedThumbnail;
    WBSceneThumbnailNavigPixmap* mLastClickedThumbnail;
    WBSceneThumbnailNavigPixmap* mDropSource;
    WBSceneThumbnailNavigPixmap* mDropTarget;
    QGraphicsRectItem *mDropBar;

    int mLongPressInterval;
    QTimer mLongPressTimer;
    QPoint mLastPressedMousePos;
};

#endif // WBDOCUMENTNAVIGATOR_H

