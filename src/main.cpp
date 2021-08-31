/*
   This software is licensed under the MIT License. See the license file for details.
   Source: https://github.com/spacehuhntech/WiFiDuck
 */

#include "config.h"
#include "debug.h"

#include "duckscript.h"
#include "webserver.h"
#include "spiffs.h"
#include "settings.h"
#include "cli.h"

#include "led.h"
#include "keyboard.h"
#include "duckparser.h"

void setup() {
    debug_init();
    Serial.begin(38400);
    keyboard::begin();
    delay(200);

    spiffs::begin();
    settings::begin();
    cli::begin();
    webserver::begin();
    led::begin();

    delay(10);

    debug("\n[~~~ WiFi Duck v");
    debug(VERSION);
    debugln(" Started! ~~~]");
    debugln("    __");
    debugln("___( o)>");
    debugln("\\ <_. )");
    debugln(" `---'   hjw\n");

    duckscript::run(settings::getAutorun());
}

void loop() {
    webserver::update();
    duckscript::nextLine();
    debug_update();
}
