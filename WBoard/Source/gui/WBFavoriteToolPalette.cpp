#include "WBFavoriteToolPalette.h"

#include "core/WBSettings.h"

#include "board/WBBoardController.h"
#include "domain/WBGraphicsWidgetItem.h"

#include "tools/WBToolsManager.h"

#include "gui/WBMainWindow.h"

#include "core/memcheck.h"

WBFavoriteToolPalette::WBFavoriteToolPalette(QWidget* parent)
    : WBActionPalette(Qt::Horizontal, parent)
{
    QWidget *container = new QWidget(this);
    container->setStyleSheet("QWidget {background-color: transparent}");

    QGridLayout *gridLayout = new QGridLayout();
    container->setLayout(gridLayout);
    layout()->addWidget(container);

    QList<QAction*> toolsActions;

    QStringList favoritesToolUris = WBSettings::settings()->favoritesNativeToolUris->get().toStringList();

    foreach(QString uri, favoritesToolUris)
    {
        WBToolsManager::WBToolDescriptor desc = WBToolsManager::manager()->toolByID(uri);

        if (desc.label.length() > 0 && !desc.icon.isNull())
        {
            QAction *action = new QAction(desc.label + " " + desc.version, this);
            action->setData(QUrl(desc.id));
            action->setIcon(desc.icon);
            connect(action, SIGNAL(triggered()), this, SLOT(addFavorite()));

            toolsActions << action;
        }
    }

    QDir favoritesDir(WBSettings::settings()->userInteractiveFavoritesDirectory());
    QStringList favoritesSubDirs =  favoritesDir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    QStringList appPathes;

    foreach(QString subDirName, favoritesSubDirs)
    {
        appPathes << favoritesDir.path() + "/" + subDirName;
    }

    foreach(QString widgetPath, appPathes)
    {
        QAction *action = new QAction(WBGraphicsWidgetItem::widgetName(QUrl::fromLocalFile(widgetPath)), this);
        action->setData(QUrl::fromLocalFile(widgetPath));
        action->setIcon(QIcon(WBGraphicsWidgetItem::iconFilePath(QUrl::fromLocalFile(widgetPath))));
        connect(action, SIGNAL(triggered()), this, SLOT(addFavorite()));

        toolsActions << action;
    }


    if (toolsActions.size() < 4)
    {
        QStringList toolsIDs = WBToolsManager::manager()->allToolIDs();

        foreach(QString id, favoritesToolUris)
            toolsIDs.removeAll(id);

        while(toolsIDs.size() > 0 && toolsActions.size() < 4)
        {
            WBToolsManager::WBToolDescriptor desc = WBToolsManager::manager()->toolByID(toolsIDs.takeFirst());

            if (desc.label.length() > 0)
            {
                QAction *action = new QAction(desc.label + " " + desc.version, this);
                action->setData(QUrl(desc.id));
                action->setIcon(desc.icon);
                connect(action, SIGNAL(triggered()), this, SLOT(addFavorite()));

                toolsActions << action;
            }
        }
    }

    int i = 0;

    foreach(QAction* action, toolsActions)
    {
        WBActionPaletteButton* button = createPaletteButton(action, container);
        gridLayout->addWidget(button, i / 4, i % 4);
        mActions << action;
        i++;
    }

    setClosable(true);
    setButtonIconSize(QSize(128, 128));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    groupActions();
}


void WBFavoriteToolPalette::addFavorite()
{
    // we need the sender :-( hugly ...
    QAction* action = qobject_cast<QAction*>(sender());

    if(action)
    {
        QVariant widgetPathVar = action->data();
        if (!widgetPathVar.isNull())
        {
            WBApplication::boardController->downloadURL(widgetPathVar.toUrl());
        }
    }
}

WBFavoriteToolPalette::~WBFavoriteToolPalette()
{
    // NOOP
}
