#ifndef WBDOCKPALETTEWIDGET_H
#define WBDOCKPALETTEWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QString>

typedef enum
{
    eWBDockPaletteWidget_BOARD,
    eWBDockPaletteWidget_WEB,
    eWBDockPaletteWidget_DOCUMENT,
    eWBDockPaletteWidget_DESKTOP,
} eWBDockPaletteWidgetMode;

class WBDockPaletteWidget : public QWidget
{
    Q_OBJECT
public:
    WBDockPaletteWidget(QWidget* parent=0, const char* name="WBDockPaletteWidget");
    ~WBDockPaletteWidget();

    QPixmap iconToRight();
    QPixmap iconToLeft();
    QString name();

    virtual bool visibleInMode(eWBDockPaletteWidgetMode mode) = 0;

    void registerMode(eWBDockPaletteWidgetMode mode);

    bool visibleState(){return mVisibleState;}
    void setVisibleState(bool state){mVisibleState = state;}

signals:
    void hideTab(WBDockPaletteWidget* widget);
    void showTab(WBDockPaletteWidget* widget);

public slots:
    void slot_changeMode(eWBDockPaletteWidgetMode newMode);

protected:
    QPixmap mIconToRight;
    QPixmap mIconToLeft;
    QString mName;

    QVector<eWBDockPaletteWidgetMode> mRegisteredModes;

    bool mVisibleState;
};

#endif // WBDOCKPALETTEWIDGET_H
