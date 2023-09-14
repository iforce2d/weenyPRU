
#include "comms.h"
#include "stepgen.h"

extern TIM_HandleTypeDef htim2;

#define PRU_BASEFREQ 	50000  // Hz
#define PRU_SERVOFREQ	 1000  // Hz

#define STEPBIT     		((int32_t)22)            	// bit location in DDS accumulator
#define STEP_MASK   		(1L<<STEPBIT)

const float frequencyScale = (float)(1 << STEPBIT) / (float)PRU_BASEFREQ;

void initStepgen(
		Stepgen* sg,
		int jointNumber,
		bool reversed,
		GPIO_TypeDef* dirPort,
		uint16_t dirPin,
		uint32_t stepTimerChannel)
{
	sg->jointNumber = jointNumber;
	sg->reversed = reversed;
	sg->dirPort = dirPort;
	sg->dirPin = dirPin;
	sg->stepTimerChannel = stepTimerChannel;
	sg->ptrFrequencyCommand = &rxData.jointFreqCmd[jointNumber];
	sg->ptrFeedback = &txData.jointFeedback[jointNumber];
	sg->DDSaccumulator = 0;
}

void prepStep(Stepgen* sg)
{
	__HAL_TIM_SET_COMPARE(&htim2, sg->stepTimerChannel, 0);

	if ( rxData.jointEnable & (1 << sg->jointNumber) ) {

		int32_t frequencyCommand = *(sg->ptrFrequencyCommand);      // Get the latest frequency command via pointer to the data source
		int32_t	DDSaddValue = frequencyCommand * frequencyScale;	// Scale the frequency command to get the DDS add value
		int32_t stepNow = sg->DDSaccumulator;                       // Save the current DDS accumulator value
		sg->DDSaccumulator += DDSaddValue;           	  			// Update the DDS accumulator with the new add value
		stepNow ^= sg->DDSaccumulator;                          	// Test for changes in the low half of the DDS accumulator
		stepNow &= (1L << STEPBIT);

		bool isForward = sg->reversed ? (DDSaddValue < 0) : (DDSaddValue > 0);	// The sign of the DDS add value indicates the desired direction
		HAL_GPIO_WritePin(sg->dirPort, sg->dirPin, isForward ? GPIO_PIN_SET : GPIO_PIN_RESET);

		if (stepNow) {
			__HAL_TIM_SET_COMPARE(&htim2, sg->stepTimerChannel, 4); // 4 microsecond pulse width
			*(sg->ptrFeedback) = sg->DDSaccumulator;                // Update position feedback via pointer to the data receiver
		}
	}

}

