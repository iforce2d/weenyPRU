# 'weeny' PRU for LinuxCNC on Raspberry Pi

This is a LinuxCNC component and firmware that uses the SPI bus on a Raspberry Pi to communicate with a STM32 microcontroller as a programmable realtime unit, for motor control, digital I/O, spindle PWM etc.

This project was inspired by and uses some code from the 'Remora' system [https://github.com/scottalford75/Remora](https://github.com/scottalford75/Remora)

The firmware for the realtime unit targets STM32F103C8 aka 'Blue Pill'. Source code includes a project file for STM32CubeIDE.

Unlike Remora, this firmware is not conveniently configurable by SD card, so all modifications will need to be changed in the source code.

Tested on Raspberry Pi 3B, 3B+ and ZeroW with 4.19.71 PREEMPT, LinuxCNC 2.8.4

<br>

## Features

- 4 axis stepper motor control at up to 50kHz step rate
- 10kHz spindle PWM frequency output (3.3V)
- 4 opto-coupler ready inputs for high-voltage limit switches
- 9 other digital I/O (allocate these to input and output as desired, 3.3V level)
- two analog inputs for joystick jogging
- one rotary encoder (slow for manual jogging etc, not for closed loop motor control!)
- WS2812 style RGB LED strip (up to 16) controllable from HAL
- TMC2209 stepper driver control from HAL (microstep and current)


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

## About TMC2209 UART control

Trinamic TMC2209 stepper drivers can be controlled via UART to alter microstep and RMS current settings. To do this:

- (v1.1 board) connect D11 to any of the 'UART' pins (side nearest the stepper driver), and add a 1k resistor pullup to VCC.
- (v1.2 board) bridge the 'UART' jumper

The default values are 8x microstepping and 200mA current. Changes are made by setting pin values, eg.

    setp  weeny.tmc.0.microsteps  16
    setp  weeny.tmc.0.current     400

Bridge the MS1/MS2 pins to set the address of each driver as desired. The example above will affect all drivers with both MS1 and MS2 left open (address 0).

Microsteps should be one of 0, 2, 4, 8, 16, 32, 63, 128, 256

Current should be a value between 0-2000 (milliamps). Note that this is an approximate setting, as the TMC2209 can only control current in 32 distinct steps and will choose whatever is the closest value it can actually perform.

Changes will be relayed to the drivers at a rate of once per second.

This was tested with these drivers, which worked out of the box without needing any solder pads bridged. I also tried another variant which ignored all attempts to change settings via UART (and didn't appear to have any empty solder pads to bridge), so for now I can only recommend this MKS one.

![alt text](https://www.iforce2d.net/tmp/mks.png)

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

### weeny.tmc.N.microsteps

Sets the microstep resolution for a TMC2209 stepper driver at address N. Values other than 0, 2, 4, 8, 16, 32, 63, 128, 256 will be ignored.

### weeny.tmc.N.current

Sets the (approximate) RMS current in milliamps for a TMC2209 stepper driver at address N. Values outside the range 0-2000 will be ignored.

<br>

<br>
<br>
<br>
<br>
<br>

