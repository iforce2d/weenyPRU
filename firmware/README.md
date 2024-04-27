# weenyPRU firmware

This firmware targets a STM32F103C8T6 (72MHz clock, 64kb flash, 20kb RAM). It will probably also work on other F1 chips with equal or greater spec.

The project was created with STM32CubeIDE so it should be importable with that IDE. If this does not work, let me know if there are any missing files I need to add.

Note that a debug build will not be fast enough for high step rates, and will probably cause the SPI connection with the RPi to drop out frequently.

## Hardware pins

You can allocate the digital I/O pins between input and output by editing the 'digitalIns' and 'digitalOuts' arrays in tasks.c, see the comments there for details.

In v1.2 support was added for UART control of TMC2209 stepper drivers, and a XGZP pressure sensor over I2C.

To use UART control of the onboard TMC drivers:
- define USE_UART_TMC in config.h
- close the 'UART' jumper to connect D12 to the UART pins of the drivers

To enable XGZP pressure sensor:
- define USE_I2C_XGZP in config.h
- close ***both*** 'I2C' jumpers to connect D13 and D14 to pullup resistors

When these are enabled, naturally the related pins can no longer be used for regular digital I/O.

See the [main page](../README.md) for further info on TMC2209 setup.


## Pre-built binaries

The 'legacy' pre-built binary was made before TMC2209 UART support was added, and will only function correctly on a pre-v1.2 board. It shares the 14 digital I/O equally between input and output. That is, pins D1-D7 are inputs, and D8-D14 are outputs.

The 'v1.2' pre-built binary enables both UART TMC and I2C XGZP. 

## Pin mappings

### SPI
<table>
<tr><td>PB15</td><td>MOSI</td></tr>
<tr><td>PB14</td><td>MISO</td></tr>
<tr><td>PB13</td><td>CLK</td></tr>
<tr><td>PB12</td><td>NSS</td></tr>
</table>

### Status LED
<table>
<tr><td>PC13</td><td>Connection LED (lights when pulled low)</td></tr>
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
<tr><td>PB2</td><td>D1 (pulled up)</td></tr>
<tr><td>PB10</td><td>D2 (pulled up)</td></tr>
<tr><td>PB11</td><td>D3 (pulled up)</td></tr>
<tr><td>PA11</td><td>D4 (pulled up)</td></tr>
<tr><td>PA9</td><td>D5 </td></tr>
<tr><td>PA10</td><td>D6 </td></tr>
<tr><td>PA12</td><td>D7 </td></tr>
</table>

### Digital outputs
<table>
<tr><td>PA8</td><td>PWM</td></tr>

<tr><td> PA15 </td><td>D8</td></tr>
<tr><td>PB3</td><td>D9 </td></tr>
<tr><td>PB4</td><td>D10 </td></tr>
<tr><td>PB7</td><td>D11 </td></tr>
<tr><td>PB6</td><td>D12 or TMC stepper control</td></tr>
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

