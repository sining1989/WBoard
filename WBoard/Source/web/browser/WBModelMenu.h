#ifndef WBMODELMENU_H
#define WBMODELMENU_H

#include <QtWidgets>
#include <QMenu>

class WBModelMenu : public QMenu
{
    Q_OBJECT

public:
    WBModelMenu(QWidget *parent = 0);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

    void setMaxRows(int max);
    int maxRows() const;

    void setFirstSeparator(int offset);
    int firstSeparator() const;

    void setRootIndex(const QModelIndex &index);
    QModelIndex rootIndex() const;

    void setHoverRole(int role);
    int hoverRole() const;

    void setSeparatorRole(int role);
    int separatorRole() const;

    QAction *makeAction(const QIcon &icon, const QString &text, QObject *parent);

protected:
    virtual bool prePopulated();
    virtual void postPopulated();
    void createMenu(const QModelIndex &parent, int max, QMenu *parentMenu = 0, QMenu *menu = 0);

signals:
	void activated(const QModelIndex &index);
	void hovered(const QString &text);

private slots:
    void aboutToShow();
    void triggered(QAction *action);
    void hovered(QAction *action);

private:
    QAction *makeAction(const QModelIndex &index);
    int m_maxRows;
    int m_firstSeparator;
    int m_maxWidth;
    int m_hoverRole;
    int m_separatorRole;
    QAbstractItemModel *m_model;
    QPersistentModelIndex m_root;
};

#endif // WBMODELMENU_H

