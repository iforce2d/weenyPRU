# 'weeny' PRU for LinuxCNC on Raspberry Pi

This is a LinuxCNC component and firmware that uses the SPI bus on a Raspberry Pi to communicate with a STM32 microcontroller as a programmable realtime unit, for motor control, digital I/O, spindle PWM etc.

This project was inspired by and uses some code from the 'Remora' system [https://github.com/scottalford75/Remora](https://github.com/scottalford75/Remora)

The firmware for the realtime unit targets STM32F103C8 aka 'Blue Pill'. Source code includes a project file for STM32CubeIDE.

Unlike Remora, this firmware is not conveniently configurable by SD card, so all modifications will need to be changed in the source code.

Tested on Raspberry Pi 3B and ZeroW with 4.19.71 PREEMPT, LinuxCNC 2.8.4

<br>

## Features

- 4 axis stepper motor control at up to 50kHz step rate  
- 4 μs step pulse by default
- 10kHz spindle PWM frequency
- two analog inputs for joystick jogging
- one rotary encoder (slow for manual jogging etc), not for closed loop motor control
- WS2812 style RGB LED strip (up to 16) controllable from HAL

<br>

## About analog input 

The analog inputs are intended for use as a joystick for jogging two axes. To use this correctly, the joystick should not be moved for 10 seconds on startup, to calibrate the center position value. There is a sizeable deadband enforced so that small jitters around the center position do not cause joint movement, but it would still be wise to use a switch to only enable joystick jogging when actually intended.

<br>

## LinuxCNC component

To install the LinuxCNC component, invoke halcompile from the components/weeny folder like this:

`sudo halcompile --install weeny.c`

<br>

## HAL pins

### weeny.input.00 - weeny.input.15

Returns the value of digital inputs (as assigned in tasks.c). Note that only a total of 14 pins exist for digital I/O

### weeny.output.00 - weeny.output.15

Sets the value of digital outputs (as assigned in tasks.c). Note that only a total of 14 pins exist for digital I/O

### weeny.analog0.jogcounts

Returns a value that will be incremented or decremented by moving the analog input on PB0 away from center

### weeny.analog1.jogcounts

Returns a value that will be incremented or decremented by moving the analog input on PB1 away from center

### weeny.encoder.jogcounts

Returns a value that will be incremented or decremented by moving a rotary encoder on PC14/PC15

### weeny.spindle.set-speed

Sets the duty cycle of the PWM pulse on PA8. This takes a value from 0-65535, where 65535 will result in an always-high pulse.

<br>

## Hardware pins

The default layout shares the 14 digital I/O equally between input and output. You can assign the digital pins in some other arrangement by editing tasks.c (move pins between the digitalIns and digitalOuts arrays).

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
<tr><td>PB6</td><td>D11 </td></tr>
<tr><td>PB7</td><td>D12 </td></tr>
<tr><td>PB8</td><td>D13 </td></tr>
<tr><td>PB9</td><td>D14 </td></tr>
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
<tr><td>PB5</td><td>WS2812 style LED strip, up to 16 LEDs</td></tr>
</table>



<br>
<br>
<br>
<br>
<br>

