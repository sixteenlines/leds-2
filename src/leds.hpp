#ifndef _FASTLED_H
#define _FASTLED_H
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>
#endif

#ifndef _ARDUINO_H
#define _ARDUINO_H
#include <Arduino.h>
#endif

#ifndef _LEDS_HPP
#define _LEDS_HPP
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#define NUM_LEDS 100
#define LED_PIN 5
#define BRIGHTNESS 255
#define SATURATION 255

void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
void pacifica_add_whitecaps();
void pacifica_deepen_colors();
void pacifica_loop();

void initLeds();
void setLeds(uint8_t r, uint8_t g, uint8_t b);

#endif
