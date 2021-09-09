#ifndef WBURLLINEEDIT_H
#define WBURLLINEEDIT_H

#include <QtWidgets>
#include <QWidget>
#include <QLineEdit>
#include <QStyleOption>

#include "WBWebView.h"

class WBClearButton;

class WBExLineEdit : public QWidget
{
    Q_OBJECT;

public:
    WBExLineEdit(QWidget *parent = 0);

    inline QLineEdit *lineEdit() const { return mLineEdit; }

    void setLeftWidget(QWidget *widget);
    QWidget *leftWidget() const;

    QSize sizeHint() const;

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    void setVisible(bool pVisible);

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void inputMethodEvent(QInputMethodEvent *e);
    bool event(QEvent *event);

protected:
    void updateGeometries();
    void initStyleOption(QStyleOptionFrameV2 *option) const;

    QWidget*        mLeftWidget;
    QLineEdit*      mLineEdit;
    WBClearButton*    mClearButton;
};


class WBWebView;

class WBUrlLineEdit : public WBExLineEdit
{
    Q_OBJECT;

public:
    WBUrlLineEdit(QWidget *parent = 0);
    void setWebView(WBWebView *webView);

protected:

    void focusOutEvent(QFocusEvent *event);

private slots:
    void webViewUrlChanged(const QUrl &url);

private:
    WBWebView *mWebView;
};

#endif // WBURLLINEEDIT_H

