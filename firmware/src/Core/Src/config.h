#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

// Disabling unnecessary devices here will make various parts of the code more efficient,
// especially for the I2C devices and analog inputs.

#define USE_RGBLED			// Use PA7 (SPI1 MOSI) to control WS2812 style RGB LEDs. PA7 will no longer available for digital I/O
#define USE_ROTARY_ENCODER	// Use PC14,PC15 for rotary encoder. PC14,PC15 will no longer available for digital I/O
#define USE_ANALOG			// Use PB0,PB1 for analog inputs. PB0,PB1 will no longer available for digital I/O
#define USE_UART_TMC		// Use PB6 for UART control of TMC2209 stepper drivers. PB6 will no longer available for digital I/O
#define USE_I2C_XGZP		// Use PB8,PB9 to read a XGZP pressure sensor over I2C. PB8,PB9 will no longer available for digital I/O
#define USE_I2C_DS3502		// Use PB8,PB9 to write to a DS3502 digipot over I2C. PB8,PB9 will no longer available for digital I/O
#define USE_LOADCELL		// Use PB3,PB4 (alt SPI1 CLK,MISO) to communicate with a HX711/HX717. PB3,PB4 will no longer available for digital I/O

// HX71x and RGB LEDs cannot both use SPI1, prioritize load cell
#ifdef USE_LOADCELL
	#undef USE_RGBLED
#endif

#endif // SRC_CONFIG_H_
