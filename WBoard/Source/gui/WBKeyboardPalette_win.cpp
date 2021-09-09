#include "WBKeyboardPalette.h"

#include <qt_windows.h>

#include "../core/WBApplication.h"
#include "../gui/WBMainWindow.h"

#include "core/memcheck.h"

void WBKeyboardButton::sendUnicodeSymbol(KEYCODE keycode)
{
    INPUT input[2];
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = keycode.symbol;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = keycode.symbol;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;

    ::SendInput(2, input, sizeof(input[0]));
}

void WBKeyboardButton::sendControlSymbol(int nSymbol)
{
    INPUT input[2];
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = nSymbol;
    input[0].ki.wScan = 0;
    input[0].ki.dwFlags = 0;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = nSymbol;
    input[1].ki.wScan = 0;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;

    ::SendInput(2, input, sizeof(input[0]));
}

void WBKeyboardPalette::createCtrlButtons()
{
    int ctrlID = 0;
    ctrlButtons = new WBKeyboardButton*[9];

    ctrlButtons[ctrlID++] = new WBCntrlButton(this, 0x08, "backspace");// Backspace
    ctrlButtons[ctrlID++] = new WBCntrlButton(this, 0x09, "tab");      // Tab
    ctrlButtons[ctrlID++] = new WBCapsLockButton(this, "capslock");    // Shift
    ctrlButtons[ctrlID++] = new WBCntrlButton(this, tr("Enter"), 0x0d);    // Enter
    ctrlButtons[ctrlID++] = new WBShiftButton(this, "shift");    // Shift
    ctrlButtons[ctrlID++] = new WBShiftButton(this, "shift");    // Shift
    ctrlButtons[ctrlID++] = new WBLocaleButton(this);                  // Language Switch 
    ctrlButtons[ctrlID++] = new WBCntrlButton(this, "", 0x20);         // Space
    ctrlButtons[ctrlID++] = new WBLocaleButton(this);                  // Language Switch 
}

void WBKeyboardPalette::checkLayout()
{}

void WBKeyboardPalette::onActivated(bool)
{}

void WBKeyboardPalette::onLocaleChanged(WBKeyboardLocale* )
{}

