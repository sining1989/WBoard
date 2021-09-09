#include "WBToolbarButtonGroup.h"

#include <QtWidgets>
#include <QLayout>
#include <QStyleOption>

#include "core/WBApplication.h"
#include "core/WBSettings.h"

#include "board/WBDrawingController.h"

#include "core/memcheck.h"

WBToolbarButtonGroup::WBToolbarButtonGroup(QToolBar *toolBar, const QList<QAction*> &actions)
    : QWidget(toolBar)
    , mActions(actions)
    , mCurrentIndex(-1)
    , mDisplayLabel(true)
    , mActionGroup(0)
{
    Q_ASSERT(actions.size() > 0);

    mToolButton = qobject_cast<QToolButton*>(toolBar->layout()->itemAt(0)->widget());
    Q_ASSERT(mToolButton);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(0);
    verticalLayout->addStretch();
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addStretch();

    mActionGroup = new QActionGroup(this);
    mActionGroup->setExclusive(true);

    QSize buttonSize;

    int i = 0;

    foreach(QAction *action, actions)
    {
        mActionGroup->addAction(action);

        QToolButton *button = new QToolButton(this);
        mButtons.append(button);
        button->setDefaultAction(action);
        button->setCheckable(true);

        if(i == 0)
        {
            button->setObjectName("ubButtonGroupLeft");
        }
        else if (i == actions.size() - 1)
        {
            button->setObjectName("ubButtonGroupRight");
        }
        else
        {
            button->setObjectName("ubButtonGroupCenter");
        }

        connect(button, SIGNAL(triggered(QAction*)), this, SLOT(selected(QAction*)));

        horizontalLayout->addWidget(button);
        mLabel = action->text();
        buttonSize = button->sizeHint();
        i++;
    }
}

WBToolbarButtonGroup::~WBToolbarButtonGroup()
{
    // NOOP
}

void WBToolbarButtonGroup::setIcon(const QIcon &icon, int index)
{
    Q_ASSERT(index < mActions.size());

    foreach(QWidget *widget, mActions.at(index)->associatedWidgets())
    {
        QToolButton *button = qobject_cast<QToolButton*>(widget);
        if (button)
        {
            button->setIcon(icon);
        }
    }
}

void WBToolbarButtonGroup::setColor(const QColor &color, int index)
{
    QPixmap pixmap(12, 12);
    pixmap.fill(color);
    QIcon icon(pixmap);
    setIcon(icon, index);
}

void WBToolbarButtonGroup::selected(QAction *action)
{
    foreach(QWidget *widget, action->associatedWidgets())
    {
        QToolButton *button = qobject_cast<QToolButton*>(widget);
        if (button)
        {
            int i = 0;
            foreach(QAction *eachAction, mActions)
            {
                if (eachAction == action)
                {
                    setCurrentIndex(i);
                    emit activated(i);
                    break;
                }
                i++;
            }
        }
    }
}

int WBToolbarButtonGroup::currentIndex() const
{
    return mCurrentIndex;
}

void WBToolbarButtonGroup::setCurrentIndex(int index)
{
    Q_ASSERT(index < mButtons.size());

    if (index != mCurrentIndex)
    {
        for(int i = 0; i < mButtons.size(); i++)
        {
            mButtons.at(i)->setChecked(i == index);
        }
        mCurrentIndex = index;
        emit currentIndexChanged(index);
    }
}

void WBToolbarButtonGroup::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QStyleOptionToolButton option;
    QPixmap emptyPixmap(32, 32);
    emptyPixmap.fill(Qt::transparent);
    QIcon emptyIcon(emptyPixmap);

    option.initFrom(mToolButton);

    option.text = mLabel;
    option.font = mToolButton->font();

    int pointSize = mToolButton->font().pointSize();
    if (pointSize > 0)
        option.font.setPointSize(pointSize);
    else
    {
        int pixelSize = mToolButton->font().pixelSize();
        if (pixelSize > 0)
            option.font.setPixelSize(pixelSize);
    }

    option.rect = rect();
    option.icon = emptyIcon; // non null icon is required for style()->drawControl(QStyle::CE_ToolButtonLabel, ...) to work correctly
    option.iconSize = emptyPixmap.size();
    option.toolButtonStyle = mDisplayLabel ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;

    style()->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, this);
}


void WBToolbarButtonGroup::colorPaletteChanged()
{
    bool isDarkBackground = WBSettings::settings()->isDarkBackground();

    QList<QColor> colors;

    if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Pen 
        || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Line)
    {
        colors = WBSettings::settings()->penColors(isDarkBackground);
    }
    else if (WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Marker)
    {
        colors = WBSettings::settings()->markerColors(isDarkBackground);
    }

    for (int i = 0; i < mButtons.size() && i < colors.size(); i++)
    {
        setColor(colors.at(i), i);
    }
}

void WBToolbarButtonGroup::displayText(QVariant display)
{
    mDisplayLabel = display.toBool();
    QVBoxLayout* verticalLayout = (QVBoxLayout*)layout();
    verticalLayout->setStretch(2, mDisplayLabel ? 1 : 0);
    update();
}
