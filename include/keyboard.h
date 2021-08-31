#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
#include "locales/locales.h"

namespace keyboard {
    typedef struct report {
        uint8_t modifiers;
        uint8_t reserved;
        uint8_t keys[6];
    } report;
    
    void setRST(uint8_t _pinrst);
    
    void begin();

    void setLocale(hid_locale_t* locale);

    void send(report* k);
    void release();

    void pressKey(uint8_t key, uint8_t modifiers = KEY_NONE);
    void pressModifier(uint8_t key);

    uint8_t press(const char* strPtr);

    uint8_t write(const char* c);
    void write(const char* str, size_t len);
}

#endif