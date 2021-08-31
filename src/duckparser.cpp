/*
   Copyright (c) 2019 Stefan Kremser
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/SimpleCLI
 */

#include "duckparser.h"

#include "config.h"
// #include "debug.h"
#include "keyboard.h"
#include "led.h"
#include "webserver.h"

#include <stdlib.h>  // malloc
#include <string.h>  // strlen
#include <stdbool.h> // bool
#include <stddef.h> // size_t

#define COMPARE_UNEQUAL 0
#define COMPARE_EQUAL 1

#define COMPARE_CASE_INSENSETIVE 0
#define COMPARE_CASE_SENSETIVE 1


typedef struct word_node {
    const char      * str;
    size_t            len;
    struct word_node* next;
} word_node;

typedef struct word_list {
    struct word_node* first;
    struct word_node* last;
    size_t            size;
} word_list;

typedef struct line_node {
    const char      * str;
    size_t            len;
    struct word_list* words;
    struct line_node* next;
} line_node;

typedef struct line_list {
    struct line_node* first;
    struct line_node* last;
    size_t            size;
} line_list;


namespace duckparser {
  
int compare(const char* user_str, size_t user_str_len, const char* templ_str, int case_sensetive);
// ===== Word Node ===== //
word_node* word_node_create(const char* str, size_t len);
word_node* word_node_destroy(word_node* n);
word_node* word_node_destroy_rec(word_node* n);

// ===== Word List ===== //
word_list* word_list_create();
word_list* word_list_destroy(word_list* l);

void word_list_push(word_list* l, word_node* n);
word_node* word_list_get(word_list* l, size_t i);

// ===== Line Node ==== //
line_node* line_node_create(const char* str, size_t len);
word_node* line_node_destroy(line_node* n);
word_node* line_node_destroy_rec(line_node* n);

// ===== Line List ===== //
line_list* line_list_create();
line_list* line_list_destroy(line_list* l);

void line_list_push(line_list* l, line_node* n);
line_node* line_list_get(line_list* l, size_t i);

// ===== Parser ===== //
word_list* parse_words(const char* str, size_t len);
line_list* parse_lines(const char* str, size_t len);
  ////////////////////////////////////////////////////////////////////////parser.c////////////////////////////////////////////////////////////////////////////////////

// My own implementation, because the default one in ctype.h make problems on older ESP8266 SDKs
char to_lower(char c) {
    if ((c >= 65) && (c <= 90)) {
        return (char)(c + 32);
    }
    return c;
}

int compare(const char* user_str, size_t user_str_len, const char* templ_str, int case_sensetive) {
    if (user_str == templ_str) return COMPARE_EQUAL;

    // null check string pointers
    if (!user_str || !templ_str) return COMPARE_UNEQUAL;

    // string lengths
    size_t str_len = user_str_len; // strlen(user_str);
    size_t key_len = strlen(templ_str);

    // when same length, it there is no need to check for slashes or commas
    if (str_len == key_len) {
        for (size_t i = 0; i < key_len; i++) {
            if (case_sensetive == COMPARE_CASE_SENSETIVE) {
                if (user_str[i] != templ_str[i]) return COMPARE_UNEQUAL;
            } else {
                if (to_lower(user_str[i]) != to_lower(templ_str[i])) return COMPARE_UNEQUAL;
            }
        }
        return COMPARE_EQUAL;
    }

    // string can't be longer than templ_str (but can be smaller because of  '/' and ',')
    if (str_len > key_len) return COMPARE_UNEQUAL;

    unsigned int res_i = 0;
    unsigned int a     = 0;
    unsigned int b     = 0;
    unsigned int res   = 1;

    while (a < str_len && b < key_len) {
        if (templ_str[b] == '/') {
            // skip slash in templ_str
            ++b;
        } else if (templ_str[b] == ',') {
            // on comma increment res_i and reset str-index
            ++b;
            a = 0;
            ++res_i;
        }

        // compare character
        if (case_sensetive == COMPARE_CASE_SENSETIVE) {
            if (user_str[a] != templ_str[b]) res = 0;
        } else {
            if (to_lower(user_str[a]) != to_lower(templ_str[b])) res = 0;
        }

        // comparison incorrect or string checked until the end and templ_str not checked until the end
        if (!res || ((a == str_len - 1) &&
                     (templ_str[b + 1] != ',') &&
                     (templ_str[b + 1] != '/') &&
                     (templ_str[b + 1] != '\0'))) {
            // fast forward to next comma
            while (b < key_len && templ_str[b] != ',') b++;
            res = 1;
        } else {
            // otherwise icrement indices
            ++a;
            ++b;
        }
    }

    // comparison correct AND string checked until the end AND templ_str checked until the end
    if (res && (a == str_len) &&
        ((templ_str[b] == ',') ||
         (templ_str[b] == '/') ||
         (templ_str[b] == '\0'))) return COMPARE_EQUAL;  // res_i

    return COMPARE_UNEQUAL;
}

// ===== Word Node ===== //
word_node* word_node_create(const char* str, size_t len) {
    word_node* n = (word_node*)malloc(sizeof(word_node));

    n->str  = str;
    n->len  = len;
    n->next = NULL;
    return n;
}

word_node* word_node_destroy(word_node* n) {
    if (n) {
        free(n);
    }
    return NULL;
}

word_node* word_node_destroy_rec(word_node* n) {
    if (n) {
        word_node_destroy_rec(n->next);
        word_node_destroy(n);
    }
    return NULL;
}

// ===== Word List ===== //
word_list* word_list_create() {
    word_list* l = (word_list*)malloc(sizeof(word_list));

    l->first = NULL;
    l->last  = NULL;
    l->size  = 0;
    return l;
}

word_list* word_list_destroy(word_list* l) {
    if (l) {
        word_node_destroy_rec(l->first);
        free(l);
    }
    return NULL;
}

void word_list_push(word_list* l, word_node* n) {
    if (l && n) {
        if (l->last) {
            l->last->next = n;
        } else {
            l->first = n;
        }

        l->last = n;
        ++l->size;
    }
}

word_node* word_list_get(word_list* l, size_t i) {
    if (!l) return NULL;

    size_t j;
    word_node* h = l->first;

    for (j = 0; j < i && h; ++j) {
        h = h->next;
    }

    return h;
}

// ===== Line Node ==== //
line_node* line_node_create(const char* str, size_t len) {
    line_node* n = (line_node*)malloc(sizeof(line_node));

    n->str   = str;
    n->len   = len;
    n->words = NULL;
    n->next  = NULL;

    return n;
}

word_node* line_node_destroy(line_node* n) {
    if (n) {
        word_list_destroy(n->words);
        free(n);
    }
    return NULL;
}

word_node* line_node_destroy_rec(line_node* n) {
    if (n) {
        line_node_destroy_rec(n->next);
        line_node_destroy(n);
    }
    return NULL;
}

// ===== Line List ===== //
line_list* line_list_create() {
    line_list* l = (line_list*)malloc(sizeof(line_list));

    l->first = NULL;
    l->last  = NULL;
    l->size  = 0;

    return l;
}

line_list* line_list_destroy(line_list* l) {
    if (l) {
        line_node_destroy_rec(l->first);
        free(l);
    }
    return NULL;
}

void line_list_push(line_list* l, line_node* n) {
    if (l && n) {
        if (l->last) {
            l->last->next = n;
        } else {
            l->first = n;
        }

        l->last = n;
        ++l->size;
    }
}

line_node* line_list_get(line_list* l, size_t i) {
    if (!l) return NULL;

    size_t j;
    line_node* h = l->first;

    for (j = 0; j < i && h; ++j) {
        h = h->next;
    }

    return h;
}

// ===== Parser ===== //
word_list* parse_words(const char* str, size_t len) {
    word_list* l = word_list_create();

    if (len == 0) return l;

    // Go through string and look for space to split it into words
    word_node* n = NULL;

    size_t i = 0; // current index
    size_t j = 0; // start index of word

    int escaped      = 0;
    int ignore_space = 0;

    for (i = 0; i <= len; ++i) {
        if ((str[i] == '\\') && (escaped == 0)) {
            escaped = 1;
        } else if ((str[i] == '"') && (escaped == 0)) {
            ignore_space = !ignore_space;
        } else if ((i == len) || ((str[i] == ' ') && (ignore_space == 0) && (escaped == 0))) {
            size_t k = i - j; // length of word

            // for every word, add to list
            if (k > 0) {
                n = word_node_create(&str[j], k);
                word_list_push(l, n);
            }

            j = i + 1; // reset start index of word
        } else if (escaped == 1) {
            escaped = 0;
        }
    }

    return l;
}

line_list* parse_lines(const char* str, size_t len) {
    line_list* l = line_list_create();

    if (len == 0) return l;

    // Go through string and look for \r and \n to split it into lines
    line_node* n = NULL;

    size_t stri = 0; // current index
    size_t ls   = 0; // start index of line

    bool escaped   = false;
    bool in_quotes = false;
    bool delimiter = false;
    bool linebreak = false;
    bool endofline = false;

    for (stri = 0; stri <= len; ++stri) {
        char prev = stri > 0 ? str[stri-1] : 0;
        char curr = str[stri];
        char next = str[stri+1];

        escaped = prev == '\\';

        // disabled because ducky script isn't using quotes
        // in_quotes = (curr == '"' && !escaped) ? !in_quotes : in_quotes;
        // delimiter = !in_quotes && !escaped && curr == ';' && next == ';';

        linebreak = !in_quotes && (curr == '\r' || curr == '\n');

        endofline = stri == len || curr == '\0';

        if (linebreak || endofline || delimiter) {
            size_t llen = stri - ls; // length of line

            // for every line, parse_words and add to list
            if (llen > 0) {
                n        = line_node_create(&str[ls], llen);
                n->words = parse_words(&str[ls], llen);
                line_list_push(l, n);
            }

            if (delimiter) ++stri;

            ls = stri+1; // reset start index of line
        }
    }

    return l;
}
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ====== PRIVATE ===== //
    bool processing = false;
    bool inString  = false;
    bool inComment = false;

    int defaultDelay = 5;
    int repeatNum    = 0;

    unsigned long interpretTime  = 0;
    unsigned long sleepStartTime = 0;
    unsigned long sleepTime      = 0;

    void type(const char* str, size_t len) {
        keyboard::write(str, len);
    }

    void press(const char* str, size_t len) {
        // character
        if (len == 1) keyboard::press(str);

        // Keys
        else if (compare(str, len, "ENTER", CASE_SENSETIVE)) keyboard::pressKey(KEY_ENTER);
        else if (compare(str, len, "MENU", CASE_SENSETIVE) || compare(str, len, "APP", CASE_SENSETIVE)) keyboard::pressKey(KEY_PROPS);
        else if (compare(str, len, "DELETE", CASE_SENSETIVE)) keyboard::pressKey(KEY_BACKSPACE);
        else if (compare(str, len, "HOME", CASE_SENSETIVE)) keyboard::pressKey(KEY_HOME);
        else if (compare(str, len, "INSERT", CASE_SENSETIVE)) keyboard::pressKey(KEY_INSERT);
        else if (compare(str, len, "PAGEUP", CASE_SENSETIVE)) keyboard::pressKey(KEY_PAGEUP);
        else if (compare(str, len, "PAGEDOWN", CASE_SENSETIVE)) keyboard::pressKey(KEY_PAGEDOWN);
        else if (compare(str, len, "UPARROW", CASE_SENSETIVE) || compare(str, len, "UP", CASE_SENSETIVE)) keyboard::pressKey(KEY_UP);
        else if (compare(str, len, "DOWNARROW", CASE_SENSETIVE) || compare(str, len, "DOWN", CASE_SENSETIVE)) keyboard::pressKey(KEY_DOWN);
        else if (compare(str, len, "LEFTARROW", CASE_SENSETIVE) || compare(str, len, "LEFT", CASE_SENSETIVE)) keyboard::pressKey(KEY_LEFT);
        else if (compare(str, len, "RIGHTARROW", CASE_SENSETIVE) || compare(str, len, "RIGHT", CASE_SENSETIVE)) keyboard::pressKey(KEY_RIGHT);
        else if (compare(str, len, "TAB", CASE_SENSETIVE)) keyboard::pressKey(KEY_TAB);
        else if (compare(str, len, "END", CASE_SENSETIVE)) keyboard::pressKey(KEY_END);
        else if (compare(str, len, "ESC", CASE_SENSETIVE) || compare(str, len, "ESCAPE", CASE_SENSETIVE)) keyboard::pressKey(KEY_ESC);
        else if (compare(str, len, "F1", CASE_SENSETIVE)) keyboard::pressKey(KEY_F1);
        else if (compare(str, len, "F2", CASE_SENSETIVE)) keyboard::pressKey(KEY_F2);
        else if (compare(str, len, "F3", CASE_SENSETIVE)) keyboard::pressKey(KEY_F3);
        else if (compare(str, len, "F4", CASE_SENSETIVE)) keyboard::pressKey(KEY_F4);
        else if (compare(str, len, "F5", CASE_SENSETIVE)) keyboard::pressKey(KEY_F5);
        else if (compare(str, len, "F6", CASE_SENSETIVE)) keyboard::pressKey(KEY_F6);
        else if (compare(str, len, "F7", CASE_SENSETIVE)) keyboard::pressKey(KEY_F7);
        else if (compare(str, len, "F8", CASE_SENSETIVE)) keyboard::pressKey(KEY_F8);
        else if (compare(str, len, "F9", CASE_SENSETIVE)) keyboard::pressKey(KEY_F9);
        else if (compare(str, len, "F10", CASE_SENSETIVE)) keyboard::pressKey(KEY_F10);
        else if (compare(str, len, "F11", CASE_SENSETIVE)) keyboard::pressKey(KEY_F11);
        else if (compare(str, len, "F12", CASE_SENSETIVE)) keyboard::pressKey(KEY_F12);
        else if (compare(str, len, "SPACE", CASE_SENSETIVE)) keyboard::pressKey(KEY_SPACE);
        else if (compare(str, len, "PAUSE", CASE_SENSETIVE) || compare(str, len, "BREAK", CASE_SENSETIVE)) keyboard::pressKey(KEY_PAUSE);
        else if (compare(str, len, "CAPSLOCK", CASE_SENSETIVE)) keyboard::pressKey(KEY_CAPSLOCK);
        else if (compare(str, len, "NUMLOCK", CASE_SENSETIVE)) keyboard::pressKey(KEY_NUMLOCK);
        else if (compare(str, len, "PRINTSCREEN", CASE_SENSETIVE)) keyboard::pressKey(KEY_SYSRQ);
        else if (compare(str, len, "SCROLLLOCK", CASE_SENSETIVE)) keyboard::pressKey(KEY_SCROLLLOCK);

        // Modifiers
        else if (compare(str, len, "CTRL", CASE_SENSETIVE) || compare(str, len, "CONTROL", CASE_SENSETIVE)) keyboard::pressModifier(KEY_MOD_LCTRL);
        else if (compare(str, len, "SHIFT", CASE_SENSETIVE)) keyboard::pressModifier(KEY_MOD_LSHIFT);
        else if (compare(str, len, "ALT", CASE_SENSETIVE)) keyboard::pressModifier(KEY_MOD_LALT);
        else if (compare(str, len, "WINDOWS", CASE_SENSETIVE) || compare(str, len, "GUI", CASE_SENSETIVE)) keyboard::pressModifier(KEY_MOD_LMETA);

        // Utf8 character
        else keyboard::press(str);
    }

    void release() {
        keyboard::release();
    }

    unsigned int toInt(const char* str, size_t len) {
        if (!str || (len == 0)) return 0;

        unsigned int val = 0;

        // HEX
        if ((len > 2) && (str[0] == '0') && (str[1] == 'x')) {
            for (size_t i = 2; i < len; ++i) {
                uint8_t b = str[i];

                if ((b >= '0') && (b <= '9')) b = b - '0';
                else if ((b >= 'a') && (b <= 'f')) b = b - 'a' + 10;
                else if ((b >= 'A') && (b <= 'F')) b = b - 'A' + 10;

                val = (val << 4) | (b & 0xF);
            }
        }
        // DECIMAL
        else {
            for (size_t i = 0; i < len; ++i) {
                if ((str[i] >= '0') && (str[i] <= '9')) {
                    val = val * 10 + (str[i] - '0');
                }
            }
        }

        return val;
    }

    void sleep(unsigned long time) {
        unsigned long offset = millis() - interpretTime;

        if (time > offset) {
            sleepStartTime = millis();
            //sleepTime      = time - offset;
            sleepTime      = time - offset + sleepStartTime;

            //delay(sleepTime);
            while(millis() < sleepTime)
            {
                webserver::update();
            }
        }
    }

    // ====== PUBLIC ===== //

    void parse(const char* str, size_t len) {
        processing = true;
        interpretTime = millis();

        // Split str into a list of lines
        line_list* l = parse_lines(str, len);

        // Go through all lines
        line_node* n = l->first;

        // Flag, no default delay after this command
        bool ignore_delay;

        while (n) {
            ignore_delay = false;

            word_list* wl  = n->words;
            word_node* cmd = wl->first;

            const char* line_str = cmd->str + cmd->len + 1;
            size_t line_str_len  = n->len - cmd->len - 1;

            char last_char = n->str[n->len];
            bool line_end  = last_char == '\r' || last_char == '\n';

            // REM (= Comment -> do nothing)
            if (inComment || compare(cmd->str, cmd->len, "REM", CASE_SENSETIVE)) {
                inComment    = !line_end;
                ignore_delay = true;
            }

            // LOCALE (-> change keyboard layout)
            else if (compare(cmd->str, cmd->len, "LOCALE", CASE_SENSETIVE)) {
                word_node* w = cmd->next;

                if (compare(w->str, w->len, "US", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_us);
                } else if (compare(w->str, w->len, "DE", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_de);
                } else if (compare(w->str, w->len, "RU", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_ru);
                } else if (compare(w->str, w->len, "GB", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_gb);
                } else if (compare(w->str, w->len, "ES", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_es);
                } else if (compare(w->str, w->len, "FR", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_fr);
                } else if (compare(w->str, w->len, "DK", CASE_SENSETIVE)) {
                    keyboard::setLocale(&locale_dk);
                }
                ignore_delay = true;
            }

            // DELAY (-> sleep for x ms)
            else if (compare(cmd->str, cmd->len, "DELAY", CASE_SENSETIVE)) {
                sleep(toInt(line_str, line_str_len));
                ignore_delay = true;
            }

            // DEFAULTDELAY/DEFAULT_DELAY (set default delay per command)
            else if (compare(cmd->str, cmd->len, "DEFAULTDELAY", CASE_SENSETIVE) || compare(cmd->str, cmd->len, "DEFAULT_DELAY", CASE_SENSETIVE)) {
                defaultDelay = toInt(line_str, line_str_len);
                ignore_delay = true;
            }

            // REPEAT (-> repeat last command n times)
            else if (compare(cmd->str, cmd->len, "REPEAT", CASE_SENSETIVE) || compare(cmd->str, cmd->len, "REPLAY", CASE_SENSETIVE)) {
                repeatNum    = toInt(line_str, line_str_len) + 1;
                ignore_delay = true;
            }

            // STRING (-> type each character)
            else if (inString || compare(cmd->str, cmd->len, "STRING", CASE_SENSETIVE)) {
                if (inString) {
                    type(n->str, n->len);
                } else {
                    type(line_str, line_str_len);
                }

                inString = !line_end;
            }

            // LED
            else if (compare(cmd->str, cmd->len, "LED", CASE_SENSETIVE)) {
                word_node* w = cmd->next;

                int c[3];

                for (uint8_t i = 0; i<3; ++i) {
                    if (w) {
                        c[i] = toInt(w->str, w->len);
                        w    = w->next;
                    } else {
                        c[i] = 0;
                    }
                }

                led::setColor(c[0], c[1], c[2]);
            }

            // KEYCODE
            else if (compare(cmd->str, cmd->len, "KEYCODE", CASE_SENSETIVE)) {
                word_node* w = cmd->next;
                if (w) {
                    keyboard::report k;

                    k.modifiers = (uint8_t)toInt(w->str, w->len);
                    k.reserved  = 0;
                    w           = w->next;

                    for (uint8_t i = 0; i<6; ++i) {
                        if (w) {
                            k.keys[i] = (uint8_t)toInt(w->str, w->len);
                            w         = w->next;
                        } else {
                            k.keys[i] = 0;
                        }
                    }

                    keyboard::send(&k);
                    keyboard::release();
                }
            }

            // Otherwise go through words and look for keys to press
            else {
                word_node* w = wl->first;

                while (w) {
                    press(w->str, w->len);
                    w = w->next;
                }

                if (line_end) release();
            }

            n = n->next;

            if (!inString && !inComment && !ignore_delay) sleep(defaultDelay);

            if (line_end && (repeatNum > 0)) --repeatNum;

            interpretTime = millis();
        }

        line_list_destroy(l);
        processing = false;
    }

    int getRepeats() {
        return repeatNum;
    }

    unsigned int getDelayTime() {
        unsigned long finishTime  = sleepStartTime + sleepTime;
        unsigned long currentTime = millis();

        if (currentTime > finishTime) {
            return 0;
        } else {
            unsigned long remainingTime = finishTime - currentTime;
            return (unsigned int)remainingTime;
        }
    }
    bool isProcessing()
    {
        return processing;
    }
}
