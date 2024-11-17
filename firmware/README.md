# weenyPRU firmware

This firmware targets a STM32F103C8T6 (72MHz clock, 64kb flash, 20kb RAM). It will probably also work on other F1 chips with equal or greater spec.

The project was created with STM32CubeIDE so it should be importable with that IDE. If this does not work, let me know if there are any missing files I need to add.

Note that a debug build will not be fast enough for high step rates, and will probably cause the SPI connection with the RPi to drop out frequently.


## Feature selection

In config.h you can select which features you need. Disabling features you're not using will (slightly) improve timing accuracy. 

Some features when disabled will free up their related pin for use as a regular digital I/O, the silkscreen labe for these are listed alongside the feature here:

- USE_RGBLED (disable to use RGB as digital I/O)
- USE_ROTARY_ENCODER (disable to use CLK, DT as digital I/O)
- USE_ANALOG (disable to use ADC1, ADC2 as digital I/O)
- USE_UART_TMC
- USE_I2C_XGZP
- USE_I2C_DS3502
- USE_LOADCELL

Note that USE_LOADCELL will disable USE_RGBLED

## Hardware pins

You can allocate the digital I/O pins between input and output by editing the 'digitalIns' and 'digitalOuts' arrays in tasks.c. The LinuxCNC HAL pin that each digital I/O pin relates to will depend on the order pins are listed in those arrays. See the comments there for details.

In v1.2 support was added for UART control of TMC2209 stepper drivers, and a XGZP pressure sensor over I2C. In v1.3 support was added for a DS3502 digital potentiometer over I2C. In v1.4 support was added for a HX711 load cell amplifier, and timing accuracy was improved.

To use UART control of the onboard TMC drivers:
- define USE_UART_TMC in config.h
- close the 'UART' jumper to connect D12 to the UART pins of the TMC drivers

To enable XGZP pressure sensor:
- define USE_I2C_XGZP in config.h
- close ***both*** 'I2C' jumpers to connect D13 (SCL) and D14 (SDA) to pullup resistors

To enable DS3502 digital potentiometer:
- define USE_I2C_DS3502 in config.h
- close ***both*** 'I2C' jumpers to connect D13 (SCL) and D14 (SDA) to pullup resistors

To enable HX711 load cell:
- define USE_LOADCELL in config.h
- connect D9 (CLK) and D10 (DT) to the HX711

When these are enabled, naturally the related pins can no longer be used for regular digital I/O.

See the [main page](../README.md) for further info on TMC2209 setup.


## Pre-built binaries

See the [binaries page](bin/README.md) for further info.


## Pin mappings

### SPI2 connection to RPi
<table>
<tr><td>PB15</td><td>MOSI</td></tr>
<tr><td>PB14</td><td>MISO</td></tr>
<tr><td>PB13</td><td>CLK</td></tr>
<tr><td>PB12</td><td>NSS</td></tr>
</table>

### Status LED
<table>
<tr><td>PC13</td><td>Connection active LED (lights when low)</td></tr>
</table>

### Step, direction outputs
<table>
<tr><td>PA0</td><td>Dir 1</td></tr>
<tr><td>PA1</td><td>Dir 2</td></tr>
<tr><td>PA2 </td><td>Dir 3</td></tr>
<tr><td>PA3</td><td>Dir 4</td></tr>
<tr><td>PA4 </td><td>Step 1</td></tr>
<tr><td>PA5 </td><td>Step 2</td></tr>
<tr><td>PA6 </td><td>Step 3</td></tr>
<tr><td>PB5 </td><td>Step 4</td></tr>
</table>

### Digital inputs
<table>
<tr><td>PB2</td><td>D1 (pulled up by resistor on PCB)</td></tr>
<tr><td>PB10</td><td>D2 (pulled up by resistor on PCB)</td></tr>
<tr><td>PB11</td><td>D3 (pulled up by resistor on PCB)</td></tr>
<tr><td>PA11</td><td>D4 (pulled up by resistor on PCB)</td></tr>
<tr><td>PA9</td><td>D5 </td></tr>
<tr><td>PA10</td><td>D6 </td></tr>
<tr><td>PA12</td><td>D7 </td></tr>
</table>

### Digital outputs
<table>
<tr><td>PA15</td><td>D8</td></tr>
<tr><td>PB3</td><td>D9 or HX711 CLK</td></tr>
<tr><td>PB4</td><td>D10 or HX711 DT </td></tr>
<tr><td>PB7</td><td>D11 </td></tr>
<tr><td>PB6</td><td>D12 or TMC stepper UART control</td></tr>
<tr><td>PB8</td><td>D13 or I2C SCL</td></tr>
<tr><td>PB9</td><td>D14 or I2C SDA</td></tr>
</table>

### PWM output
<table>
<tr><td>PA8</td><td>PWM</td></tr>
</table>

### Analog input
<table>
<tr><td>PB0</td><td>Analog 1</td></tr>
<tr><td>PB1</td><td>Analog 2</td></tr>
</table>

### Encoder input
<table>
<tr><td>PC14</td><td>Data</td></tr>
<tr><td>PC15</td><td>Clock</td></tr>
</table>

### RGB LED output
<table>
<tr><td>PA7</td><td>WS2812 style LED strip</td></tr>
</table>

