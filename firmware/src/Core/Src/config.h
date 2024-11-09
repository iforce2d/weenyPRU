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
//#define USE_LOADCELL		// Use PB3,PB4 (alt SPI1 CLK,MISO) to communicate with a HX711/HX717. PB3,PB4 will no longer available for digital I/O

// HX71x and RGB LEDs cannot both use SPI1, prioritize load cell
#if (defined(USE_LOADCELL) && defined(USE_RGBLED))
#error Cannot use LOADCELL and RGBLED together! Disable one of them.
// Reason: HX71x requires clocking data out at about 1MHz, with the high/low duration of each pulse not exceeding 50us.
// This can be done with regular GPIO which has the advantage of being able to use any pin. Unfortunately there are too
// many interrupts going on (base thread takes 12us which at 50kHz is about half of the entire time available!). Time
// spent inside the HAL_SPI_TxRxCpltCallback for RPi communication is also quite long. When these coincide with loadcell
// reading, the loadcell values will become garbage. One workaround would be to check how long the loadcell read took,
// and if it was more than the expected time by some amount, ignore that reading. That's not ideal because the 'certain
// amount' is not well defined and may need updating in future, and missing readings is not desirable either.
//
// Another workaround is to use an SPI with DMA so that the clock pulses will always be well formed regardless of other
// jobs going on. The drawback is that (on F103C8 at least) we cannot run RGB LEDs at the same time. The pins don't
// conflict (RGB uses only MOSI and loadcell uses only CLK and MISO) but the clock speeds need to be different. I suppose
// it might be possible to reconfigure the SPI every time before communicating with each, but overall the loss of RGB
// LEDs doesn't seem like a big deal given how much further this already wacky workaround would need to be pushed. In
// future using a higher spec MCU will make this a moot point anyway. So for now, it's either loadcell or RGB LEDs.
#endif

#endif // SRC_CONFIG_H_
