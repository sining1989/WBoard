#ifndef WBZOOMPALETTE_H_
#define WBZOOMPALETTE_H_

#include "WBFloatingPalette.h"

class QPushButton;
class WBBoardController;

class WBZoomPalette : public WBFloatingPalette
{
    Q_OBJECT

public:
    WBZoomPalette(QWidget *parent);
    virtual ~WBZoomPalette();

public slots:
    void hide();
    void refreshPalette();

private:
    WBBoardController* mBoardController;
    QPushButton *mCurrentZoomButton;
    QPushButton *mHundredButton;
    QPushButton *mShowAllButton;

    bool mIsExpanded;

private slots:
    void showHideExtraButton();
    void goHundred();

};

#endif /* WBZOOMPALETTE_H_ */
