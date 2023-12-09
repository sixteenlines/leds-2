#include <macros.hpp>
#include <leds.hpp>
#include <webserver.hpp>
#include <filesystem.hpp>
#include <Arduino.h>

// loop control
extern bool wmanager;
extern uint8_t mode;

extern uint8_t r;
extern uint8_t g;
extern uint8_t b;

void initIO() {
    Serial.begin(115200);
    Serial.println();
}

void setup() {
    initIO();
    initFS();
    initLeds();

    switch (initWifi()) {
    case 0:
        Serial.println("[\e[0;31mFAILED\e[0;37m] Initializing WiFi");
        break;
    case 1:
        Serial.println("[\e[0;32m  OK  \e[0;37m] Initializing WiFi");
        break;
    default:
        Serial.println("[\e[0;32m  OK  \e[0;37m] Hosted Access Point");
        wmanager = true;
        break;
    }
    mode = _DEFAULT;
    setLeds(120, 120, 120);
}

void loop() {
    if (wmanager) {
        dnsNext();
    }
    if (mode == _AQUA) {
        EVERY_N_MILLISECONDS(20) {
            pacifica_loop();
            FastLED.show();
        }
    }
    yield();
}