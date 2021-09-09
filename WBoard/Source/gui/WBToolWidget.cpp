#include <QtWidgets>
#include <QWebEngineView>
#include <QtWebChannel/QWebChannel>

#include "WBToolWidget.h"
#include "api/WBWidgetUniboardAPI.h"
#include "api/WBW3CWidgetAPI.h"
#include "board/WBBoardController.h"
#include "board/WBBoardView.h"
#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsWidgetItem.h"
#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"
#include "core/memcheck.h"


QPixmap* WBToolWidget::sClosePixmap = 0;
QPixmap* WBToolWidget::sUnpinPixmap = 0;


WBToolWidget::WBToolWidget(const QUrl& pUrl, QWidget *pParent)
    : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , mWebView(0)
    , mToolWidget(0)
    , mShouldMoveWidget(false)
    , mContentMargin(0)
    , mFrameWidth(0)

{
    int widgetType = WBGraphicsWidgetItem::widgetType(pUrl);
    if (widgetType == WBWidgetType::Apple)
        mToolWidget = new WBGraphicsAppleWidgetItem(pUrl);
    else if (widgetType == WBWidgetType::W3C)
        mToolWidget = new WBGraphicsW3CWidgetItem(pUrl);
    else
        qDebug() << "WBToolWidget::WBToolWidget: Unknown widget Type";

    initialize();
}

WBToolWidget::WBToolWidget(WBGraphicsWidgetItem *pWidget, QWidget *pParent)
    : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , mWebView(0)
    , mToolWidget(pWidget)
    , mShouldMoveWidget(false)
    , mContentMargin(0)
    , mFrameWidth(0)

{
    initialize();
    javaScriptWindowObjectCleared();
}

WBToolWidget::~WBToolWidget()
{
    // NOOP
}

void WBToolWidget::initialize()
{
    if (!sClosePixmap)
        sClosePixmap = new QPixmap(":/images/close.svg");

    if(!sUnpinPixmap)
        sUnpinPixmap = new QPixmap(":/images/unpin.svg");

    WBGraphicsScene *wscene = dynamic_cast<WBGraphicsScene *>(mToolWidget->scene());
    if (wscene)
    {
        wscene->removeItemFromDeletion(mToolWidget);
        wscene->removeItem(mToolWidget);
    }


    mWebView = new QWebEngineView(this);

    //QPalette palette = mWebView->page()->palette();
    //palette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    //mWebView->page()->setPalette(palette);


    mWebView->installEventFilter(this);

    mFrameWidth = WBSettings::settings()->objectFrameWidth;
    mContentMargin = sClosePixmap->width() / 2 + mFrameWidth;
    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(mContentMargin, mContentMargin, mContentMargin, mContentMargin);
    layout()->addWidget(mWebView);

    setFixedSize(mToolWidget->boundingRect().width() + mContentMargin * 2, mToolWidget->boundingRect().height() + mContentMargin * 2);

    //connect(mWebView->page()-> mainFrame(), &QWebChannel::javaScriptWindowObjectCleared,
    //        this, &WBToolWidget::javaScriptWindowObjectCleared);//zhusizhi
    mWebView->load(mToolWidget->mainHtml());


    mWebView->setAcceptDrops(false);
    //mWebView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    mWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);


    connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), this, SLOT(javaScriptWindowObjectCleared()));
}


bool WBToolWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (mShouldMoveWidget && obj == mWebView && event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent*>(event);

        if (mouseMoveEvent->buttons() & Qt::LeftButton)
        {
            move(pos() - mMousePressPos + mWebView->mapTo(this, mouseMoveEvent->pos()));

            event->accept();
            return true;
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

void WBToolWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    /* did webkit consume the mouse press ? */
    mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);
    mMousePressPos = event->pos();
    event->accept();
    update();
}

void WBToolWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(mShouldMoveWidget && (event->buttons() & Qt::LeftButton)) {
        move(pos() - mMousePressPos + event->pos());
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void WBToolWidget::mouseReleaseEvent(QMouseEvent *event)
{
    mShouldMoveWidget = false;

    if (event->pos().x() >= 0 && event->pos().x() < sClosePixmap->width() && event->pos().y() >= 0 && event->pos().y() < sClosePixmap->height()) {
        WBApplication::boardController->removeTool(this);
        event->accept();
    }
    else if (mToolWidget->canBeContent() && event->pos().x() >= mContentMargin && event->pos().x() < mContentMargin + sUnpinPixmap->width() && event->pos().y() >= 0 && event->pos().y() < sUnpinPixmap->height()) {
        WBApplication::boardController->moveToolWidgetToScene(this);
        event->accept();
    }
    else
        QWidget::mouseReleaseEvent(event); /* don't propgate to parent, the widget is deleted in WBApplication */

}

void WBToolWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    //if (isActiveWindow())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(127, 127, 127, 127));

        painter.drawRoundedRect(QRectF(sClosePixmap->width() / 2
                                     , sClosePixmap->height() / 2
                                     , width() - sClosePixmap->width()
                                     , mFrameWidth)
                                     , mFrameWidth / 2
                                     , mFrameWidth / 2);

        painter.drawPixmap(0, 0, *sClosePixmap);

        if (mToolWidget->canBeContent())
            painter.drawPixmap(mContentMargin, 0, *sUnpinPixmap);
    }
}

void WBToolWidget::javaScriptWindowObjectCleared()
{
    WBWidgetUniboardAPI *uniboardAPI = new WBWidgetUniboardAPI(WBApplication::boardController->activeScene(), mToolWidget);
	
    //mWebView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", uniboardAPI);

    //WBGraphicsW3CWidgetItem *graphicsW3cWidgetItem = dynamic_cast<WBGraphicsW3CWidgetItem*>(mToolWidget);
    //if (graphicsW3cWidgetItem)
    //{
    //    WBW3CWidgetAPI* widgetAPI = new WBW3CWidgetAPI(graphicsW3cWidgetItem);
    //    mWebView->page()->mainFrame()->addToJavaScriptWindowObject("widget", widgetAPI);
    //}
}

WBGraphicsWidgetItem* WBToolWidget::toolWidget() const
{
    return mToolWidget;
}

QPoint WBToolWidget::naturalCenter() const
{
    if (mWebView)
        return mWebView->geometry().center();
    else
        return QPoint(0, 0);
}

void WBToolWidget::remove()
{
    mToolWidget = NULL;
    hide();
    deleteLater();
}

void WBToolWidget::centerOn(const QPoint& pos)
{
    QWidget::move(pos - QPoint(width() / 2, height() / 2));
}
