#ifndef WBTOOLBARSEARCH_H
#define WBTOOLBARSEARCH_H

#include "WBSearchLineEdit.h"

#include <QtWidgets>

class WBAutoSaver;

class WBToolbarSearch : public WBSearchLineEdit
{
    Q_OBJECT;

public:
    WBToolbarSearch(QWidget *parent = 0);
    ~WBToolbarSearch();

signals:
	void search(const QUrl &url);

public slots:
    void clear();
    void searchNow();

private slots:
    void save();
    void aboutToShowMenu();
    void triggeredMenuAction(QAction *action);

private:
    void load();

    WBAutoSaver *mAutosaver;
    int mMaxSavedSearches;
    QStringListModel *mStringListModel;
};

#endif // WBTOOLBARSEARCH_H

