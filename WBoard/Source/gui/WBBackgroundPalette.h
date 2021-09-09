#ifndef WBBACKGROUNDPALETTE_H
#define WBBACKGROUNDPALETTE_H

#include "gui/WBActionPalette.h"
#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "domain/WBGraphicsScene.h"

class WBBackgroundPalette : public WBActionPalette
{
Q_OBJECT

public:
    WBBackgroundPalette(QList<QAction*> actions, QWidget* parent = 0);
    WBBackgroundPalette(QWidget* parent = 0);

    void addAction(QAction *action);
    void setActions(QList<QAction *> actions);
    void clearLayout();

public slots:
    void showEvent(QShowEvent* event);
    void backgroundChanged();
    void refresh();

protected slots:
    void sliderValueChanged(int value);
    void defaultBackgroundGridSize();

protected:
    virtual void updateLayout();
    void init();

    QVBoxLayout* mVLayout;
    QHBoxLayout* mTopLayout;
    QHBoxLayout* mBottomLayout;

    QSlider* mSlider;
    QLabel* mSliderLabel;
    WBActionPaletteButton* mResetDefaultGridSizeButton;

};

#endif // WBBACKGROUNDPALETTE_H
