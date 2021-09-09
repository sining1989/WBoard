#include "WBBackgroundPalette.h"

#include "gui/WBMainWindow.h"

WBBackgroundPalette::WBBackgroundPalette(QList<QAction*> actions, QWidget * parent)
    : WBActionPalette(parent)
{
    init();
    setActions(actions);
}


WBBackgroundPalette::WBBackgroundPalette(QWidget * parent)
     : WBActionPalette(parent)
{
    init();
}



void WBBackgroundPalette::init()
{
    WBActionPalette::clearLayout();
    delete layout();


    m_customCloseProcessing = false;

    mButtonSize = QSize(32, 32);
    mIsClosable = false;
    mAutoClose = false;
    mButtonGroup = 0;
    mToolButtonStyle = Qt::ToolButtonIconOnly;
    mButtons.clear();

    mVLayout = new QVBoxLayout(this);
    mTopLayout = new QHBoxLayout();
    mBottomLayout = new QHBoxLayout();

    mVLayout->addLayout(mTopLayout);
    mVLayout->addLayout(mBottomLayout);

    mSlider = new QSlider(Qt::Horizontal);

    mSlider->setMinimum(WBSettings::settings()->minCrossSize);
    mSlider->setMaximum(WBSettings::settings()->maxCrossSize);
    mSlider->setSingleStep(2);
    mSlider->setTracking(true); // valueChanged() is emitted during movement and not just upon releasing the slider

    mSliderLabel = new QLabel(tr("Grid size"));

    mResetDefaultGridSizeButton = createPaletteButton(WBApplication::mainWindow->actionDefaultGridSize, this);
    mResetDefaultGridSizeButton->setFixedSize(24,24);
    mActions << WBApplication::mainWindow->actionDefaultGridSize;

    connect(WBApplication::mainWindow->actionDefaultGridSize, SIGNAL(triggered()), this, SLOT(defaultBackgroundGridSize()));

    mBottomLayout->addSpacing(16);
    mBottomLayout->addWidget(mSliderLabel);
    mBottomLayout->addWidget(mSlider);
    mBottomLayout->addWidget(mResetDefaultGridSizeButton);
    mBottomLayout->addSpacing(16);

    updateLayout();
}

void WBBackgroundPalette::addAction(QAction* action)
{
    WBActionPaletteButton* button = createPaletteButton(action, this);

    mTopLayout->addWidget(button);
    mActions << action;
}

void WBBackgroundPalette::setActions(QList<QAction*> actions)
{
    mMapActionToButton.clear();

    foreach(QAction* action, actions)
    {
        addAction(action);
    }

    actionChanged();
}

void WBBackgroundPalette::updateLayout()
{
    if (mToolButtonStyle == Qt::ToolButtonIconOnly) {
        mVLayout->setContentsMargins (sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border()
                , sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border());
    }
    else
    {
        mVLayout->setContentsMargins (sLayoutContentMargin  + border(), sLayoutContentMargin  + border()
                , sLayoutContentMargin  + border(), sLayoutContentMargin + border());

    }
   update();
}

void WBBackgroundPalette::clearLayout()
{
    while(!mTopLayout->isEmpty()) {
        QLayoutItem* pItem = mTopLayout->itemAt(0);
        QWidget* pW = pItem->widget();
        mTopLayout->removeItem(pItem);
        delete pItem;
        mTopLayout->removeWidget(pW);
        delete pW;
    }

    delete mTopLayout;

    while(!mBottomLayout->isEmpty()) {
        QLayoutItem* pItem = mBottomLayout->itemAt(0);
        QWidget* pW = pItem->widget();
        mBottomLayout->removeItem(pItem);
        delete pItem;
        mBottomLayout->removeWidget(pW);
        delete pW;
    }

    delete mBottomLayout;

    delete mVLayout;

    mActions.clear();
    mButtons.clear();
}

void WBBackgroundPalette::showEvent(QShowEvent* event)
{
    backgroundChanged();

    mSlider->setValue(WBApplication::boardController->activeScene()->backgroundGridSize());
    connect(mSlider, SIGNAL(valueChanged(int)),
            this, SLOT(sliderValueChanged(int)));

    QWidget::showEvent(event);
}

void WBBackgroundPalette::sliderValueChanged(int value)
{
    WBApplication::boardController->activeScene()->setBackgroundGridSize(value);
    WBSettings::settings()->crossSize = value; // since this function is called (indirectly, by refresh) when we switch scenes, the settings will always have the current scene's cross size.
}

void WBBackgroundPalette::defaultBackgroundGridSize()
{
    mSlider->setValue(WBSettings::settings()->defaultCrossSize);
    sliderValueChanged(WBSettings::settings()->defaultCrossSize);
}

void WBBackgroundPalette::backgroundChanged()
{
    bool dark = WBApplication::boardController->activeScene()->isDarkBackground();

    if (dark)
        mSliderLabel->setStyleSheet("QLabel { color : white; }");
    else
        mSliderLabel->setStyleSheet("QLabel { color : black; }");
}

void WBBackgroundPalette::refresh()
{
    backgroundChanged();
    mSlider->setValue(WBApplication::boardController->activeScene()->backgroundGridSize());
}
