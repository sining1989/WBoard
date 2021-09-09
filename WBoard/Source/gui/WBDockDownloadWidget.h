#ifndef WBDOCKDOWNLOADWIDGET_H
#define WBDOCKDOWNLOADWIDGET_H

#include <QWidget>
#include <QVBoxLayout>

#include "WBDockPaletteWidget.h"
#include "WBDownloadWidget.h"

class WBDockDownloadWidget : public WBDockPaletteWidget
{
    Q_OBJECT
public:
    WBDockDownloadWidget(QWidget* parent=0, const char* name="WBDockDownloadWidget");
    ~WBDockDownloadWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

private:
    QVBoxLayout* mpLayout;
    WBDownloadWidget* mpDLWidget;
};

#endif // WBDOCKDOWNLOADWIDGET_H
