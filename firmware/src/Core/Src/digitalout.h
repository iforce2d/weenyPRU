#ifndef SRC_DIGITALOUT_H_
#define SRC_DIGITALOUT_H_

#include "main.h"
#include <stdbool.h>

typedef struct {

	uint8_t mask;
    GPIO_TypeDef* port;
	uint16_t pin;

} DigitalOut;

void initDigitalOut(
		DigitalOut* sg,
		int number,
		GPIO_TypeDef* port,
		uint16_t pin);
void updateDigitalOut(DigitalOut* sg);

#endif /* SRC_DIGITALOUT_H_ */
