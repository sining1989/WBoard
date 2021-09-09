#include <QButtonGroup>
#include <QAction>

#include "WBFeaturesActionBar.h"
#include "core/memcheck.h"
#include "gui/WBFeaturesWidget.h"

WBFeaturesActionBar::WBFeaturesActionBar( WBFeaturesController *controller, QWidget* parent, const char* name ) : QWidget (parent)
    , featuresController(controller)
    , mButtonGroup(NULL)
    , mSearchBar(NULL)
    , mLayout(NULL)
    , mpFavoriteAction(NULL)
    , mpSocialAction(NULL)
    , mpRescanModelAction(NULL)
    , mpDeleteAction(NULL)
    , mpSearchAction(NULL)
    , mpCloseAction(NULL)
    , mpRemoveFavorite(NULL)
    , mpNewFolderAction(NULL)
    , mpFavoriteBtn(NULL)
    , mpSocialBtn(NULL)
    , mpRescanModelBtn(NULL)
    , mpDeleteBtn(NULL)
    , mpCloseBtn(NULL)
    , mpRemoveFavoriteBtn(NULL)
    , mpNewFolderBtn(NULL)
{
    setObjectName(name);
    setStyleSheet(QString("background: #EEEEEE; border-radius : 10px; border : 2px solid #999999;"));

    setAcceptDrops(true);

    mButtonGroup = new QButtonGroup(this);
    mSearchBar = new QLineEdit(this);
    mSearchBar->setStyleSheet(QString("background-color:white; border-radius : 10px; padding : 2px;"));

    mLayout = new QHBoxLayout();
    setLayout(mLayout);

    setMaximumHeight(ACTIONBAR_HEIGHT);

    // Create the actions
    mpFavoriteAction = new QAction(QIcon(":/images/libpalette/miniFavorite.png"), tr("Add to favorites"), this);
    mpSocialAction = new QAction(QIcon(":/images/libpalette/social.png"), tr("Share"), this);
    mpSearchAction = new QAction(QIcon(":/images/libpalette/miniSearch.png"), tr("Search"), this);
    mpRescanModelAction = new QAction(QIcon(":/images/cursors/rotate.png"), tr("Rescan file system"), this);
    mpDeleteAction = new QAction(QIcon(":/images/libpalette/miniTrash.png"), tr("Delete"), this);
    mpCloseAction = new QAction(QIcon(":/images/close.svg"), tr("Back to folder"), this);
    mpRemoveFavorite = new QAction(QIcon(":/images/libpalette/trash_favorite.svg"), tr("Remove from favorites"), this);
    mpNewFolderAction = new QAction(QIcon(":/images/libpalette/miniNewFolder.png"), tr("Create new folder"), this);

    // Create the buttons
    mpFavoriteBtn = new WBActionButton(this, mpFavoriteAction);
    mpSocialBtn = new WBActionButton(this, mpSocialAction);

    //mpSearchBtn = new WBActionButton(this, mpSearchAction);
    mpRescanModelBtn = new WBActionButton(this, mpRescanModelAction);
    mpRescanModelBtn->hide();

    mpDeleteBtn = new WBActionButton(this, mpDeleteAction);
    mpCloseBtn = new WBActionButton(this, mpCloseAction);
    mpRemoveFavoriteBtn = new WBActionButton(this, mpRemoveFavorite);
    mpNewFolderBtn = new WBActionButton(this, mpNewFolderAction);

    // Initialize the buttons
    //mpSearchBtn->setEnabled(false);
    mpNewFolderBtn->setEnabled(false);

    // Add the buttons to the button group
    mButtonGroup->addButton(mpFavoriteBtn);
    mButtonGroup->addButton(mpSocialBtn);
    //mButtonGroup->addButton(mpSearchBtn);
    mButtonGroup->addButton(mpDeleteBtn);
    mButtonGroup->addButton(mpCloseBtn);
    mButtonGroup->addButton(mpRemoveFavoriteBtn);
    mButtonGroup->addButton(mpNewFolderBtn);
    // Connect signals & slots
    /*connect(mpFavoriteAction,SIGNAL(triggered()), this, SLOT(onActionFavorite()));
    connect(mpSocialAction,SIGNAL(triggered()), this, SLOT(onActionSocial()));
    connect(mpSearchAction,SIGNAL(triggered()), this, SLOT(onActionSearch()));
    connect(mpDeleteAction,SIGNAL(triggered()), this, SLOT(onActionTrash()));
    connect(mpCloseAction, SIGNAL(triggered()), this, SLOT(onActionClose()));
    connect(mpRemoveFavorite, SIGNAL(triggered()), this, SLOT(onActionRemoveFavorite()));
    connect(mSearchBar, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(mpNewFolderAction, SIGNAL(triggered()), this, SLOT(onActionNewFolder()));*/

    connect(mpFavoriteAction,SIGNAL(triggered()), this, SLOT(onActionFavorite()));
    connect(mSearchBar, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(mpNewFolderAction, SIGNAL(triggered()), this, SLOT(onActionNewFolder()));
    connect(mpRemoveFavorite, SIGNAL(triggered()), this, SLOT(onActionRemoveFavorite()));
    connect(mpRescanModelAction, SIGNAL(triggered()), this , SLOT(onActionRescanModel()));
    connect(mpDeleteAction,SIGNAL(triggered()), this, SLOT(onActionTrash()));


    // Build the default toolbar
    mLayout->addWidget(mpFavoriteBtn);
    mLayout->addWidget(mpSocialBtn);
    mLayout->addWidget(mpNewFolderBtn);
    mLayout->addWidget(mSearchBar);
    //mLayout->addWidget(mpSearchBtn);
    mLayout->addWidget(mpRescanModelBtn);
    mLayout->addWidget(mpDeleteBtn);
    mLayout->addWidget(mpCloseBtn);
    mLayout->addWidget(mpRemoveFavoriteBtn);
    setCurrentState( IN_ROOT );
    mpDeleteBtn->setAcceptDrops(true);
    setAcceptDrops( true );
}

void WBFeaturesActionBar::setCurrentState( WBFeaturesActionBarState state )
{
    currentState = state;
    setButtons();
}

void WBFeaturesActionBar::setButtons()
{
    switch( currentState )
    {
    case IN_FOLDER:
        mpFavoriteBtn->show();
        mpSocialBtn->hide();
        mSearchBar->show();
        mpDeleteBtn->show();
        mpCloseBtn->hide();
        mpRemoveFavoriteBtn->hide();
        mpNewFolderBtn->show();
        mpNewFolderBtn->setEnabled(true);
        mpDeleteBtn->setEnabled(true);
//        mpRescanModelBtn->show();
        break;
    case IN_ROOT:
        mpFavoriteBtn->show();
        mpSocialBtn->hide();
        mSearchBar->show();
        mpDeleteBtn->show();
        mpCloseBtn->hide();
        mpRemoveFavoriteBtn->hide();
        mpNewFolderBtn->show();
        mpNewFolderBtn->setEnabled(false);
        mpDeleteBtn->setEnabled(false);
//        mpRescanModelBtn->show();
        break;
    case IN_PROPERTIES:
        mpFavoriteBtn->show();
        mpSocialBtn->hide();
        mSearchBar->show();
        //mpSearchBtn->show();
        mpDeleteBtn->hide();
        mpCloseBtn->hide();
        mpRemoveFavoriteBtn->hide();
        mpNewFolderBtn->hide();
//        mpRescanModelBtn->hide();
        break;
    case IN_FAVORITE:
        mpFavoriteBtn->hide();
        mpSocialBtn->hide();
        mSearchBar->show();
        //mpSearchBtn->show();
        mpDeleteBtn->hide();
        mpCloseBtn->hide();
        mpRemoveFavoriteBtn->show();
        mpNewFolderBtn->hide();
//        mpRescanModelBtn->hide();
        break;
    case IN_TRASH:
        mpFavoriteBtn->hide();
        mpSocialBtn->hide();
        mSearchBar->show();
        mpDeleteBtn->show();
        mpDeleteBtn->setEnabled(true);
        //mpSearchBtn->show();
        //mpDeleteBtn->hide();
        mpCloseBtn->hide();
        //mpRemoveFavoriteBtn->show();
        mpNewFolderBtn->hide();
//        mpRescanModelBtn->hide();
        break;
    default:
        break;
    }
}

void WBFeaturesActionBar::onSearchTextChanged(QString txt)
{
    Q_UNUSED(txt)
    emit searchElement(mSearchBar->text());
}
   
void WBFeaturesActionBar::onActionNewFolder()
{
    emit newFolderToCreate();
}

void WBFeaturesActionBar::onActionFavorite()
{
    emit addElementsToFavorite();
}

void WBFeaturesActionBar::onActionRemoveFavorite()
{
    emit removeElementsFromFavorite();
}

void WBFeaturesActionBar::onActionTrash()
{
    emit deleteSelectedElements();
}
void WBFeaturesActionBar::onActionRescanModel()
{
    emit rescanModel();
}

void WBFeaturesActionBar::lockIt()
{
    setEnabled(false);
}

void WBFeaturesActionBar::unlockIt()
{
    setEnabled(true);
}

void WBFeaturesActionBar::dragEnterEvent( QDragEnterEvent *event )
{
    const WBFeaturesMimeData *fMimeData = qobject_cast<const WBFeaturesMimeData*>(event->mimeData());
    if (fMimeData) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void WBFeaturesActionBar::dropEvent(QDropEvent *event)
{    
    const WBFeaturesMimeData *fMimeData = qobject_cast<const WBFeaturesMimeData*>(event->mimeData());

    if (!fMimeData) {
        qWarning() << "data came from not supported widget";
        event->ignore();
        return;
    }

    QWidget *dest = childAt(event->pos());
    if (dest == mpDeleteBtn) {
        QList<WBFeature> featuresList = fMimeData->features();
        foreach (WBFeature curFeature, featuresList) {
            if (!curFeature.isDeletable()) {
                qWarning() << "Undeletable feature found, stopping deleting process";
                event->ignore();
                return;
            }
        }
        event->setDropAction(Qt::MoveAction);
        event->accept();

        emit deleteElements(fMimeData);

    } else if (dest == mpFavoriteBtn) {
        event->setDropAction( Qt::CopyAction);
        event->accept();

        emit addToFavorite(fMimeData);

    } else if (dest == mpRemoveFavoriteBtn) {
        event->setDropAction( Qt::MoveAction );
        event->accept();

        emit removeFromFavorite(fMimeData);
    }
}

WBFeaturesActionBar::~WBFeaturesActionBar()
{
}

/**
 * \brief Construtor
 * @param parent as the parent widget
 * @param action as the related action
 * @param name as the related object name
 */
WBActionButton::WBActionButton(QWidget *parent, QAction* action, const char *name):QToolButton(parent)
{
    setObjectName(name);
    addAction(action);
    setDefaultAction(action);
    setIconSize(QSize(BUTTON_SIZE, BUTTON_SIZE));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setStyleSheet(QString("QToolButton {color: white; font-weight: bold; font-family: Arial; background-color: transparent; border: none}"));
    setFocusPolicy(Qt::NoFocus);
}

/**
 * \brief Destructor
 */
WBActionButton::~WBActionButton()
{

}
