#ifndef SRC_STEPGEN_H_
#define SRC_STEPGEN_H_

#include "main.h"
#include <stdbool.h>

typedef struct {

	int jointNumber;
    GPIO_TypeDef* dirPort;
	uint16_t dirPin;
	uint32_t stepTimerChannel;
    int32_t DDSaccumulator;       			// Direct Digital Synthesis accumulator
    int32_t rawCount;

} Stepgen;

void initStepgen(
		Stepgen* sg,
		int jointNumber,
		GPIO_TypeDef* dirPort,
		uint16_t dirPin,
		uint32_t stepTimerChannel);
void prepStep(Stepgen* sg);

#endif /* SRC_STEPGEN_H_ */
