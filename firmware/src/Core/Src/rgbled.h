#ifndef SRC_RGBLED_H_
#define SRC_RGBLED_H_

#include "config.h"

#ifdef USE_RGBLED

void initRGBLED();
void turnOffAllRGBLEDs();
void setRGBLedColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void updateRGBLEDs();
void doRGBLEDOutput();

#endif // USE_RGBLED

#endif // SRC_RGBLED_H_
