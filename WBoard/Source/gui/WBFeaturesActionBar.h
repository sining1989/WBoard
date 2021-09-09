#ifndef WBFEATURESACTIONBAR_H
#define WBFEATURESACTIONBAR_H

#include <QWidget>
#include <QToolButton>
#include <QDropEvent>
#include <QLineEdit>
#include <QLayout>

#include "board/WBFeaturesController.h"

#define BUTTON_SIZE 24
#define ACTIONBAR_HEIGHT 42

class WBFeaturesMimeData;

typedef enum
{
    eButtonSet_Default,
    eButtonSet_Properties,
    eButtonSet_Favorite
} eButtonSet;

class WBActionButton : public QToolButton
{
public:
    WBActionButton(QWidget* parent=0, QAction* action=0, const char* name="WBActionButton");
    ~WBActionButton();
};

enum WBFeaturesActionBarState
{
    IN_ROOT,
    IN_FOLDER,
    IN_PROPERTIES,
    IN_FAVORITE,
    IN_TRASH
};

class WBFeaturesActionBar : public QWidget
{
    Q_OBJECT
public:
    WBFeaturesActionBar(WBFeaturesController *controller, QWidget* parent=0, const char* name="WBFeaturesActionBar");
    ~WBFeaturesActionBar();
    
    void setCurrentState( WBFeaturesActionBarState state );
    void cleanText(){ mSearchBar->clear(); }

signals:
    void searchElement(const QString &text);
    void newFolderToCreate();
    void deleteElements(const WBFeaturesMimeData *data);
    void addToFavorite(const WBFeaturesMimeData *data);
    void removeFromFavorite(const WBFeaturesMimeData *data);
    void addElementsToFavorite();
    void removeElementsFromFavorite();
    void deleteSelectedElements();
    void rescanModel();

private slots:
    void onSearchTextChanged(QString txt);
    void onActionNewFolder();
    void onActionFavorite();
    void onActionRemoveFavorite();
    void onActionTrash();
    void onActionRescanModel();
    void lockIt();
    void unlockIt();

protected:
    void dragEnterEvent( QDragEnterEvent *event );
    void dropEvent( QDropEvent *event );

private:
    void setButtons();
    WBFeaturesController *featuresController;
    WBFeaturesActionBarState currentState;

    eButtonSet mCrntButtonSet;
    eButtonSet mPreviousButtonSet;

    QButtonGroup* mButtonGroup;
    QLineEdit* mSearchBar;
    QHBoxLayout* mLayout;
    QAction* mpFavoriteAction;
    QAction* mpSocialAction;
    QAction* mpRescanModelAction;
    QAction* mpDeleteAction;
    QAction* mpSearchAction;
    QAction* mpCloseAction;
    QAction* mpRemoveFavorite;
    QAction* mpNewFolderAction;
    WBActionButton* mpFavoriteBtn;
    WBActionButton* mpSocialBtn;
    WBActionButton* mpRescanModelBtn;
    WBActionButton* mpDeleteBtn;
    WBActionButton* mpCloseBtn;
    WBActionButton* mpRemoveFavoriteBtn;
    WBActionButton* mpNewFolderBtn;
};

#endif
