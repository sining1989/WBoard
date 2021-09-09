#include <QtWidgets>
#include <QList>
#include <QSize>

#include "WBKeyboardPalette.h"
#include "core/WBSettings.h"

#include "core/WBApplication.h"
#include "gui/WBMainWindow.h"

#include "core/memcheck.h"

/*

            WBKeyboardPalette

*/


WBKeyboardPalette::WBKeyboardPalette(QWidget *parent)
        : WBActionPalette(Qt::TopRightCorner, parent)
{

  //  setWindowFlags(/*Qt::CustomizeWindowHint|*/Qt::WindowStaysOnTopHint|Qt::FramelessWindowHint);

    setCustomCloseProcessing(true);
    setCustomPosition(true);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setFocusPolicy(Qt::NoFocus);
    setClosable(true);
    setGrip(false);

    capsLock = false;
    shift = false;
    languagePopupActive = false;
    keyboardActive = false;
    nSpecialModifierIndex = 0;
    specialModifier = 0;
    btnWidth = btnHeight = 16;
    strSize = "16x16";
    currBtnImages = new BTNImages("16", btnWidth, btnHeight);
    storage = NULL;
	nLocalesCount = 0;

    buttons = new WBKeyButton*[47];
    for (int i=0; i<47; i++)
    {
        buttons[i] = new WBKeyButton(this);
    }

    locales = WBPlatformUtils::getKeyboardLayouts(this->nLocalesCount);

    createCtrlButtons();

    nCurrentLocale = WBSettings::settings()->KeyboardLocale->get().toInt();
    if (nCurrentLocale < 0 || nCurrentLocale >= nLocalesCount)
        nCurrentLocale = 0;
    if (locales!=NULL)
        setInput(locales[nCurrentLocale]);

    setContentsMargins( 22, 22, 22, 22 );

    init();
}

//QList<WBKeyboardPalette*> WBKeyboardPalette::instances;
void WBKeyboardPalette::init()
{
    m_isVisible = false;
    setVisible(false);

    setKeyButtonSize(WBSettings::settings()->boardKeyboardPaletteKeyBtnSize->get().toString());

    connect(this, SIGNAL(keyboardActivated(bool)), this, SLOT(onActivated(bool)));
    connect(WBSettings::settings()->boardKeyboardPaletteKeyBtnSize, SIGNAL(changed(QVariant)), this, SLOT(keyboardPaletteButtonSizeChanged(QVariant)));
    connect(WBApplication::mainWindow->actionVirtualKeyboard, SIGNAL(triggered(bool)), this, SLOT(showKeyboard(bool)));
    connect(this, SIGNAL(closed()), this, SLOT(hideKeyboard()));

    //------------------------------//

    WBPlatformUtils::setWindowNonActivableFlag(this, true);
}

void WBKeyboardPalette::showKeyboard(bool show)
{
    m_isVisible = show;
}


void WBKeyboardPalette::hideKeyboard()
{
    WBApplication::mainWindow->actionVirtualKeyboard->activate(QAction::Trigger);
}

void WBKeyboardPalette::syncPosition(const QPoint & pos)
{
    m_pos = pos;
    move(pos);
}

void WBKeyboardPalette::syncLocale(int nLocale)
{
    nCurrentLocale = nLocale;
    setInput(locales[nCurrentLocale]);
}

void WBKeyboardPalette::keyboardPaletteButtonSizeChanged(QVariant size)
{
    setKeyButtonSize(size.toString());
}

void WBKeyboardPalette::setInput(const WBKeyboardLocale* locale)
{
    if (locale!=NULL)
    {
        for (int i=0; i<47; i++)
            buttons[i]->setKeyBt((*locale)[i]);
    }
    else
    {
        this->hide();
    }
}

WBKeyboardPalette::~WBKeyboardPalette()
{
    //for (int i=0; i<47; i++)
    //    delete buttons[i];
    delete [] buttons;

    //for (int i=0; i<8; i++)
    //    delete ctrlButtons[i];
    delete [] ctrlButtons;

    //if (locales!=NULL)
    //{
    //    for (int i=0; i<nLocalesCount; i++)
    //        delete locales[i];
    //    delete [] locales;
    //}

    if(currBtnImages != NULL)
    {
        delete currBtnImages;
        currBtnImages = NULL;
    }

    onActivated(false);
}

QSize  WBKeyboardPalette::sizeHint () const
{
    int w = contentsMargins().left() + 15 * btnWidth + contentsMargins().right();
    int h = contentsMargins().top() + 5 * btnHeight + contentsMargins().bottom();
    return QSize(w, h);
}

const QString* WBKeyboardPalette::getLocaleName()
{
    return locales == NULL ? NULL : &(locales[nCurrentLocale]->name);
}

void WBKeyboardPalette::setLocale(int nLocale)
{
    if (locales != NULL)
    {
        nCurrentLocale = nLocale;

        setInput(locales[nCurrentLocale]);
        onLocaleChanged(locales[nCurrentLocale]);
        update();

        WBSettings::settings()->KeyboardLocale->set(nCurrentLocale);
    }
    emit localeChanged(nLocale);
}

void WBKeyboardPalette::setKeyButtonSize(const QString& _strSize)
{
    QStringList strs = _strSize.split('x');

    if (strs.size()==2)
    {
        strSize = _strSize;
        btnWidth = strs[0].toInt();
        btnHeight = strs[1].toInt();

        if(currBtnImages != NULL)
            delete currBtnImages;
        currBtnImages = new BTNImages(strs[1], btnWidth, btnHeight);

        adjustSizeAndPosition();
    }
}

void WBKeyboardPalette::enterEvent ( QEvent * )
{
    if (keyboardActive)
        return;

    keyboardActive = true;

    adjustSizeAndPosition();

    emit keyboardActivated(true);
}

void WBKeyboardPalette::leaveEvent ( QEvent * )
{
    if (languagePopupActive || !keyboardActive || mIsMoving)
        return;

    keyboardActive = false;

    adjustSizeAndPosition();

    emit keyboardActivated(false);
}

void  WBKeyboardPalette::moveEvent ( QMoveEvent * event )
{
    WBActionPalette::moveEvent(event);
    emit moved(event->pos());
}

void WBKeyboardPalette::adjustSizeAndPosition(bool pUp)
{
    QSize rSize = sizeHint();
    if (rSize != size())
    {
        int dx = (rSize.width() - size().width()) /2;
        int dy = rSize.height() - size().height();

        this->move(x()-dx, y() - dy);
        this->resize(rSize.width(), rSize.height());
    }
    WBActionPalette::adjustSizeAndPosition(pUp);
}

void  WBKeyboardPalette::paintEvent( QPaintEvent* event)
{
    checkLayout();

    WBActionPalette::paintEvent(event);

    QRect r = this->geometry();

    int lleft, ltop, lright, lbottom;
    getContentsMargins ( &lleft, &ltop, &lright, &lbottom ) ;

    //------------------------------------------------
    // calculate start offset from left, and from top

    int ctrlButtonsId = 0;
    lleft = ( r.width() - btnWidth * 15 ) / 2;
    ltop = ( r.height() - btnHeight * 5 ) / 2;

    //------------------------------------------------
    // set geometry (position) for row 1

    int offX = lleft;
    int offY = ltop;

    //-------------------

    // buttons [`]..[+]
    for (int i = 0; i<13; i++)
    {
        buttons[i]->setGeometry(offX, offY, btnWidth, btnHeight);
        offX += btnWidth;
    }

    // button Backspace
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 2, btnHeight);
    offX += btnWidth * 2;

    //------------------------------------------------
    // set geometry (position) for row 2

    offX = lleft;
    offY += btnHeight;
    offX += btnWidth / 2;

    //-------------------

    // button Tab
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 2, btnHeight);
    offX += btnWidth * 2;

    // buttons [q]..[]]
    for (int i = 0; i<12; i++)
    {
        buttons[i + 13]->setGeometry(offX, offY, btnWidth, btnHeight);
        offX += btnWidth;
    }

//     // Row 2 Stub
//     ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 1.5, btnHeight);
//     offX += btnWidth * 1.5;

    //------------------------------------------------
    // set geometry (position) for row 3

    offX = lleft;
    offY += btnHeight;

    //-------------------

//     // Row 3 Stub

    // button Enter
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 1, btnHeight);
    offX += btnWidth*1;

    // buttons [a]..[\]
    for (int i = 0; i < 12; i++)
    {
        buttons[i + 12 + 13]->setGeometry(offX, offY, btnWidth, btnHeight);
        offX += btnWidth;
    }

    // button Enter
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth * 2, btnHeight);
    offX += btnWidth*2;

    //------------------------------------------------
    // set geometry (position) for row 4

    offX = lleft;
    offY += btnHeight;

    //-------------------

    // button LCapsLock
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth*2.5, btnHeight);
    offX += btnWidth*2.5;

    for (int i = 0; i < 10; i++)
    {
        buttons[i + 12 + 12 + 13]->setGeometry(offX, offY, btnWidth, btnHeight);
        offX += btnWidth;
    }

    // button RCapsLock
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX, offY, btnWidth*2.5, btnHeight);
    offX += btnWidth*2.5;

    //------------------------------------------------
    // set geometry (position) for row 5

    offX = lleft;
    offY += btnHeight;

    //-------------------

    ctrlButtons[ctrlButtonsId++]->setGeometry(offX + btnWidth * 1 , offY, btnWidth * 2, btnHeight);
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX + btnWidth * 3 , offY, btnWidth * 9, btnHeight);
    ctrlButtons[ctrlButtonsId++]->setGeometry(offX + btnWidth * 12, offY, btnWidth * 2, btnHeight);

    //------------------------------------------------
}

void  WBKeyboardPalette::onDeactivated()
{
    onActivated(false);
}


//-----------------------------------------------------------------------//
// BTNImages Class
//-----------------------------------------------------------------------//

BTNImages::BTNImages(QString strHeight, int width, int height)
{
    m_strHeight = strHeight;
    m_width = width;
    m_height = height;

    m_strLeftPassive    = ":/images/virtual.keyboard/" + strHeight + "/left-passive.png";
    m_strCenterPassive  = ":/images/virtual.keyboard/" + strHeight + "/centre-passive.png";
    m_strRightPassive   = ":/images/virtual.keyboard/" + strHeight + "/right-passive.png";
    m_strLeftActive     = ":/images/virtual.keyboard/" + strHeight + "/left-active.png";
    m_strCenterActive   = ":/images/virtual.keyboard/" + strHeight + "/centre-active.png";
    m_strRightActive    = ":/images/virtual.keyboard/" + strHeight + "/right-active.png";

    m_btnLeftPassive    = QImage(m_strLeftPassive);
    m_btnCenterPassive  = QImage(m_strCenterPassive);
    m_btnRightPassive   = QImage(m_strRightPassive);
    m_btnLeftActive     = QImage(m_strLeftActive);
    m_btnCenterActive   = QImage(m_strCenterActive);
    m_btnRightActive    = QImage(m_strRightActive);
}

ContentImage::ContentImage(QString strHeight, int height, QString strContentName)
{
    m_strHeight = strHeight;
    m_height = height;

    m_strContent = ":/images/virtual.keyboard/" + strHeight + "/" + strContentName + ".png";
    m_btnContent = QImage(m_strContent);
}

//-----------------------------------------------------------------------//
// WBKeyboardButton Class
//-----------------------------------------------------------------------//

WBKeyboardButton::WBKeyboardButton(WBKeyboardPalette* parent, QString contentImagePath = "")
        :QWidget(parent),
        keyboard(parent),
        bFocused(false),
        bPressed(false)
{
    m_parent = parent;

    m_contentImagePath = contentImagePath;
    imgContent = NULL;

    setCursor(Qt::PointingHandCursor);
}

WBKeyboardButton::~WBKeyboardButton()
{
    if(imgContent != NULL)
    {
        delete imgContent;
        imgContent = NULL;
    }
}

bool WBKeyboardButton::isPressed()
{
    return bPressed;
}

void WBKeyboardButton::paintEvent(QPaintEvent*)
{

    QPainter painter(this);

    //--------------------------

    if(imgContent != NULL)
    {
        if(imgContent->m_height != m_parent->currBtnImages->m_height)
        {
            delete imgContent;
            if(!m_contentImagePath.isEmpty())
                imgContent = new ContentImage(m_parent->currBtnImages->m_strHeight, m_parent->currBtnImages->m_height, m_contentImagePath);
        }
    }
    else
    if(!m_contentImagePath.isEmpty())
        imgContent = new ContentImage(m_parent->currBtnImages->m_strHeight, m_parent->currBtnImages->m_height, m_contentImagePath);

    //--------------------------

    if (isPressed())
    {
        painter.drawImage( 0,0, m_parent->currBtnImages->m_btnLeftActive, 0,0, m_parent->currBtnImages->m_btnLeftActive.width(), m_parent->currBtnImages->m_btnLeftActive.height() );
        painter.drawImage( QRect(m_parent->currBtnImages->m_btnLeftActive.width(), 0, width() - m_parent->currBtnImages->m_btnLeftActive.width() - m_parent->currBtnImages->m_btnRightActive.width(), height()), m_parent->currBtnImages->m_btnCenterActive );
        painter.drawImage( width() - m_parent->currBtnImages->m_btnRightActive.width(), 0, m_parent->currBtnImages->m_btnRightActive, 0,0, m_parent->currBtnImages->m_btnRightActive.width(), m_parent->currBtnImages->m_btnRightActive.height() );
    }
    else
    {
        painter.drawImage( 0,0, m_parent->currBtnImages->m_btnLeftPassive, 0,0, m_parent->currBtnImages->m_btnLeftPassive.width(), m_parent->currBtnImages->m_btnLeftPassive.height() );
        painter.drawImage( QRect(m_parent->currBtnImages->m_btnLeftPassive.width(), 0, width() - m_parent->currBtnImages->m_btnLeftPassive.width() - m_parent->currBtnImages->m_btnRightPassive.width(), height()), m_parent->currBtnImages->m_btnCenterPassive );
        painter.drawImage( width() - m_parent->currBtnImages->m_btnRightPassive.width(), 0, m_parent->currBtnImages->m_btnRightPassive, 0,0, m_parent->currBtnImages->m_btnRightPassive.width(), m_parent->currBtnImages->m_btnRightPassive.height() );
    }

    //--------------------------

    this->paintContent(painter);

    //--------------------------
}

void  WBKeyboardButton::enterEvent ( QEvent*)
{
    bFocused = true;
    update();
}

void  WBKeyboardButton::leaveEvent ( QEvent*)
{
    bFocused = false;
    update();
}

void  WBKeyboardButton::mousePressEvent ( QMouseEvent * event)
{
    event->accept(); 
    bPressed = true;
    update();
    this->onPress();
}

void  WBKeyboardButton::mouseReleaseEvent ( QMouseEvent * )
{
    bPressed = false;
    update();
    this->onRelease();
}

WBKeyButton::WBKeyButton(WBKeyboardPalette* parent)
        :WBKeyboardButton(parent),
        keybt(0)
{}

WBKeyButton::~WBKeyButton()
{}

bool WBKeyButton::shifted()
{
    bool b = keyboard->shift;
    if (keybt->capsLockSwitch && keyboard->capsLock)
        b = !b;
    return b;
}

void WBKeyButton::onPress()
{
    if (keybt!=NULL)
    {
        int codeIndex = keyboard->nSpecialModifierIndex * 2 + shifted();

        if (keyboard->nSpecialModifierIndex)
        {
            if (keybt->codes[codeIndex].empty())
            {
                sendUnicodeSymbol(keyboard->specialModifier);
                sendUnicodeSymbol(keybt->codes[shifted()]);
            }
            else
            {
                sendUnicodeSymbol(keybt->codes[codeIndex]);
            }

            keyboard->nSpecialModifierIndex = 0;
        }
        else
        {
            int nSpecialModifierIndex = shifted()? keybt->modifier2 : keybt->modifier1;

            if (nSpecialModifierIndex)
            {
                keyboard->nSpecialModifierIndex = nSpecialModifierIndex;
                keyboard->specialModifier = keybt->codes[codeIndex];
            }
            else
            {
                sendUnicodeSymbol(keybt->codes[codeIndex]);            
            }
        }
    }

    if (keyboard->shift)
    {
        keyboard->shift = false;
        keyboard->update();
    }
}

void WBKeyButton::onRelease()
{}

void WBKeyButton::paintContent(QPainter& painter)
{
    if (keybt)
    {
        QString text(QChar(shifted() ? keybt->symbol2 : keybt->symbol1));
        QRect textRect(rect().x()+2, rect().y()+2, rect().width()-4, rect().height()-4);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
}

WBCntrlButton::WBCntrlButton(WBKeyboardPalette* parent, int _code, const QString& _contentImagePath )
        :WBKeyboardButton(parent, _contentImagePath),
        label(""),
        code(_code)
{}


WBCntrlButton::WBCntrlButton(WBKeyboardPalette* parent, const QString& _label, int _code )
        :WBKeyboardButton(parent),
        label(_label),
        code(_code)
{}

WBCntrlButton::~WBCntrlButton()
{}

void WBCntrlButton::onPress()
{
     sendControlSymbol(code);
}

void WBCntrlButton::onRelease()
{}

void WBCntrlButton::paintContent(QPainter& painter)
{
    if(!label.isEmpty())
    {
        painter.drawText(rect(), Qt::AlignCenter, label);
    }
    else
    if(imgContent != NULL)
    {
        painter.drawImage(( rect().width() - imgContent->m_btnContent.width() ) / 2, ( rect().height() - imgContent->m_btnContent.height() ) / 2,
            imgContent->m_btnContent, 0,0, imgContent->m_btnContent.width(), imgContent->m_btnContent.height());
    }
}

WBCapsLockButton::WBCapsLockButton(WBKeyboardPalette* parent, const QString _contentImagePath)
        :WBKeyboardButton(parent, _contentImagePath)
{}

WBCapsLockButton::~WBCapsLockButton()
{}

void WBCapsLockButton::onPress()
{
    keyboard->capsLock = !keyboard->capsLock;
    keyboard->update();
}

void WBCapsLockButton::onRelease()
{}

bool WBCapsLockButton::isPressed()
{
    return keyboard->capsLock;
}

void WBCapsLockButton::paintContent(QPainter& painter)
{
    if(imgContent != NULL)
    {
        painter.drawImage(( rect().width() - imgContent->m_btnContent.width() ) / 2, ( rect().height() - imgContent->m_btnContent.height() ) / 2,
            imgContent->m_btnContent, 0,0, imgContent->m_btnContent.width(), imgContent->m_btnContent.height());
    }
    else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}

WBShiftButton::WBShiftButton(WBKeyboardPalette* parent, const QString _contentImagePath)
    :WBKeyboardButton(parent, _contentImagePath)
{}

WBShiftButton::~WBShiftButton()
{}

void WBShiftButton::onPress()
{
    keyboard->shift = !keyboard->shift;
    keyboard->update();
}


void WBShiftButton::onRelease()
{}

bool WBShiftButton::isPressed()
{
    return keyboard->shift;
}

void WBShiftButton::paintContent(QPainter& painter)
{
    if(imgContent != NULL)
    {
        painter.drawImage(( rect().width() - imgContent->m_btnContent.width() ) / 2, ( rect().height() - imgContent->m_btnContent.height() ) / 2,
            imgContent->m_btnContent, 0,0, imgContent->m_btnContent.width(), imgContent->m_btnContent.height());
    }
    else
        painter.drawText(rect(), Qt::AlignCenter, "^");
}



WBLocaleButton::WBLocaleButton(WBKeyboardPalette* parent)
        :WBKeyboardButton(parent)
{
    localeMenu = new QMenu(this);

    for (int i=0; i<parent->nLocalesCount; i++)
    {
        QAction* action = (parent->locales[i]->icon!=NULL) ?
                          localeMenu->addAction(*parent->locales[i]->icon, parent->locales[i]->fullName)
                          : localeMenu->addAction(parent->locales[i]->fullName);
        action->setData(QVariant(i));
    }
}

WBLocaleButton::~WBLocaleButton()
{
    delete localeMenu;
}

void WBLocaleButton::onPress()
{
}

void WBLocaleButton::onRelease()
{
    keyboard->languagePopupActive = true;
    QAction* action = localeMenu->exec(mapToGlobal(QPoint(0,0)));
    keyboard->languagePopupActive = false;
    if (action!=NULL)
    {
        int nLocale = action->data().toInt();
        keyboard->setLocale(nLocale);
    }
}

void WBLocaleButton::paintContent(QPainter& painter)
{
    const QString* localeName = keyboard->getLocaleName();
    if (localeName!=NULL)
        painter.drawText(rect(), Qt::AlignCenter, *localeName);
}
