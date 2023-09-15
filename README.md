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
- 14 digital I/O (allocate these to input and output as desired)
- two analog inputs for joystick jogging
- one rotary encoder (slow for manual jogging etc, not for closed loop motor control!)
- WS2812 style RGB LED strip (up to 16) controllable from HAL

<br>

## About step pulse length

The default step pulse length (the time the pin is held high) is set to 4 microseconds. You can change this in the call to __HAL_TIM_SET_COMPARE in stepgen.c if your stepper drivers require a longer pulse. Keep in mind that at 50kHz there is only 20 microseconds available for a full pulse cycle.

<br>

## About analog input 

The analog inputs are intended for use as a joystick for jogging two axes. To use this correctly, the joystick should not be moved for 10 seconds on startup, to calibrate the center position value. There is a sizeable deadband enforced so that small jitters around the center position do not cause joint movement, but it would still be wise to use a switch to only enable joystick jogging when actually intended.

<br>

## About connection status LED

Pin PC13 is used to indicate that SPI connection to Raspberry Pi is established. This LED should light up when you remove e-stop, and remain lit while SPI connection is active.

<br>

## About RGB LEDs

The WS2812 protocol is used to control up to 16 addressable RGB LEDs on PB5. Although these are not officially supposed to work with 3.3V signals, in my experience many variants do actually work just fine. You can control the individual red/green/blue components of each LED (simply on or off, no gradual dimming) via HAL pins. Note that when SPI connection is lost, all RGB LEDs will also turn off.

<br>

## About debug builds

The STM32F103C8 seems to struggle with this workload when running a debug build, and you may find the SPI connection occasionally dropping. With a release build though, I've had it running fine for 3 weeks at a time without any loss of communication.

<br>

## LinuxCNC component

To install the LinuxCNC component, invoke halcompile from the components/weeny folder like this:

`sudo halcompile --install weeny.c`

<br>

## HAL pins

Note that even though the default firmware only has a total of 14 pins for digital I/O, there are 16 digital pins each for input and output in the HAL settings. This is mainly because the messaging between Raspberry Pi and the PRU uses uint16 types to hold these values. In future the extra 2 bits might be useful if for example, two more digital I/O were needed instead of a rotary encoder.

### weeny.input.00 - weeny.input.15

Returns the value of digital inputs (as assigned in tasks.c)

### weeny.output.00 - weeny.output.15

Sets the value of digital outputs (as assigned in tasks.c)

### weeny.analogN.raw

Returns the raw (floating point 0-1) value on PB0/PB1

### weeny.analogN.jogcounts

Returns a value that will be incremented or decremented by moving the analog input on PB0/PB1 away from center. The speed of change will be faster when further from center.

### weeny.encoder.jogcounts

Returns a value that will be incremented or decremented by moving a rotary encoder on PC14/PC15

### weeny.spindle.set-speed

Sets the duty cycle of the PWM pulse on PA8. This takes a value from 0-65535, where 65535 will result in an always-high pulse.

### weeny.rgb.led00.red / green / blue

Sets the red/green/blue component to on or off in the RGB LED of the given index.

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
<tr><td>PB5</td><td>WS2812 style LED strip</td></tr>
</table>



<br>
<br>
<br>
<br>
<br>

