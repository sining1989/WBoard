#ifndef WBKEYBOARDPALETTE_H
#define WBKEYBOARDPALETTE_H

#include "WBActionPalette.h"

#include <QLayout>
#include <QPainter>
#include <QMenu>
#include <QIcon>

#include "frameworks/WBPlatformUtils.h"

class WBKeyButton;
class WBKeyboardButton;

class WBApplication;
class WBMainWindow;

class BTNImages
{
public:
    BTNImages(QString _strHeight, int _width, int _height);

    QString m_strHeight;
    int m_width;
    int m_height;

    QImage m_btnLeftPassive;
    QImage m_btnCenterPassive;
    QImage m_btnRightPassive;
    QImage m_btnLeftActive;
    QImage m_btnCenterActive;
    QImage m_btnRightActive;

private:
    QString m_strLeftPassive;
    QString m_strCenterPassive;
    QString m_strRightPassive;
    QString m_strLeftActive;
    QString m_strCenterActive;
    QString m_strRightActive;

};

class ContentImage
{
public:
    ContentImage(QString strHeight, int m_height, QString strContentPath);

    QString m_strHeight;
    int m_height;

    QImage m_btnContent;

private:
    QString m_strContent;
};

class WBKeyboardPalette : public WBActionPalette
{
    Q_OBJECT

friend class WBKeyboardButton;
friend class WBCapsLockButton;
friend class WBShiftButton;
friend class WBLocaleButton;
friend class WBKeyButton;

public:
    WBKeyboardPalette(QWidget *parent);
    ~WBKeyboardPalette();

    BTNImages *currBtnImages;

    bool isEnabled(){return locales!= NULL;}
    virtual QSize  sizeHint () const;
    virtual void adjustSizeAndPosition(bool pUp = true);
    QString getKeyButtonSize() const {QString res; res.sprintf("%dx%d", btnWidth, btnHeight); return res;}
    void setKeyButtonSize(const QString& strSize);

    bool m_isVisible;
    QPoint m_pos;

signals:
    void moved(const QPoint&);
    void localeChanged(int);
    void keyboardActivated(bool);

private slots:
    void syncPosition(const QPoint & pos);
    void syncLocale(int nLocale);
    void keyboardPaletteButtonSizeChanged(QVariant size);
    void onActivated(bool b);
    void onDeactivated();
    void showKeyboard(bool show);
    void hideKeyboard();

protected:
    bool capsLock;
    bool shift;
    int nCurrentLocale;
    int nLocalesCount;
    WBKeyboardLocale** locales;

    int nSpecialModifierIndex;
    KEYCODE specialModifier;

    QString strSize;
    int btnWidth;
    int btnHeight;

    bool languagePopupActive;
    bool keyboardActive;

    virtual void  enterEvent ( QEvent * event );
    virtual void  leaveEvent ( QEvent * event );
    virtual void  paintEvent(QPaintEvent *event);
    virtual void  moveEvent ( QMoveEvent * event );

    void sendKeyEvent(KEYCODE keyCode);

    void setLocale(int nLocale);

    const QString* getLocaleName();

    void init();

private:
    QRect originalRect;

    WBKeyButton** buttons;
    WBKeyboardButton** ctrlButtons;

    void checkLayout();

    void createCtrlButtons();

    void setInput(const WBKeyboardLocale* locale);

    // Can be redefined under each platform
    void onLocaleChanged(WBKeyboardLocale* locale);

    // Storage for platform-dependent objects (linux)
    void* storage;
    // Linux-related parameters
    int min_keycodes, max_keycodes, byte_per_code;
};

class WBKeyboardButton : public QWidget
{
    Q_OBJECT

public:
    WBKeyboardButton(WBKeyboardPalette* parent, QString contentImagePath);
    ~WBKeyboardButton();

protected:
    WBKeyboardPalette* m_parent;
    ContentImage *imgContent;
    QString m_contentImagePath;

    void paintEvent(QPaintEvent *event);

    virtual void  enterEvent ( QEvent * event );
    virtual void  leaveEvent ( QEvent * event );
    virtual void  mousePressEvent ( QMouseEvent * event );
    virtual void  mouseReleaseEvent ( QMouseEvent * event );

    virtual void onPress() = 0;
    virtual void onRelease() = 0;
    virtual void paintContent(QPainter& painter) = 0;

    virtual bool isPressed();

    WBKeyboardPalette* keyboard;

    void sendUnicodeSymbol(KEYCODE keycode);
    void sendControlSymbol(int nSymbol);

private:
    bool bFocused;
    bool bPressed;
};

class WBKeyButton : public WBKeyboardButton
{
    Q_OBJECT

public:
    WBKeyButton(WBKeyboardPalette* parent);
    ~WBKeyButton();

    void setKeyBt(const KEYBT* keybt){this->keybt = keybt;}

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter& painter);

private:
    bool shifted();
    const KEYBT* keybt;
};

class WBCntrlButton : public WBKeyboardButton
{
    Q_OBJECT

public:
    WBCntrlButton(WBKeyboardPalette* parent, int _code, const QString& _contentImagePath );
    WBCntrlButton(WBKeyboardPalette* parent, const QString& _label, int _code );
    ~WBCntrlButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter& painter);

private:
    QString label;
    int code;
};

class WBCapsLockButton : public WBKeyboardButton
{
    Q_OBJECT

public:
    WBCapsLockButton(WBKeyboardPalette* parent, const QString _contentImagePath);
    ~WBCapsLockButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter& painter);

protected:
    virtual bool isPressed();
};

class WBShiftButton : public WBKeyboardButton
{
    Q_OBJECT

public:
    WBShiftButton(WBKeyboardPalette* parent, const QString _contentImagePath);
    ~WBShiftButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter& painter);

protected:
    virtual bool isPressed();
};

class WBLocaleButton : public WBKeyboardButton
{
    Q_OBJECT

public:
    WBLocaleButton(WBKeyboardPalette* parent);
    ~WBLocaleButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter& painter);

protected:
    QMenu* localeMenu;
};

#endif // WBKEYBOARDPALETTE_H
