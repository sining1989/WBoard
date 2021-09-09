#ifndef WBOPENSANKOREIMPORTERWIDGET_H
#define WBOPENSANKOREIMPORTERWIDGET_H

class QCheckBox;
class QPushButton;

#include "WBFloatingPalette.h"

class WBOpenSankoreImporterWidget : public WBFloatingPalette
{
    Q_OBJECT

public:
    WBOpenSankoreImporterWidget(QWidget* parent);
    QPushButton* proceedButton(){return mProceedButton;}

protected:
    void showEvent(QShowEvent *event);
    int border();

    QCheckBox* mDisplayOnNextRestart;
    QPushButton* mProceedButton;

private slots:
    void onNextRestartCheckBoxClicked(bool clicked);

};

#endif // WBOPENSANKOREIMPORTERWIDGET_H
