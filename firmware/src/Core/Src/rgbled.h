#ifndef SRC_RGBLED_H_
#define SRC_RGBLED_H_

#include "config.h"

#ifndef USE_LOADCELL

void initRGBLED();
void turnOffAllRGBLEDs();
void setRGBLedColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void updateRGBLEDs();
void doRGBLEDOutput();

#endif

#endif /* SRC_RGBLED_H_ */
