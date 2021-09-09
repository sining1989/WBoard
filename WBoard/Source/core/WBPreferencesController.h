#ifndef WBPREFERENCESCONTROLLER_H_
#define WBPREFERENCESCONTROLLER_H_

#include <QtWidgets>
#include <QDialog>
#include "ui_brushProperties.h"

class WBColorPicker;
class WBApplication;
class WBSettings;
class WBPreferencesController;
class WBBrushPropertiesFrame;

namespace Ui
{
	class preferencesDialog;
}

class WBPreferencesDialog : public QDialog
{
    Q_OBJECT;

public:
    WBPreferencesDialog(WBPreferencesController* prefController, QWidget* parent = 0,Qt::WindowFlags f = 0 );
    ~WBPreferencesDialog();

protected:
    void closeEvent(QCloseEvent* e);
    WBPreferencesController *mPreferencesController;
};

class WBPreferencesController : public QObject
{
    Q_OBJECT

public:
    WBPreferencesController(QWidget *parent);
    virtual ~WBPreferencesController();

public slots:
    void show();

protected:
    void wire();
    void init();

    WBPreferencesDialog* mPreferencesWindow;
    Ui::preferencesDialog* mPreferencesUI;
    WBBrushPropertiesFrame* mPenProperties;
    WBBrushPropertiesFrame* mMarkerProperties;
    WBColorPicker* mDarkBackgroundGridColorPicker;
    WBColorPicker* mLightBackgroundGridColorPicker;

protected slots:
    void close();
    void defaultSettings();
    void penPreviewFromSizeChanged(int value);
    void darkBackgroundCrossOpacityValueChanged(int value);
    void lightBackgroundCrossOpacityValueChanged(int value);
    void widthSliderChanged(int value);
    void opacitySliderChanged(int value);
    void colorSelected(const QColor&);
    void setCrossColorOnDarkBackground(const QColor& color);
    void setCrossColorOnLightBackground(const QColor& color);
    void toolbarPositionChanged(bool checked);
    void toolbarOrientationVertical(bool checked);
    void toolbarOrientationHorizontal(bool checked);
    void systemOSKCheckBoxToggled(bool checked);

private slots:
    void adjustScreens(int screen);

private:
    static qreal sSliderRatio;
    static qreal sMinPenWidth;
    static qreal sMaxPenWidth;
    QDesktopWidget* mDesktop;

};

class WBBrushPropertiesFrame : public Ui::brushProperties
{
public:
    WBBrushPropertiesFrame(QFrame* owner, const QList<QColor>& lightBackgroundColors,const QList<QColor>& darkBackgroundColors, const QList<QColor>& lightBackgroundSelectedColors,const QList<QColor>& darkBackgroundSelectedColors, WBPreferencesController* controller);

    virtual ~WBBrushPropertiesFrame(){}

    QList<WBColorPicker*> lightBackgroundColorPickers;
    QList<WBColorPicker*> darkBackgroundColorPickers;

};

#endif /* WBPREFERENCESCONTROLLER_H_ */
