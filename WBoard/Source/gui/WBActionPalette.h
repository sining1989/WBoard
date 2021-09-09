#ifndef WBACTIONPALETTE_H_
#define WBACTIONPALETTE_H_

#include <QtWidgets>
#include <QPoint>
#include <QButtonGroup>
#include <QToolButton>

#include "WBFloatingPalette.h"

class WBActionPaletteButton;

class WBActionPalette : public WBFloatingPalette
{
    Q_OBJECT

    public:
        WBActionPalette(QList<QAction*> actions, Qt::Orientation orientation = Qt::Vertical, QWidget* parent = 0);
        WBActionPalette(Qt::Orientation orientation, QWidget* parent = 0);
        WBActionPalette(Qt::Corner corner, QWidget* parent = 0, Qt::Orientation orient = Qt::Vertical);
        WBActionPalette(QWidget* parent = 0);

        virtual ~WBActionPalette();

        void setButtonIconSize(const QSize& size);
        void setToolButtonStyle(Qt::ToolButtonStyle);

        QList<QAction*> actions();
        virtual void setActions(QList<QAction*> actions);
        void groupActions();
        virtual void addAction(QAction* action);

        void setClosable(bool closable);
        void setAutoClose(bool autoClose)
        {
            mAutoClose = autoClose;
        }

        void setCustomCloseProcessing(bool customCloseProcessing)
        {
            m_customCloseProcessing = customCloseProcessing;
        }
        bool m_customCloseProcessing;

        virtual int border();
        virtual void clearLayout();
        QSize buttonSize();

        virtual WBActionPaletteButton* getButtonFromAction(QAction* action);

    public slots:
        void close();

    signals:
        void closed();
        void buttonGroupClicked(int id);
        void customMouseReleased();

    protected:
        virtual void paintEvent(QPaintEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent * event);
        virtual void init(Qt::Orientation orientation);

        virtual void updateLayout();

        QList<WBActionPaletteButton*> mButtons;
        QButtonGroup* mButtonGroup;
        QList<QAction*> mActions;
        QMap<QAction*, WBActionPaletteButton*> mMapActionToButton;

        bool mIsClosable;
        Qt::ToolButtonStyle mToolButtonStyle;
        bool mAutoClose;
        QSize mButtonSize;
        QPoint mMousePos;
        WBActionPaletteButton *createPaletteButton(QAction* action, QWidget *parent);

    protected slots:
        void buttonClicked();
        void actionChanged();
};

class WBActionPaletteButton : public QToolButton
{
    Q_OBJECT

    public:
        WBActionPaletteButton(QAction* action, QWidget * parent = 0);
        virtual ~WBActionPaletteButton();

    signals:
        void doubleClicked();

    protected:
        virtual void mouseDoubleClickEvent(QMouseEvent *event);
        virtual bool hitButton(const QPoint &pos) const;

};

#endif /* WBACTIONPALETTE_H_ */
