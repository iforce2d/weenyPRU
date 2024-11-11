#ifndef SRC_TMC2209_H_
#define SRC_TMC2209_H_

#include "main.h"

typedef struct TMC_UART {

	uint8_t address; // 0x00 - 0x03
	uint32_t gconf;
	uint32_t chopconf;

	//uint8_t  lastWrittenMicrostepsCode;	// the register value (eg. 5 for 8x microstepping)
	//uint16_t lastWrittenCurrent;			// 0-2000 (actual current)

	//uint8_t microstepWriteRepeatsRemaining;
	//uint8_t currentWriteRepeatsRemaining;

} TMC_UART;

void initTMCUART(TMC_UART* u, uint8_t address, uint8_t microsteps, uint16_t mA);

//uint8_t test_connection(TMC_UART* u);
void set_pdn_disable_and_mstep_reg_select(TMC_UART* u);
void set_microstep_code(TMC_UART* u, uint8_t code);
void set_microsteps(TMC_UART* u, uint16_t ms);
//uint16_t get_microsteps(TMC_UART* u);
void set_rms_current(TMC_UART* u, uint16_t c);

#endif /* SRC_TMC2209_H_ */
