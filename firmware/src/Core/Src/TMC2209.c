
#include "TMC2209.h"

#define GCONF_ADDRESS			0x00
#define IHOLD_IRUN_ADDRESS		0x10
#define CHOPCONF_ADDRESS		0x6C
#define DRV_STATUS_ADDRESS		0x6F

#define GCONF_PDNDISABLE_MASK		0x00000040
#define GCONF_PDNDISABLE_POS		6

#define GCONF_MSTEPREGSELECT_MASK 	0x00000080
#define GCONF_MSTEPREGSELECT_POS		7

#define CHOPCONF_MRES_MASK 			0x0F000000
#define CHOPCONF_MRES_POS 			24

#define CHOPCONF_VSENSE_MASK 		0x00020000
#define CHOPCONF_VSENSE_POS 		17


#define IHOLD_IRUN_IRUN_MASK 		0x00001F00
#define IHOLD_IRUN_IRUN_POS 		8

#define IHOLD_IRUN_IHOLD_MASK 		0x0000001F
#define IHOLD_IRUN_IHOLD_POS 		0


#define GET_REG_VAL(reg, m) ((reg & m##_MASK) >> m##_POS)
#define SET_REG_BITS(reg, m, v) reg = (reg & ~(m##_MASK)) | ((v << m##_POS)&(m##_MASK))

#define TMC2208_SYNC		0x05

//#define TMC_READ 		0x00
#define TMC_WRITE_MASK		0x80

#define INVALID_REG_VALUE			0xFFFFFFFF
#define CHOPCONF_DEFAULT_VALUE		0x10010053

#define WRITE_REPEATS				3

extern UART_HandleTypeDef huart1;

void uartTransmit(uint8_t* buf, uint16_t size) {
	HAL_HalfDuplex_EnableTransmitter(&huart1);
	HAL_UART_Transmit(&huart1, buf, size, 5);
}

bool uartReceive(uint8_t* buf, uint16_t size) {
	HAL_HalfDuplex_EnableReceiver(&huart1);
	if ( HAL_OK != HAL_UART_Receive(&huart1, buf, size, 5) ) {
		return false;
	}
	return true;
}

uint8_t calcCRC(uint8_t* datagram, uint8_t len) {
	uint8_t crc = 0;
	for (int i = 0; i < len-1; i++) {
		uint8_t currentByte = datagram[i];
		for (int j = 0; j < 8; j++) {
			if ((crc >> 7) ^ (currentByte & 0x01)) {
				crc = (crc << 1) ^ 0x07;
			} else {
				crc = (crc << 1);
			}
			crc &= 0xff;
			currentByte = currentByte >> 1;
		}
	}
	return crc;
}

void write(TMC_UART* u, uint8_t addr, uint32_t regVal) {
	uint8_t len = 8;
	addr |= TMC_WRITE_MASK;
	uint8_t datagram[] = {TMC2208_SYNC, u->address, addr, (uint8_t)(regVal>>24), (uint8_t)(regVal>>16), (uint8_t)(regVal>>8), (uint8_t)(regVal>>0), 0x00};

	datagram[len-1] = calcCRC(datagram, len);

	uartTransmit(datagram, len);
}

//uint32_t read(TMC_UART* u, uint8_t register_addr) {
//	uint8_t len = 4;
//	uint8_t datagram[] = {TMC2208_SYNC, u->address, register_addr, 0x00};
//	datagram[len-1] = calcCRC(datagram, len);
//	uint64_t out = 0x00000000UL;
//
//	bool CRCerror = false;
//
//	for (uint8_t i = 0; i < 1; i++) {
//
//		uartTransmit(datagram, len);
//
//		uartReceive((uint8_t*)&out, 8);
//
//		CRCerror = false;
//
//		uint8_t crc = calcCRC((uint8_t*)&out, 8);
//		if ((crc != (uint8_t)(out>>56)) || crc == 0 ) {
//			CRCerror = true;
//			out = 0;
//		} else {
//			break;
//		}
//	}
//
//	if ( CRCerror )
//		return INVALID_REG_VALUE;
//
//	uint32_t ret = 0;
//	ret |= (uint8_t)(out>>24); ret <<= 8;
//	ret |= (uint8_t)(out>>32); ret <<= 8;
//	ret |= (uint8_t)(out>>40); ret <<= 8;
//	ret |= (uint8_t)(out>>48);
//
//	return ret;
//}

// run once at startup
void set_pdn_disable_and_mstep_reg_select(TMC_UART* u)	{
	SET_REG_BITS(u->gconf, GCONF_PDNDISABLE, 1);
	SET_REG_BITS(u->gconf, GCONF_MSTEPREGSELECT, 1);
	write(u, GCONF_ADDRESS, u->gconf);
//	uint32_t tmp = read(u, CHOPCONF_ADDRESS);
//	if ( tmp != INVALID_REG_VALUE )
//		u->chopconf = tmp;
//	else
		u->chopconf = CHOPCONF_DEFAULT_VALUE;
}

//uint8_t test_connection(TMC_UART* u) {
//	uint32_t drv_status = read(u, DRV_STATUS_ADDRESS);
//	switch (drv_status) {
//	  case INVALID_REG_VALUE: return 1;
//	  case 0: return 2;
//	  default: return 0;
//	}
//}

void set_microstep_code( TMC_UART* u, uint8_t  v )	{

/* This code was to only write a few times after a value was actually changed
 * to avoid unnecessary messages, but I think the TMC drivers can handle being
 * updated at the 1Hz rate without any problems, so let's make it continuous.
 * This also helps to avoid missing messages, or not starting up correctly.
	if ( v != u->lastWrittenMicrostepsCode )
		u->microstepWriteRepeatsRemaining = WRITE_REPEATS;

	if ( u->microstepWriteRepeatsRemaining < 1 )
		return;

	u->microstepWriteRepeatsRemaining--;
	u->lastWrittenMicrostepsCode = v;
*/
	if ( v > 8 )
		return;

	SET_REG_BITS(u->chopconf, CHOPCONF_MRES, v);
	write(u, CHOPCONF_ADDRESS, u->chopconf);

}

void set_microsteps(TMC_UART* u, uint16_t msteps) {
	uint8_t code = 0xFF; // invalid
  switch(msteps) {
    case 256: code = 0; break;
    case 128: code = 1; break;
    case  64: code = 2; break;
    case  32: code = 3; break;
    case  16: code = 4; break;
    case   8: code = 5; break;
    case   4: code = 6; break;
    case   2: code = 7; break;
    case   0: code = 8; break;
    default: break;
  }
  if ( code != 0xFF )
	  set_microstep_code(u, code);
}

void set_vsense( TMC_UART* u, uint8_t  v )	{
	SET_REG_BITS(u->chopconf, CHOPCONF_VSENSE, v);
	write(u, CHOPCONF_ADDRESS, u->chopconf);
}

void set_irun_and_ihold( TMC_UART* u, uint8_t  R, uint8_t  H )	{
	uint32_t tmp = 0;
	SET_REG_BITS(tmp, IHOLD_IRUN_IRUN, R);
	SET_REG_BITS(tmp, IHOLD_IRUN_IHOLD, H);
	write(u, IHOLD_IRUN_ADDRESS, tmp);
}

void set_rms_current(TMC_UART* u, uint16_t mA) {

/* This code was to only write a few times after a value was actually changed
 * to avoid unnecessary messages, but I think the TMC drivers can handle being
 * updated at the 1Hz rate without any problems, so let's make it continuous.
 * This also helps to avoid missing messages, or not starting up correctly.
	if ( mA != u->lastWrittenCurrent )
		u->currentWriteRepeatsRemaining = WRITE_REPEATS;

	if ( u->currentWriteRepeatsRemaining < 1 )
		return;

	u->currentWriteRepeatsRemaining--;
	u->lastWrittenCurrent = mA;
*/
	float Rsense = 0.11f;
	float holdMultiplier = 0.5;

	uint8_t CS = 32.0*1.41421*mA/1000.0*(Rsense+0.02)/0.325 - 1;

	// If Current Scale is too low, turn on high sensitivity R_sense and calculate again
	if (CS < 16) {
		set_vsense(u, true);
		CS = 32.0*1.41421*mA/1000.0*(Rsense+0.02)/0.180 - 1;
	} else { // If CS >= 16, turn off high_sense_r
		set_vsense(u, false);
	}

	if (CS > 31)
		CS = 31;

	set_irun_and_ihold(u, CS, CS*holdMultiplier);
}

void initTMCUART(TMC_UART* u, uint8_t address, uint8_t microsteps, uint16_t mA)
{
	u->address = address;
	u->gconf = 0;
	u->chopconf = 0;
	//u->microstepWriteRepeatsRemaining = WRITE_REPEATS;
	//u->currentWriteRepeatsRemaining = WRITE_REPEATS;

	set_pdn_disable_and_mstep_reg_select(u);
	set_microsteps(u, microsteps);
	set_rms_current(u, mA);
}

