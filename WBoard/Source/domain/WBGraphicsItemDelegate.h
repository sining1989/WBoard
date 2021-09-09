#ifndef WBGRAPHICSITEMDELEGATE_H_
#define WBGRAPHICSITEMDELEGATE_H_

#define WB_FREE_CONTROL(Object, Scene) \
    if (Object) {                      \
        if (Scene) {                   \
            Scene->removeItem(Object); \
        }                              \
        Object->deleteLater();         \
        Object = 0;                    \
    }                                  \

#include <QtWidgets>
#include <QtSvg>
#include <QMimeData>
#include <QGraphicsVideoItem>

#include "core/WB.h"
#include "core/WBSettings.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class WBGraphicsScene;
class WBGraphicsProxyWidget;
class WBGraphicsDelegateFrame;
class WBGraphicsWidgetItem;
class WBGraphicsMediaItem;

class DelegateButton: public QGraphicsSvgItem
{
    Q_OBJECT

    public:
        DelegateButton(const QString & fileName, QGraphicsItem* pDelegated, QGraphicsItem * parent = 0, Qt::WindowFrameSection section = Qt::TopLeftSection);

        virtual ~DelegateButton();

        enum { Type = WBGraphicsItemType::DelegateButtonType };
        virtual int type() const { return Type; }

        void setTransparentToMouseEvent(bool tr)
        {
            mIsTransparentToMouseEvent = tr;
        }

        void setFileName(const QString & fileName);

        void setShowProgressIndicator(bool pShow) {mShowProgressIndicator = pShow;}
        bool testShowProgresIndicator() const {return mShowProgressIndicator;}

        void setSection(Qt::WindowFrameSection section) {mButtonAlignmentSection =  section;}
        Qt::WindowFrameSection getSection() const {return mButtonAlignmentSection;}

    protected:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
        void timerEvent(QTimerEvent *event);

        void modified();

	private slots:
        void startShowProgress();

    private:
        QGraphicsItem* mDelegated;

        QTime mPressedTime;
        bool mIsTransparentToMouseEvent;
        bool mIsPressed;
        int mProgressTimerId;
        int mPressProgres;
        bool mShowProgressIndicator;
        Qt::WindowFrameSection mButtonAlignmentSection;

    signals:
        void clicked (bool checked = false);
        void longClicked();

};

class MediaTimer: public QGraphicsRectItem
{
public:
    MediaTimer(QGraphicsItem * parent = 0);
    ~MediaTimer();

    void positionHandles();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget);

    void display(const QString &str);
    void setNumDigits(int nDigits);

private:
    static const char* getSegments(char);
    void drawString(const QString& s, QPainter &, QBitArray * = 0, bool = true);
    void drawDigit(const QPoint &, QPainter &, int, char, char = ' ');
    void drawSegment(const QPoint &, char, QPainter &, int, bool = false);
    void addPoint(QPolygon&, const QPoint&);
    void internalSetString(const QString& s);

    static char segments [][8];

    int ndigits;
    QString digitStr;
    QBitArray points;
    double val;

    uint shadow : 1;
    uint smallPoint : 1;

    int digitSpace;
    int xSegLen;
    int ySegLen;
    int segLen;
    int xAdvance;
    int xOffset;
    int yOffset;
};

class DelegateMediaControl: public QObject, public QGraphicsRectItem
{
    Q_OBJECT

    public:
        DelegateMediaControl(WBGraphicsMediaItem* pDelegated, QGraphicsItem * parent = 0);

        virtual ~DelegateMediaControl()
        {

		}

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget);

        QPainterPath shape() const;

        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        virtual void update();

        void positionHandles();
        void updateTicker(qint64 time);
        void totalTimeChanged(qint64 newTotalTime);

    signals:
        void used();

    protected:
        void seekToMousePos(QPointF mousePos);

        WBGraphicsMediaItem* mDelegate;
        bool mDisplayCurrentTime;

        qint64 mCurrentTimeInMs;
        qint64 mTotalTimeInMs;

    private:
        int mStartWidth;
        int mSeecAreaBorderHeight;

        QRectF mSeecArea;
        QRectF mLCDTimerArea;

        MediaTimer *lcdTimer;

        QString mDisplayFormat;
};

class WBGraphicsToolBarItem : public QObject, public QGraphicsRectItem 
{
    public:
        WBGraphicsToolBarItem(QGraphicsItem * parent = 0);
        virtual ~WBGraphicsToolBarItem() {;}

        bool isVisibleOnBoard() const { return mVisible; }
        void setVisibleOnBoard(bool visible) { mVisible = visible; }
        bool isShifting() const { return mShifting; }
        void setShifting(bool shifting) { mShifting = shifting; }
        QList<QGraphicsItem*> itemsOnToolBar() const { return mItemsOnToolBar; }
        void setItemsOnToolBar(QList<QGraphicsItem*> itemsOnToolBar) { mItemsOnToolBar = itemsOnToolBar;}
        int minWidth() { return mMinWidth; }
        void positionHandles();
        void update();
        int getElementsPadding(){return mElementsPadding;}

    private:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                QWidget *widget);

    private:
        bool mShifting;
        bool mVisible;
        int mMinWidth;
        int mInitialHeight;
        int mElementsPadding;
        QList<QGraphicsItem*> mItemsOnToolBar;
};

class WBGraphicsItemDelegate : public QObject
{
    Q_OBJECT

    public:
		WBGraphicsItemDelegate(QGraphicsItem* pDelegated, QObject * parent = 0, WBGraphicsFlags fls = 0);

        virtual ~WBGraphicsItemDelegate();

        virtual void createControls();
        virtual void freeControls();
        virtual void showControls();
        virtual bool controlsExist() const;

        virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        virtual bool wheelEvent(QGraphicsSceneWheelEvent *event);

        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

        virtual bool keyPressEvent(QKeyEvent *event);
        virtual bool keyReleaseEvent(QKeyEvent *event);

        virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change,
                const QVariant &value);
        virtual WBGraphicsScene *castUBGraphicsScene();
        virtual void postpaint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        void printMessage(const QString &mess) {qDebug() << mess;}

		QGraphicsItem* delegated();

        virtual void positionHandles();
        void setZOrderButtonsVisible(bool visible);

        void startUndoStep();
        void commitUndoStep();

        WBGraphicsDelegateFrame* frame() {return mFrame;}

        bool isLocked() const;

        QMimeData* mimeData(){ return mMimeData; }
        void setMimeData(QMimeData* mimeData);
        void setDragPixmap(const QPixmap &pix) {mDragPixmap = pix;}

        void setLocked(bool pLocked);
        void setButtonsVisible(bool visible);

        WBGraphicsToolBarItem* getToolBarItem() const { return mToolBarItem; }

        qreal antiScaleRatio() const { return mAntiScaleRatio; }
        virtual void update() {positionHandles();}

        WBGraphicsFlags ubflags() const {return mFlags;}
        bool testUBFlags(WBGraphicsFlags pf) const {return mFlags & pf;}
        void setUBFlags(WBGraphicsFlags pf);
        void setUBFlag(WBGraphicsFlags pf, bool set = true);

        virtual void showToolBar(bool autohide = true) {Q_UNUSED(autohide);}

    signals:
        void showOnDisplayChanged(bool shown);
        void lockChanged(bool locked);

    public slots:
        virtual void remove(bool canUndo = true);
        void showMenu();

        virtual void showHide(bool show);
        virtual void lock(bool lock);
        virtual void duplicate();

        void increaseZLevelUp();
        void increaseZLevelDown();
        void increaseZlevelTop();
        void increaseZlevelBottom();

        void onZoomChanged();

    protected:
        virtual void buildButtons();
        virtual void freeButtons();
        virtual void decorateMenu(QMenu *menu);
        virtual void updateMenuActionState();

        void showHideRecurs(const QVariant &pShow, QGraphicsItem *pItem);
        void setLockedRecurs(const QVariant &pLock, QGraphicsItem *pItem);

        QList<DelegateButton*> buttons() {return mButtons;}
        QGraphicsItem* mDelegated;

        //buttons from the top left section of delegate frame
        DelegateButton* mDeleteButton;
        DelegateButton* mDuplicateButton;
        DelegateButton* mMenuButton;

        //buttons from the bottom left section of delegate frame
        DelegateButton *mZOrderUpButton;
        DelegateButton *mZOrderDownButton;

        QMenu* mMenu;

        QAction* mLockAction;
        QAction* mShowOnDisplayAction;
        QAction* mSetAsBackgroundAction;
        QAction* mGotoContentSourceAction;

        WBGraphicsDelegateFrame* mFrame;
        qreal mFrameWidth;
        qreal mAntiScaleRatio;

        QList<DelegateButton*> mButtons;
        QList<DelegateButton*> mToolBarButtons;
        WBGraphicsToolBarItem* mToolBarItem;

	protected slots:
        virtual void setAsBackground();
        virtual void gotoContentSource();

	private:
        void updateFrame();
        void updateButtons(bool showUpdated = false);

        QPointF mOffset;
        QTransform mPreviousTransform;
        QPointF mPreviousPosition;
        QPointF mDragStartPosition;
        qreal mPreviousZValue;
        QSizeF mPreviousSize;
        QMimeData* mMimeData;
        QPixmap mDragPixmap;

        bool mMoved;
        WBGraphicsFlags mFlags;
};

#endif /* WBGRAPHICSITEMDELEGATE_H_ */
