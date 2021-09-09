#ifndef WBBOARDVIEW_H_
#define WBBOARDVIEW_H_

#define CONTROLVIEW_OBJ_NAME "ControlView"

#include <QtWidgets>
#include <QGraphicsView>
#include <QRubberBand>

#include "core/WB.h"
#include "domain/WBGraphicsDelegateFrame.h"

class WBBoardController;
class WBGraphicsScene;
class WBGraphicsWidgetItem;
class WBRubberBand;

class WBBoardView : public QGraphicsView
{
    Q_OBJECT

	public:
		WBBoardView(WBBoardController* pController, QWidget* pParent = 0, bool isControl = false, bool isDesktop = false);
		WBBoardView(WBBoardController* pController, int pStartLayer, int pEndLayer, QWidget* pParent = 0, bool isControl = false, bool isDesktop = false);
		virtual ~WBBoardView();

		WBGraphicsScene* scene();

		void forcedTabletRelease();

		void setToolCursor(int tool);

		void rubberItems();
		void moveRubberedItems(QPointF movingVector);

		void setMultiselection(bool enable);
		bool isMultipleSelectionEnabled() { return mMultipleSelectionIsEnabled; }

	#if defined(Q_OS_OSX)
		bool directTabletEvent(QEvent *event);
		QWidget *widgetForTabletEvent(QWidget *w, const QPoint &pos);
	#endif
	signals:
		void resized(QResizeEvent* event);
		void shown();
		void mouseReleased();

	protected:
		bool itemIsLocked(QGraphicsItem *item);
		bool isWBItem(QGraphicsItem *item);
		bool isCppTool(QGraphicsItem *item);
		void handleItemsSelection(QGraphicsItem *item);
		bool itemShouldReceiveMousePressEvent(QGraphicsItem *item);
		bool itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item);
		bool itemHaveParentWithType(QGraphicsItem *item, int type);
		bool itemShouldBeMoved(QGraphicsItem *item);
		QGraphicsItem* determineItemToPress(QGraphicsItem *item);
		QGraphicsItem* determineItemToMove(QGraphicsItem *item);
		void handleItemMousePress(QMouseEvent *event);
		void handleItemMouseMove(QMouseEvent *event);

		virtual bool event (QEvent * e);

		virtual void keyPressEvent(QKeyEvent *event);
		virtual void keyReleaseEvent(QKeyEvent *event);
		virtual void tabletEvent(QTabletEvent * event);
		virtual void mouseDoubleClickEvent(QMouseEvent *event);
		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);
		virtual void mouseReleaseEvent(QMouseEvent *event);
		virtual void wheelEvent(QWheelEvent *event);
		virtual void leaveEvent ( QEvent * event);

		virtual void focusOutEvent ( QFocusEvent * event );

		virtual void drawItems(QPainter *painter, int numItems,
							   QGraphicsItem *items[],
							   const QStyleOptionGraphicsItem options[]);

		virtual void dropEvent(QDropEvent *event);
		virtual void dragMoveEvent(QDragMoveEvent *event);

		virtual void resizeEvent(QResizeEvent * event);

		virtual void drawBackground(QPainter *painter, const QRectF &rect);

	private:
		void init();

		inline bool shouldDisplayItem(QGraphicsItem *item)
		{
			bool ok;
			int itemLayerType = item->data(WBGraphicsItemData::ItemLayerType).toInt(&ok);
			return (ok && (itemLayerType >= mStartLayer && itemLayerType <= mEndLayer));
		}

		QList<QUrl> processMimeData(const QMimeData* pMimeData);

		WBBoardController* mController;

		int mStartLayer, mEndLayer;
		bool mFilterZIndex;

		bool mTabletStylusIsPressed;
		bool mUsingTabletEraser;

		bool mPendingStylusReleaseEvent;

		bool mMouseButtonIsPressed;
		QPointF mPreviousPoint;
		QPoint mMouseDownPos;

		bool mPenPressureSensitive;
		bool mMarkerPressureSensitive;
		bool mUseHighResTabletEvent;

		QRubberBand *mRubberBand;
		bool mIsCreatingTextZone;
		bool mIsCreatingSceneGrabZone;

		bool isAbsurdPoint(QPoint point);

		bool mVirtualKeyboardActive;
		bool mOkOnWidget;

		bool mWidgetMoved;
		QPointF mLastPressedMousePos;
		QGraphicsItem *movingItem;
		QMouseEvent *suspendedMousePressEvent;

		bool moveRubberBand;
		WBRubberBand *mUBRubberBand;

		QList<QGraphicsItem *> mRubberedItems;
		QSet<QGraphicsItem*> mJustSelectedItems;

		int mLongPressInterval;
		QTimer mLongPressTimer;

		bool mIsDragInProgress;
		bool mMultipleSelectionIsEnabled;
		bool bIsControl;
		bool bIsDesktop;
		bool mRubberBandInPlayMode;

		static bool hasSelectedParents(QGraphicsItem * item);

	private slots:
		void settingChanged(QVariant newValue);

	public slots:
		void virtualKeyboardActivated(bool b);
		void longPressEvent();

};

#endif /* WBBOARDVIEW_H_ */
