#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QCheckBox>
#include <QtWebChannel/QWebChannel>
#include "WBStartupHintsPalette.h"

#include "globals/WBGlobals.h"
#include "core/WBSettings.h"


WBStartupHintsPalette::WBStartupHintsPalette(QWidget *parent) :
    WBFloatingPalette(Qt::TopRightCorner,parent)
{
    setObjectName("WBStartupHintsPalette");
    if(WBSettings::settings()->appStartupHintsEnabled->get().toBool()){
        setFixedSize(700,450);
        mLayout = new QVBoxLayout();
        mLayout->setContentsMargins(10,28,10,10);
        setLayout(mLayout);
        QString url = WBSettings::settings()->applicationStartupHintsDirectory() + "/index.html";
        mpWebView = new QWebEngineView(this);
        mpSankoreAPI = new WBWidgetUniboardAPI(0);
        //mpWebView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
        //connect(mpWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(javaScriptWindowObjectCleared()));
        mpWebView->setUrl(QUrl::fromLocalFile(url));
        mpWebView->setAcceptDrops(false);
        mLayout->addWidget(mpWebView);
        mButtonLayout = new QHBoxLayout();
        mLayout->addLayout(mButtonLayout);
        mShowNextTime = new QCheckBox(tr("Visible next time"),this);
        mShowNextTime->setCheckState(Qt::Checked);
        connect(mShowNextTime,SIGNAL(stateChanged(int)),this,SLOT(onShowNextTimeStateChanged(int)));
        mButtonLayout->addStretch();
        mButtonLayout->addWidget(mShowNextTime);
    }
    else
        hide();
}

WBStartupHintsPalette::~WBStartupHintsPalette()
{
//    DELETEPTR(mButtonLayout);
//    DELETEPTR(mLayout);
}

void WBStartupHintsPalette::paintEvent(QPaintEvent *event)
{
    WBFloatingPalette::paintEvent(event);

    QPainter painter(this);
    painter.drawPixmap(0, 0, QPixmap(":/images/close.svg"));
}


void WBStartupHintsPalette::close()
{
    hide();
}


void WBStartupHintsPalette::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->pos().x() >= 0 && event->pos().x() < QPixmap(":/images/close.svg").width()
        && event->pos().y() >= 0 && event->pos().y() < QPixmap(":/images/close.svg").height())
    {
        event->accept();
        close();
    }

    WBFloatingPalette::mouseReleaseEvent(event);
}

void WBStartupHintsPalette::onShowNextTimeStateChanged(int state)
{
    WBSettings::settings()->appStartupHintsEnabled->setBool(state == Qt::Checked);
}

void WBStartupHintsPalette::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    adjustSizeAndPosition();
    move((parentWidget()->width() - width()) / 2, (parentWidget()->height() - height()) / 5);
}


int WBStartupHintsPalette::border()
{
    return 40;
}

void WBStartupHintsPalette::javaScriptWindowObjectCleared()
{
    //mpWebView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
}
