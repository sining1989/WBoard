#ifndef WBSTARTUPTIPSPALETTE_H
#define WBSTARTUPTIPSPALETTE_H

class QCheckBox;
class QVBoxLayout;
class QHBoxLayout;

#include <QWebEngineView>

#include "WBFloatingPalette.h"
#include "api/WBWidgetUniboardAPI.h"

class WBStartupHintsPalette : public WBFloatingPalette
{
    Q_OBJECT
public:
    WBStartupHintsPalette(QWidget *parent = 0);
    ~WBStartupHintsPalette();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void showEvent(QShowEvent *event);

private:
    void close();
    int border();
    QCheckBox* mShowNextTime;
    QVBoxLayout* mLayout;
    QHBoxLayout* mButtonLayout;
    WBWidgetUniboardAPI *mpSankoreAPI;
	QWebEngineView* mpWebView;

private slots:
    void onShowNextTimeStateChanged(int state);
    void javaScriptWindowObjectCleared();
};

#endif // WBSTARTUPTIPSPALETTE_H
