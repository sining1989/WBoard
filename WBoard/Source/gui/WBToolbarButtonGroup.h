#ifndef WBTOOLBARBUTTONGROUP_H_
#define WBTOOLBARBUTTONGROUP_H_

#include <QtWidgets>
#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>

class WBToolbarButtonGroup : public QWidget
{
    Q_OBJECT

public:
    WBToolbarButtonGroup(QToolBar *toolbar, const QList<QAction*> &actions = QList<QAction*>());
    virtual ~WBToolbarButtonGroup();

    void setIcon(const QIcon &icon, int index);
    void setColor(const QColor &color, int index);
    int currentIndex() const;

protected:
    void paintEvent(QPaintEvent *);

public slots:
    void setCurrentIndex(int index);
    void colorPaletteChanged();
    void displayText(QVariant display);

private slots:
    void selected(QAction *action);

signals:
    void activated(int index);
    void currentIndexChanged(int index);

private:
	QToolButton         *mToolButton;
	QString              mLabel;
	QList<QAction*>      mActions;
	QList<QToolButton*>  mButtons;
	int                  mCurrentIndex;
	bool                 mDisplayLabel;
	QActionGroup*        mActionGroup;
};

#endif /* WBTOOLBARBUTTONGROUP_H_ */
