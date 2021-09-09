#ifndef WBPLATFORMUTILS_H_
#define WBPLATFORMUTILS_H_

#include <QtCore>
#include <QIcon>
#include <QProcess>

class QMainWindow;

#define SYMBOL_KEYS_COUNT 47

struct KEYCODE{
    KEYCODE()
        :symbol(0)
        ,code(0)
        ,modifier(0)
    {}

    KEYCODE(int _symbol)
        :symbol(_symbol)
        ,code(0)
        ,modifier(0)
    {}

    KEYCODE(int _symbol,
            unsigned char _code,
            int _modifier)
        :symbol(_symbol)
        ,code(_code)
        ,modifier(_modifier)
    {}
	
    bool empty() const
    {
        return symbol == 0;
    }

    int symbol;
    unsigned char code;
    int modifier;
};

struct KEYBT
{
    QChar symbol1;
    QChar symbol2;
    bool capsLockSwitch;
    int modifier1;
    int modifier2;
    KEYCODE codes[8];

    KEYBT(  QChar _symbol1,
            QChar _symbol2,
            bool _capsLockSwitch,
            int _modifier1,
            int _modifier2,
            KEYCODE c1 = 0,
            KEYCODE c2 = 0,
            KEYCODE c3 = 0,
            KEYCODE c4 = 0,
            KEYCODE c5 = 0,
            KEYCODE c6 = 0,
            KEYCODE c7 = 0,
            KEYCODE c8 = 0)
                :symbol1(_symbol1)
                ,symbol2(_symbol2)
                ,capsLockSwitch(_capsLockSwitch)
                ,modifier1(_modifier1)
                ,modifier2(_modifier2)
    {
        codes[0] = c1;
        codes[1] = c2;
        codes[2] = c3;
        codes[3] = c4;
        codes[4] = c5;
        codes[5] = c6;
        codes[6] = c7;
        codes[7] = c8;
    }

    ~KEYBT()
    {}
};

class WBKeyboardLocale
{
public:
    WBKeyboardLocale(const QString& _fullName,
        const QString& _name,
        const QString& _id,
        QIcon* _icon,
        KEYBT** _symbols)
        :fullName(_fullName),name(_name), id(_id), icon(_icon),
                    constSymbols(NULL), varSymbols(_symbols)
    {}
    WBKeyboardLocale(const QString& _fullName,
        const QString& _name,
        const QString& _id,
        QIcon* _icon,
        KEYBT _symbols[])
        :fullName(_fullName),name(_name),  id(_id), icon(_icon),
                    constSymbols(_symbols), varSymbols(NULL)
    {}

    ~WBKeyboardLocale();

    const QString fullName;
    const QString name;
    const QString id;
    QIcon* icon;
    KEYBT* operator[] (int index) const
    {
        return (varSymbols==NULL)? constSymbols + index : varSymbols[index];
    }

private:
    KEYBT* constSymbols;
    KEYBT** varSymbols;
};

class WBPlatformUtils
{
    Q_DECLARE_TR_FUNCTIONS(WBPlatformUtils)

    private:
        WBPlatformUtils();
        virtual ~WBPlatformUtils();

        static void initializeKeyboardLayouts();
        static void destroyKeyboardLayouts();
        static int nKeyboardLayouts;
        static WBKeyboardLocale** keyboardLayouts;
		
	public:
        static void init();
        static void destroy();
        static QString applicationResourcesDirectory();
        static void hideFile(const QString &filePath);
        static void setFileType(const QString &filePath, unsigned long fileType);
        static void fadeDisplayOut();
        static void fadeDisplayIn();
        static QString translationPath(QString pFilePrefix, QString pLanguage);
        static QString systemLanguage();
        static bool hasVirtualKeyboard();
        static void bringPreviousProcessToFront();
        static QString osUserLoginName();
        static void setDesktopMode(bool desktop);
        static void setWindowNonActivableFlag(QWidget* widget, bool nonAcivable);
        static QString computerName();
        static WBKeyboardLocale** getKeyboardLayouts(int& nCount);
        static QString urlFromClipboard();
        static QStringList availableTranslations();
        static void setFrontProcess();
        static void showFullScreen(QWidget * pWidget);
        static void showOSK(bool show);

#ifdef Q_OS_OSX
        static void SetMacLocaleByIdentifier(const QString& id);
#endif
};

#endif /* WBPLATFORMUTILS_H_ */
