#ifndef WBUNINOTESWINDOW_H_
#define WBUNINOTESWINDOW_H_
#include <QtWidgets>
#include <QShowEvent>
#include <QHideEvent>

#include "gui/WBActionPalette.h"
#include "gui/WBRightPalette.h"

class WBDesktopPalette : public WBActionPalette
{
    Q_OBJECT

    public:
        WBDesktopPalette(QWidget *parent, WBRightPalette* rightPalette);
        virtual ~WBDesktopPalette();

        void disappearForCapture();
        void appear();
        QPoint buttonPos(QAction* action);

    signals:
        void uniboardClick();
        void customClick();
        void windowClick();
        void screenClick();

//#ifdef Q_OS_LINUX //TODO: check why this produces an error on linux if uncommented
        void refreshMask();
//#endif

    public slots:
        void showHideClick(bool checked);
        void updateShowHideState(bool pShowEnabled);
        void setShowHideButtonVisible(bool visible);
        void setDisplaySelectButtonVisible(bool show);
        void minimizeMe(eMinimizedLocation location);
        void maximizeMe();
        void parentResized();

	protected:
        void showEvent(QShowEvent *event);
        void hideEvent(QHideEvent *event);

        virtual int getParentRightOffset();

	private:
        QAction *mShowHideAction;
        QAction *mDisplaySelectAction;
        QAction *mMaximizeAction;
        QAction *mActionUniboard;
        QAction *mActionCustomSelect;
        QAction* mActionTest;

        WBRightPalette* rightPalette;
        void adjustPosition();

	signals:
        void stylusToolChanged(int tool);

};

#endif /* WBUNINOTESWINDOW_H_ */
