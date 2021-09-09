#ifndef WBDOCKPALETTE_H
#define WBDOCKPALETTE_H

class WBDocumentProxy;

#include <QWidget>
#include <QMouseEvent>
#include <QBrush>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QTime>
#include <QPoint>
#include <QPixmap>
#include <QMap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVector>

#include "WBDockPaletteWidget.h"

#define TABSIZE 50    
#define CLICKTIME 1000000

/**
 * \brief The dock positions
 */
typedef enum
{
    eWBDockOrientation_Left, 
    eWBDockOrientation_Right,
    eWBDockOrientation_Top,
    eWBDockOrientation_Bottom
}eWBDockOrientation;

typedef enum
{
    eWBDockTabOrientation_Up,
    eWBDockTabOrientation_Down
}eWBDockTabOrientation;

class WBDockPalette;

class WBTabDockPalette : public QWidget
{
    Q_OBJECT
    friend class WBDockPalette;

public:
    WBTabDockPalette(WBDockPalette *dockPalette, QWidget *parent = 0);
    ~WBTabDockPalette();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    WBDockPalette *dock;
    int mVerticalOffset;
    bool mFlotable;
};

typedef enum
{
    eWBDockPaletteType_LEFT,
    eWBDockPaletteType_RIGHT
} eWBDockPaletteType;


class WBDockPalette : public QWidget
{
    Q_OBJECT
    friend class WBTabDockPalette;

public:
    WBDockPalette(eWBDockPaletteType paletteType, QWidget* parent=0, const char* name="WBDockPalette");
    ~WBDockPalette();

    eWBDockOrientation orientation();
    void setOrientation(eWBDockOrientation orientation);
    void setTabsOrientation(eWBDockTabOrientation orientation);
    void showTabWidget(int tabIndex);
    QRect getTabPaletteRect();

    virtual void assignParent(QWidget *widget);
    virtual void setVisible(bool visible);

    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    void setBackgroundBrush(const QBrush& brush);
    void registerWidget(WBDockPaletteWidget* widget);

    void addTab(WBDockPaletteWidget* widget);
    void removeTab(WBDockPaletteWidget* widget);

    void connectSignals();

    bool switchMode(eWBDockPaletteWidgetMode mode);

    eWBDockPaletteWidgetMode mCurrentMode;

    QVector<WBDockPaletteWidget*> GetWidgetsList() { return mRegisteredWidgets; }

public:
    bool isTabFlotable() {return mTabPalette->mFlotable;}
    void setTabFlotable(bool newFlotable) {mTabPalette->mFlotable = newFlotable;}
    int getAdditionalVOffset() const {return mTabPalette->mVerticalOffset;}
    void setAdditionalVOffset(int newOffset) {mTabPalette->mVerticalOffset = newOffset;}

    eWBDockPaletteType paletteType(){return mPaletteType;}

public slots:
    void onShowTabWidget(WBDockPaletteWidget* widget);
    void onHideTabWidget(WBDockPaletteWidget* widget);
    void onAllDownloadsFinished();
    virtual void onDocumentSet(WBDocumentProxy* documentProxy);

signals:
    void mouseEntered();
    void pageSelectionChangedRequired();

protected:
    virtual int border();
    virtual int radius();
    virtual int customMargin();
    virtual void updateMaxWidth();
    virtual void resizeEvent(QResizeEvent *event);
    virtual int collapseWidth();

    /** The current dock orientation */
    eWBDockOrientation mOrientation;
    /** The current background brush */
    QBrush mBackgroundBrush;
    /** The preferred width */
    int mPreferredWidth;
    /** The preferred height */
    int mPreferredHeight;
    /** A flag used to allow the resize */
    bool mCanResize;
    /** A flag indicating if the palette has been resized between a click and a release */
    bool mResized;
    /** The width that trig the collapse */
    int mCollapseWidth;
    /** The last width of the palette */
    int mLastWidth;
    /** The click time*/
    QTime mClickTime;
    /** The mouse pressed position */
    QPoint mMousePressPos;
    /** The tab orientation */
    eWBDockTabOrientation mTabsOrientation;
    /** The h position of the tab */
    int mHTab;
    /** The stacked widget */
    QStackedWidget* mpStackWidget;
    /** The layout */
    QVBoxLayout* mpLayout;
    /** The current tab index */
    int mCurrentTab;
    /** The visible tab widgets */
    QVector<WBDockPaletteWidget*> mTabWidgets;
    /** The current widget */
    QVector<WBDockPaletteWidget*> mRegisteredWidgets;
    /** The current tab widget */
    QString mCrntTabWidget;
    /** Last opened tab index depending on mode */
    QMap<eWBDockPaletteWidgetMode,int> mLastOpenedTabForMode;

private slots:
    void onToolbarPosUpdated();
    void onResizeRequest(QResizeEvent* event);

private:
    void tabClicked(int tabIndex);
    int tabSpacing();
    void toggleCollapseExpand();
    void moveTabs();
    void resizeTabs();

private:
    eWBDockPaletteType mPaletteType;
    WBTabDockPalette *mTabPalette;
};

#endif // WBDOCKPALETTE_H
