# weenyPRU firmware

This firmware targets a STM32F103C8T6 (72MHz clock, 64kb flash, 20kb RAM). It will probably also work on other F1 chips with equal or greater flash and RAM spec.

The project was created with STM32CubeIDE so it should be importable with that IDE. If this does not work, let me know if there are any missing files I need to add.

Note that a debug build will not be fast enough for high step rates, and will also probably cause the SPI connection with the RPi to drop out frequently.

## Hardware pins

The 'TMC2209' pre-built binary dedicates PB6 as a UART TX pin and is what the current source code will build. The pin layout below is for this version.

The 'default' pre-built binary is a legacy build made before TMC2209 UART support was added. It shares the 14 digital I/O equally between input and output. You can share these pins differently between input and output by editing tasks.c (move pins between the digitalIns and digitalOuts arrays). The pin layout below also applies for this version except that PB6-PB9 map to D11-D14.

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
<tr><td>PA7 </td><td>Step 4</td></tr>
</table>

### Digital inputs
<table>
<tr><td>PB2</td><td>D1 </td></tr>
<tr><td>PB10</td><td>D2 </td></tr>
<tr><td>PB11</td><td>D3 </td></tr>
<tr><td>PA11</td><td>D4 </td></tr>
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
<tr><td>PB8</td><td>D12 </td></tr>
<tr><td>PB9</td><td>D13 </td></tr>
</table>

### UART
<table>
<tr><td>PB6</td><td>TMC stepper control (one-wire)</td></tr>
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
<tr><td>PB5</td><td>WS2812 style LED strip</td></tr>
</table>

