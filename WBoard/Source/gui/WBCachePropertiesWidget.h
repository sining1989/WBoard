#ifndef WBCACHEPROPERTIESWIDGET_H
#define WBCACHEPROPERTIESWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QVector>

#include "WBDockPaletteWidget.h"
#include "tools/WBGraphicsCache.h"

#define MAX_SHAPE_WIDTH 200

class WBCachePropertiesWidget : public WBDockPaletteWidget
{
    Q_OBJECT
public:
    WBCachePropertiesWidget(QWidget* parent=0, const char* name="WBCachePropertiesWidget");
    ~WBCachePropertiesWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

public slots:
    void updateCurrentCache();

private slots:
    void onCloseClicked();
    void updateCacheColor(QColor color);
    void onColorClicked();
    void updateShapeButtons();
    void onSizeChanged(int newSize);
    void onCacheEnabled();

private:
    QVBoxLayout* mpLayout;
    QLabel* mpCachePropertiesLabel;
    QLabel* mpColorLabel;
    QLabel* mpShapeLabel;
    QLabel* mpSizeLabel;
    QPushButton* mpColor;
    QPushButton* mpSquareButton;
    QPushButton* mpCircleButton;
    QPushButton* mpCloseButton;
    QSlider* mpSizeSlider;
    QHBoxLayout* mpColorLayout;
    QHBoxLayout* mpShapeLayout;
    QHBoxLayout* mpSizeLayout;
    QHBoxLayout* mpCloseLayout;
    QWidget* mpProperties;
    QVBoxLayout* mpPropertiesLayout;
    QColor mActualColor;
    eMaskShape mActualShape;
    WBGraphicsCache* mpCurrentCache;

};

#endif // WBCACHEPROPERTIESWIDGET_H
