#ifndef SRC_RGBLED_H_
#define SRC_RGBLED_H_

void initRGBLED();
void turnOffAllRGBLEDs();
void setRGBLedColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
void doRGBLEDOutput();

#endif /* SRC_RGBLED_H_ */
