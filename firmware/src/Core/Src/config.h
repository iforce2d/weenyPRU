#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#define USE_UART_TMC		// Use PB6 for UART control of TMC2209 stepper drivers. PB6 will no longer available for digital I/O
//#define USE_I2C_XGZP		// Use PB8,PB9 to read a XGZP pressure sensor over I2C. PB8,PB9 will no longer available for digital I/O
#define USE_I2C_DS3502		// Use PB8,PB9 to write to a DS3502 digipot over I2C. PB8,PB9 will no longer available for digital I/O

#endif /* SRC_CONFIG_H_ */
