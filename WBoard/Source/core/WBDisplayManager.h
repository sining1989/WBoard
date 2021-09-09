#ifndef WBDISPLAYMANAGER_H_
#define WBDISPLAYMANAGER_H_

#include <QtWidgets>

class WBBlackoutWidget;
class WBBoardView;
class QDesktopWidget;

class WBDisplayManager : public QObject
{
    Q_OBJECT

    public:
        WBDisplayManager(QObject *parent = 0);
        virtual ~WBDisplayManager();

        int numScreens();

        int numPreviousViews();

        void setControlWidget(QWidget* pControlWidget);

        void setDisplayWidget(QWidget* pDisplayWidget);

        void setDesktopWidget(QWidget* pControlWidget);

        void setPreviousDisplaysWidgets(QList<WBBoardView*> pPreviousViews);

        bool hasControl()
        {
            return mControlScreenIndex > -1;
        }

        bool hasDisplay()
        {
            return mDisplayScreenIndex > -1;
        }

        bool hasPrevious()
        {
            return !mPreviousScreenIndexes.isEmpty();
        }

        enum DisplayRole
        {
            None = 0, Control, Display, Previous1, Previous2, Previous3, Previous4, Previous5
        };

        bool useMultiScreen() { return mUseMultiScreen; }

        void setUseMultiScreen(bool pUse);

        int controleScreenIndex()
        {
            return mControlScreenIndex;
        }

        QRect controlGeometry();
        QRect displayGeometry();

   signals:
        void screenLayoutChanged();
        void adjustDisplayViewsRequired();

   public slots:
        void reinitScreens(bool bswap);

        void adjustScreens(int screen);

        void blackout();

        void unBlackout();

        void setRoleToScreen(DisplayRole role, int screenIndex);

        void swapDisplayScreens(bool swap);

    private:
        void positionScreens();

        void initScreenIndexes();

        int mControlScreenIndex;

        int mDisplayScreenIndex;

        QList<int> mPreviousScreenIndexes;

        QDesktopWidget* mDesktop;

        QWidget* mControlWidget;

        QWidget* mDisplayWidget;

        QWidget *mDesktopWidget;

        QList<WBBoardView*> mPreviousDisplayWidgets;

        QList<WBBlackoutWidget*> mBlackoutWidgets;

        QList<DisplayRole> mScreenIndexesRoles;

        bool mUseMultiScreen;

};

#endif /* WBDISPLAYMANAGER_H_ */
