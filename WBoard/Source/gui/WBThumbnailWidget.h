#ifndef WBTHUMBNAILWIDGET_H_
#define WBTHUMBNAILWIDGET_H_

#include <QtWidgets>
#include <QtSvg>
#include <QTime>
#include <QGraphicsSceneHoverEvent>

#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "frameworks/WBCoreGraphicsScene.h"
#include "core/WBSettings.h"
#include "domain/WBItem.h"
#include "gui/WBThumbnailView.h"
#include "document/WBDocumentProxy.h"

#include <QLabel>
#include <QHBoxLayout>

#define STARTDRAGTIME   1000000
#define BUTTONSIZE      96
#define BUTTONSPACING 10

class WBDocumentProxy;
class WBThumbnailTextItem;
class WBThumbnail;

class WBThumbnailWidget : public QGraphicsView
{
    Q_OBJECT

public:
    WBThumbnailWidget(QWidget* parent);
    virtual ~WBThumbnailWidget();

    QList<QGraphicsItem*> selectedItems();
    void selectItemAt(int pIndex, bool extend = false);
    void unselectItemAt(int pIndex);

    qreal thumbnailWidth()
    {
        return mThumbnailWidth;
    }

    void setBackgroundBrush(const QBrush& brush)
    {
        mThumbnailsScene.setBackgroundBrush(brush);
    }

public slots:
    void setThumbnailWidth(qreal pThumbnailWidth);
    void setSpacing(qreal pSpacing);
    virtual void setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems, const QList<QUrl>& pItemPaths, const QStringList pLabels = QStringList(), const QString& pMimeType = QString(""));
    void refreshScene();
    void sceneSelectionChanged();

signals:
    void resized();
    void selectionChanged();
    void mouseDoubleClick(QGraphicsItem* item, int index);
    void mouseClick(QGraphicsItem* item, int index);


protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);

    QList<QGraphicsItem*> mGraphicItems;
    QList<WBThumbnailTextItem*> mLabelsItems;
    QPointF mMousePressScenePos;
    QPoint mMousePressPos;

protected:
    qreal spacing() { return mSpacing; }
    QList<QUrl> mItemsPaths;
    QStringList mLabels;
    bool bSelectionInProgress;
    bool bCanDrag;

private:
    void selectAll();
    void selectItems(int startIndex, int count);
    int rowCount() const;
    int columnCount() const;

    static bool thumbnailLessThan(QGraphicsItem* item1, QGraphicsItem* item2);

    void deleteLasso();

    WBCoreGraphicsScene mThumbnailsScene;

    QString mMimeType;

    QPointF prevMoveMousePos;

    qreal mThumbnailWidth;
    qreal mThumbnailHeight;
    qreal mSpacing;

    WBThumbnail *mLastSelectedThumbnail;
    int mSelectionSpan;
    QRectF mPrevLassoRect;
    QGraphicsRectItem *mLassoRectItem;
    QSet<QGraphicsItem*> mSelectedThumbnailItems;
    QSet<QGraphicsItem*> mPreviouslyIncrementalSelectedItemsX;
    QSet<QGraphicsItem*> mPreviouslyIncrementalSelectedItemsY;
    QTime mClickTime;
};

class WBThumbnail
{
public:
    WBThumbnail();

    virtual ~WBThumbnail();

    QStyleOptionGraphicsItem muteStyleOption(const QStyleOptionGraphicsItem *option)
    {
        QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
        styleOption.state &= ~QStyle::State_Selected;

        return styleOption;
    }

        virtual void itemChange(QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const QVariant &value)
    {
        Q_UNUSED(value);

        if ((change == QGraphicsItem::ItemSelectedHasChanged
                || change == QGraphicsItem::ItemTransformHasChanged
                || change == QGraphicsItem::ItemPositionHasChanged)
                &&  item->scene())
        {
            if (item->isSelected())
            {
                if (!mSelectionItem->scene())
                {
                    item->scene()->addItem(mSelectionItem);
                    mSelectionItem->setZValue(item->zValue() - 1);
                    mAddedToScene = true;
                }

                mSelectionItem->setRect(
                    item->sceneBoundingRect().x() - 5,
                    item->sceneBoundingRect().y() - 5,
                    item->sceneBoundingRect().width() + 10,
                    item->sceneBoundingRect().height() + 10);

                mSelectionItem->show();

            }
            else
            {
                mSelectionItem->hide();
            }
        }
    }

    int column() { return mColumn; }
    void setColumn(int column) { mColumn = column; }
    int row() { return mRow; }
    void setRow(int row) { mRow = row; }
    WBThumbnailTextItem *label(){return mLabel;}
    void setLabel(WBThumbnailTextItem *label){mLabel = label;}

protected:
    QGraphicsRectItem *mSelectionItem;
    private:
    bool mAddedToScene;

    int mColumn;
    int mRow;
    WBThumbnailTextItem *mLabel;
};

class WBThumbnailTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    WBThumbnailTextItem(int index)
        : QGraphicsTextItem(tr("Page %0").arg(index+1))
        , mWidth(0)
        , mUnelidedText(toPlainText())
        , mIsHighlighted(false)
    {

    }

    WBThumbnailTextItem(const QString& text)
        : QGraphicsTextItem(text)
        , mWidth(0)
        , mUnelidedText(text)
        , mIsHighlighted(false)
    {
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    QRectF boundingRect() const { return QRectF(QPointF(0.0, 0.0), QSize(mWidth, QFontMetricsF(font()).height() + 5));}

    void setWidth(qreal pWidth)
    {
            if (mWidth != pWidth)
            {
                    prepareGeometryChange();
                    mWidth = pWidth;
                    computeText();
            }
    }

    qreal width() {return mWidth;}

    void highlight()
    {
            if (!mIsHighlighted)
            {
                    mIsHighlighted = true;
                    computeText();
            }
    }

    void computeText()
    {
        QFontMetricsF fm(font());
        QString elidedText = fm.elidedText(mUnelidedText, Qt::ElideRight, mWidth);

        if (mIsHighlighted)
        {
            setHtml("<span style=\"color: #6682b5\">" + elidedText + "</span>");
        }
        else
        {
            setPlainText(elidedText);
        }
    }

private:
    qreal mWidth;
    QString mUnelidedText;
    bool mIsHighlighted;
};

class WBThumbnailPixmap : public QGraphicsPixmapItem, public WBThumbnail
{
public:
    WBThumbnailPixmap(const QPixmap& pix)
        : QGraphicsPixmapItem(pix)
    {
        setTransformationMode(Qt::SmoothTransformation);
        setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    virtual ~WBThumbnailPixmap()
    {
        // NOOP
    }

    virtual QPainterPath shape () const
    {
        QPainterPath path;
        path.addRect(boundingRect());
        return path;
    }


    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QStyleOptionGraphicsItem styleOption = WBThumbnail::muteStyleOption(option);
        QGraphicsPixmapItem::paint(painter, &styleOption, widget);
    }

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        WBThumbnail::itemChange(this, change, value);
        return QGraphicsPixmapItem::itemChange(change, value);
    }
};

class WBSceneThumbnailPixmap : public WBThumbnailPixmap
{
public:
    WBSceneThumbnailPixmap(const QPixmap& pix, WBDocumentProxy* proxy, int pSceneIndex)
        : WBThumbnailPixmap(pix)
        , mProxy(proxy)
        , mSceneIndex(pSceneIndex)
    {
 
    }

    virtual ~WBSceneThumbnailPixmap()
    {
 
    }

    WBDocumentProxy* proxy()
    {
        return mProxy;
    }

    int sceneIndex()
    {
        return mSceneIndex;
    }

    void highlight()
    {

    }

private:
    WBDocumentProxy* mProxy;
    int mSceneIndex;
};

class WBSceneThumbnailNavigPixmap : public WBSceneThumbnailPixmap
{
public:
    WBSceneThumbnailNavigPixmap(const QPixmap& pix, WBDocumentProxy* proxy, int pSceneIndex);
    ~WBSceneThumbnailNavigPixmap();

    bool editable()
    {
        return mEditable;
    }

    bool deletable()
    {
        return proxy()->pageCount() > 1;
    }

    bool movableUp()
    {
        return sceneIndex() > 0;
    }

    bool movableDown()
    {
        return sceneIndex() < (proxy()->pageCount() -1);
    }

    void showUI()
    {
        setEditable(true);
    }

    void hideUI()
    {
        setEditable(false);
    }

    void setEditable(bool editable)
    {
        mEditable = editable;
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    void deletePage();
    void duplicatePage();
    void moveUpPage();
    void moveDownPage();

    bool mEditable;
};

class WBImgTextThumbnailElement
{
private:
    WBSceneThumbnailNavigPixmap* thumbnail;
    WBThumbnailTextItem* caption;
    int border;

public:
    WBImgTextThumbnailElement(WBSceneThumbnailNavigPixmap* thumb, WBThumbnailTextItem* text): border(0)
    {
        this->thumbnail = thumb;
        this->caption = text;
    }

    WBSceneThumbnailNavigPixmap* getThumbnail() const { return this->thumbnail; }
    void setThumbnail(WBSceneThumbnailNavigPixmap* newGItem) { this->thumbnail = newGItem; }

    WBThumbnailTextItem* getCaption() const { return this->caption; }
    void setCaption(WBThumbnailTextItem* newcaption) { this->caption = newcaption; }

    void Place(int row, int col, qreal width, qreal height);

    int getBorder() const { return this->border; }
    void setBorder(int newBorder) { this->border = newBorder; }
};

class WBThumbnailProxyWidget : public QGraphicsProxyWidget
{
public:
    WBThumbnailProxyWidget(WBDocumentProxy* proxy, int index)
        : mDocumentProxy(proxy)
        , mSceneIndex(index)
    {

    }

    WBDocumentProxy* documentProxy()
    {
        return mDocumentProxy;
    }

    void setSceneIndex(int i)
    {
        mSceneIndex = i;
    }

    int sceneIndex()
    {
        return mSceneIndex;
    }

private:
    WBDocumentProxy* mDocumentProxy;
    int mSceneIndex;
};

class WBDraggableThumbnail : public WBThumbnailProxyWidget
{
    Q_OBJECT
public:
    WBDraggableThumbnail(WBDocumentProxy* documentProxy, int index)
    : WBThumbnailProxyWidget(documentProxy, index)
    , mPageNumber(new WBThumbnailTextItem(index))
    , mEditable(false)
    {

    }

    ~WBDraggableThumbnail()
    {
        delete mPageNumber;
    }

    bool editable()
    {
        return mEditable;
    }

    bool deletable()
    {
        return documentProxy()->pageCount() > 1;
    }

    bool movableUp()
    {
        return sceneIndex() > 0;
    }

    bool movableDown()
    {
        return sceneIndex() < (documentProxy()->pageCount() -1);
    }

    void showUI()
    {
        setEditable(true);
    }

    void hideUI()
    {
        setEditable(false);
    }

    void setEditable(bool editable)
    {
        mEditable = editable;
    }


    WBThumbnailTextItem* pageNumber()
    {
        return mPageNumber;
    }

    void setPageNumber(int i)
    {
        mPageNumber->setPlainText(tr("Page %0").arg(i+1));

        if (WBApplication::boardController->activeSceneIndex() == i)
            mPageNumber->setHtml("<span style=\";font-weight:bold;color: red\">" + tr("Page %0").arg(i+1) + "</span>");
        else
            mPageNumber->setHtml("<span style=\";color: #000000\">" + tr("Page %0").arg(i+1) + "</span>");
    }

    virtual void updatePos(qreal w, qreal h);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    WBThumbnailTextItem* mPageNumber;
private:
    void deletePage();
    void duplicatePage();
    void moveUpPage();
    void moveDownPage();

    bool mEditable;
};

class WBDraggableThumbnailPixmap : public WBDraggableThumbnail
{
    Q_OBJECT
public:
    WBDraggableThumbnailPixmap(WBThumbnailPixmap* thumbnailPixmap, WBDocumentProxy* documentProxy, int index)
        : WBDraggableThumbnail(documentProxy, index)
        , mThumbnailPixmap(thumbnailPixmap)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setAcceptDrops(true);
    }

    ~WBDraggableThumbnailPixmap()
    {

    }

    WBThumbnailPixmap* thumbnailPixmap()
    {
        return mThumbnailPixmap;
    }

    void updatePos(qreal w, qreal h);

private:
    WBThumbnailPixmap* mThumbnailPixmap;
};

class WBDraggableThumbnailView : public WBDraggableThumbnail
{
    Q_OBJECT
public:
    WBDraggableThumbnailView(WBThumbnailView* thumbnailView, WBDocumentProxy* documentProxy, int index)
        : WBDraggableThumbnail(documentProxy, index)
        , mThumbnailView(thumbnailView)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setWidget(mThumbnailView);
        setAcceptDrops(true);            
    }

    ~WBDraggableThumbnailView()
    {
        delete mPageNumber;
    }

    WBThumbnailView* thumbnailView()
    {
        return mThumbnailView;
    }

    WBThumbnailTextItem* pageNumber()
    {
        return mPageNumber;
    }

    void setPageNumber(int i)
    {
        mPageNumber->setPlainText(tr("Page %0").arg(i+1));

        if (WBApplication::boardController->activeSceneIndex() == i)
            mPageNumber->setHtml("<span style=\";font-weight:bold;color: red\">" + tr("Page %0").arg(i+1) + "</span>");
        else
            mPageNumber->setHtml("<span style=\";color: #000000\">" + tr("Page %0").arg(i+1) + "</span>");
    }

private:        
    WBThumbnailView* mThumbnailView;
};

namespace WBThumbnailUI
{
    const int ICONSIZE      = 96;
    const int ICONSPACING   = 10;

    class WBThumbnailUIIcon : public QPixmap
    {
        public:
            WBThumbnailUIIcon(const QString& filename, int pos)
                : QPixmap(QSize(ICONSIZE, ICONSIZE))
                , mPos(pos)
            {
                QSvgRenderer svgRenderer(filename);
                QPainter painter;
                fill(Qt::transparent);
                painter.begin(this);
                svgRenderer.render(&painter);
                painter.end();
            }

            int pos() const
            {
                return mPos;
            }

            bool triggered(qreal x) const
            {
                using namespace WBThumbnailUI;
                return (x >= pos()*(ICONSIZE + ICONSPACING) && x <= (pos()+1)*ICONSIZE + pos()*ICONSPACING);
            }

        private:
            int mPos;
    };

    namespace _private
    {
        static QMap<QString, WBThumbnailUIIcon*> catalog;
        void initCatalog();
    }

    WBThumbnailUIIcon* addIcon(const QString& thumbnailIcon, int pos);
    WBThumbnailUIIcon* getIcon(const QString& thumbnailIcon);
    void draw(QPainter* painter, const WBThumbnailUIIcon& thumbnailIcon);
    bool triggered(qreal y);
}

#endif /* WBTHUMBNAILWIDGET_H_ */
