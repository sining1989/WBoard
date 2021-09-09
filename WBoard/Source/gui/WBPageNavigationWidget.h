#ifndef WBPAGENAVIGATIONWIDGET_H
#define WBPAGENAVIGATIONWIDGET_H

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QLabel>
#include <QString>

#include "WBBoardThumbnailsView.h"
#include "WBDocumentNavigator.h"
#include "WBDockPaletteWidget.h"
#include "document/WBDocumentProxy.h"

class WBPageNavigationWidget : public WBDockPaletteWidget
{
    Q_OBJECT
public:
    WBPageNavigationWidget(QWidget* parent=0, const char* name="WBPageNavigationWidget");
    ~WBPageNavigationWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

signals:
    void resizeRequest(QResizeEvent* event);

public slots:
    void setPageNumber(int current, int total);

protected:
    virtual void timerEvent(QTimerEvent *event);


private:
    void updateTime();
    int customMargin();
    int border();

    WBDocumentNavigator* mNavigator;
    QVBoxLayout* mLayout;
    QHBoxLayout* mHLayout;
    QLabel* mPageNbr;
    QLabel* mClock;
    QString mTimeFormat;
    int mTimerID;

};

#endif // WBPAGENAVIGATIONWIDGET_H
