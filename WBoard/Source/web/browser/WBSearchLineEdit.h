#ifndef WBSEARCHLINEEDIT_H
#define WBSEARCHLINEEDIT_H

#include "WBUrlLineEdit.h"

#include <QtWidgets>
#include <QAbstractButton>

class WBSearchButton;

class WBClearButton : public QAbstractButton
{
    Q_OBJECT;

    public:
        WBClearButton(QWidget *parent = 0);
        void paintEvent(QPaintEvent *event);

    public slots:
        void textChanged(const QString &text);
};

class WBSearchLineEdit : public WBExLineEdit
{
    Q_OBJECT;
    Q_PROPERTY(QString inactiveText READ inactiveText WRITE setInactiveText)

public:
    WBSearchLineEdit(QWidget *parent = 0);

    QString inactiveText() const;
    void setInactiveText(const QString &text);

    QMenu *menu() const;
    void setMenu(QMenu *menu);

    void setVisible(bool pVisible);

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
signals:
	void textChanged(const QString &text);

private:
    void updateGeometries();

    WBSearchButton *mSearchButton;
    QString mInactiveText;
};

#endif // WBSEARCHLINEEDIT_H

