#include <QLayout>
#include <QAction>

#include "WBActionPalette.h"

#include "core/memcheck.h"

WBActionPalette::WBActionPalette(QList<QAction*> actions, Qt::Orientation orientation, QWidget * parent)
    : WBFloatingPalette(Qt::TopRightCorner, parent)
{
    init(orientation);
    setActions(actions);
}


WBActionPalette::WBActionPalette(Qt::Orientation orientation, QWidget * parent)
     : WBFloatingPalette(Qt::TopRightCorner, parent)
{
    init(orientation);
}


WBActionPalette::WBActionPalette(QWidget * parent)
     : WBFloatingPalette(Qt::TopRightCorner, parent)
{
    init(Qt::Vertical);
}


WBActionPalette::WBActionPalette(Qt::Corner corner, QWidget * parent, Qt::Orientation orient)
     : WBFloatingPalette(corner, parent)
{
    init(orient);
}


void WBActionPalette::init(Qt::Orientation orientation)
{
    m_customCloseProcessing = false;

    mButtonSize = QSize(32, 32);
    mIsClosable = false;
    mAutoClose = false;
    mButtonGroup = 0;
    mToolButtonStyle = Qt::ToolButtonIconOnly;
    mButtons.clear();

    if (orientation == Qt::Horizontal)
        new QHBoxLayout(this);
    else
        new QVBoxLayout(this);

    updateLayout();
}

void WBActionPalette::setActions(QList<QAction*> actions)
{
    mMapActionToButton.clear();

    foreach(QAction* action, actions)
    {
        addAction(action);
    }

    actionChanged();
}


WBActionPaletteButton* WBActionPalette::createPaletteButton(QAction* action, QWidget *parent)
{
    WBActionPaletteButton* button = new WBActionPaletteButton(action, parent);
    button->setIconSize(mButtonSize);
    button->setToolButtonStyle(mToolButtonStyle);

    if (mButtonGroup)
        mButtonGroup->addButton(button, mButtons.length());

    mButtons << button;

    mMapActionToButton[action] = button;

    connect(button, &WBActionPaletteButton::clicked,
            this, &WBActionPalette::buttonClicked);
    connect(action, &QAction::changed,
            this, &WBActionPalette::actionChanged);

    return button;
}

void WBActionPalette::addAction(QAction* action)
{
    WBActionPaletteButton* button = createPaletteButton(action, this);

    layout()->addWidget(button);

    mActions << action;
}

void WBActionPalette::buttonClicked()
{
    if (mAutoClose)
    {
        close();
    }
}

QList<QAction*> WBActionPalette::actions()
{
    return mActions;
}


WBActionPalette::~WBActionPalette()
{
    qDeleteAll(mButtons.begin(), mButtons.end());
    mButtons.clear();
}


void WBActionPalette::setButtonIconSize(const QSize& size)
{
    foreach(QToolButton* button, mButtons)
        button->setIconSize(size);

    mButtonSize = size;
}


void WBActionPalette::groupActions()
{
    mButtonGroup = new QButtonGroup(this);
    int i = 0;
    foreach(QToolButton* button, mButtons)
    {
        mButtonGroup->addButton(button, i);
        ++i;
    }

    connect(mButtonGroup, SIGNAL(buttonClicked(int)), this, SIGNAL(buttonGroupClicked(int)));
}


void WBActionPalette::setToolButtonStyle(Qt::ToolButtonStyle tbs)
{
    foreach(QToolButton* button, mButtons)
        button->setToolButtonStyle(tbs);

    mToolButtonStyle = tbs;

    updateLayout();

}

void WBActionPalette::updateLayout()
{
    if (mToolButtonStyle == Qt::ToolButtonIconOnly)
    {
        layout()->setContentsMargins (sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border()
                , sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border());
    }
    else
    {
        layout()->setContentsMargins (sLayoutContentMargin  + border(), sLayoutContentMargin  + border()
                , sLayoutContentMargin  + border(), sLayoutContentMargin + border());

    }
   update();
}


void WBActionPalette::setClosable(bool pClosable)
{
    mIsClosable = pClosable;

    updateLayout();
}


int WBActionPalette::border()
{
    if (mIsClosable)
        return 10;
    else
        return 5;
}


void WBActionPalette::paintEvent(QPaintEvent *event)
{
    WBFloatingPalette::paintEvent(event);

    if (mIsClosable)
    {
        QPainter painter(this);
        painter.drawPixmap(0, 0, QPixmap(":/images/close.svg"));
    }
}


void WBActionPalette::close()
{
    if(!m_customCloseProcessing)
        hide();

    emit closed();
}


void WBActionPalette::mouseReleaseEvent(QMouseEvent * event)
{
    if (mIsClosable && event->pos().x() >= 0 && event->pos().x() < QPixmap(":/images/close.svg").width()
        && event->pos().y() >= 0 && event->pos().y() < QPixmap(":/images/close.svg").height())
    {
        event->accept();
        close();
    }

    WBFloatingPalette::mouseReleaseEvent(event);
}


void WBActionPalette::actionChanged()
{
    for(int i = 0; i < mActions.length() && i < mButtons.length(); i++)
    {
        mButtons.at(i)->setVisible(mActions.at(i)->isVisible());
    }
}

void WBActionPalette::clearLayout()
{
    QLayout* pLayout = layout();
    if(NULL != pLayout)
    {
        while(!pLayout->isEmpty())
        {
            QLayoutItem* pItem = pLayout->itemAt(0);
            QWidget* pW = pItem->widget();
            pLayout->removeItem(pItem);
            delete pItem;
            pLayout->removeWidget(pW);
            delete pW;
        }

        mActions.clear();
        mButtons.clear();
    }
}

WBActionPaletteButton::WBActionPaletteButton(QAction* action, QWidget * parent)
    : QToolButton(parent)
{
    setIconSize(QSize(32, 32));
    setDefaultAction(action);
    setStyleSheet(QString("QToolButton {color: white; font-weight: bold; font-family: Arial; background-color: transparent; border: none}"));

    setFocusPolicy(Qt::NoFocus);

    setObjectName("ubActionPaletteButton");
}


WBActionPaletteButton::~WBActionPaletteButton()
{

}


void WBActionPaletteButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    emit doubleClicked();
}

QSize WBActionPalette::buttonSize()
{
    return mButtonSize;
}

/**
 * \brief Returns the button related to the given action
 * @param action as the given action
 */
WBActionPaletteButton* WBActionPalette::getButtonFromAction(QAction *action)
{
    WBActionPaletteButton* pButton = NULL;

    pButton = mMapActionToButton.value(action);

    return pButton;
}

bool WBActionPaletteButton::hitButton(const QPoint &pos) const
{
    Q_UNUSED(pos);
    return true;
}
