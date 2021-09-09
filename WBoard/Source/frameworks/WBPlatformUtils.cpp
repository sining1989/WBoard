#include "WBPlatformUtils.h"

#include "core/memcheck.h"

void WBPlatformUtils::destroy()
{
    destroyKeyboardLayouts();
}

WBPlatformUtils::WBPlatformUtils()
{
    // NOOP
}

WBPlatformUtils::~WBPlatformUtils()
{
    // NOOP
}

bool WBPlatformUtils::hasVirtualKeyboard()
{
    return keyboardLayouts!=NULL && nKeyboardLayouts!=0;
}


WBKeyboardLocale::~WBKeyboardLocale()
{
    if (varSymbols!=NULL)
    {
        for(int i=0; i<SYMBOL_KEYS_COUNT; i++)
            delete varSymbols[i];
        delete [] varSymbols;
    }
    delete icon;
}


int WBPlatformUtils::nKeyboardLayouts;
WBKeyboardLocale** WBPlatformUtils::keyboardLayouts;

WBKeyboardLocale** WBPlatformUtils::getKeyboardLayouts(int& nCount)
{
    nCount = nKeyboardLayouts;
    return keyboardLayouts;
}

