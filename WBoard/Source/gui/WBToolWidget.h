#ifndef WBTOOLWIDGET_H_
#define WBTOOLWIDGET_H_

#include <QtWidgets>
#include <QWidget>

class WBGraphicsWidgetItem;
class QWidget;
class WBGraphicsScene;
class QWebEngineView;

class WBToolWidget : public QWidget
{
    Q_OBJECT

public:
    WBToolWidget(const QUrl& pUrl, QWidget* pParent = 0);
    WBToolWidget(WBGraphicsWidgetItem* pWidget, QWidget* pParent = 0);
    virtual ~WBToolWidget();

    void remove();
    void centerOn(const QPoint& pos);

    QPoint naturalCenter() const;

    WBGraphicsWidgetItem *toolWidget() const;

protected:
    void initialize();
    virtual void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void javaScriptWindowObjectCleared();

protected:
	QWebEngineView *mWebView;
    WBGraphicsWidgetItem *mToolWidget;

    static QPixmap *sClosePixmap;
    static QPixmap *sUnpinPixmap;

    QPoint mMousePressPos;
    bool mShouldMoveWidget;
    int mContentMargin;
    int mFrameWidth;
};

#endif /* WBTOOLWIDGET_H_ */
