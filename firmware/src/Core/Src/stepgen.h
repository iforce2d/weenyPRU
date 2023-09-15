#ifndef SRC_STEPGEN_H_
#define SRC_STEPGEN_H_

#include "main.h"
#include <stdbool.h>

typedef struct {

	int jointNumber;
    GPIO_TypeDef* dirPort;
	uint16_t dirPin;
	uint32_t stepTimerChannel;

    volatile int32_t *ptrFrequencyCommand; 	// pointer to the data source where to get the frequency command
    volatile int32_t *ptrFeedback;       	// pointer where to put the feedback

    int32_t DDSaccumulator;       			// Direct Digital Synthesis accumulator

} Stepgen;

void initStepgen(
		Stepgen* sg,
		int jointNumber,
		GPIO_TypeDef* dirPort,
		uint16_t dirPin,
		uint32_t stepTimerChannel);
void prepStep(Stepgen* sg);

#endif /* SRC_STEPGEN_H_ */
