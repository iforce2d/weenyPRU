# 'weeny' PRU for LinuxCNC on Raspberry Pi

This is a LinuxCNC component and firmware that uses the SPI bus on a Raspberry Pi to communicate with a STM32 microcontroller as a programmable realtime unit, for motor control, digital I/O, spindle PWM etc.

This project was inspired by and uses some code from the 'Remora' system [https://github.com/scottalford75/Remora](https://github.com/scottalford75/Remora)

The firmware for the realtime unit targets STM32F103C8 aka 'Blue Pill'. Source code includes a project file for STM32CubeIDE.

Unlike Remora, this firmware is not conveniently configurable by SD card, so all modifications will need to be changed in the source code.

Tested on Raspberry Pi 3B, 3B+, 4B and ZeroW with 4.19.71 PREEMPT_RT, LinuxCNC 2.8.4

Tested on Raspberry 4B with 6.6.44 PREEMPT_RT, LinuxCNC 2.9.3 (see setupStepsPi4.txt)

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
- XGZP (I2C) pressure sensor reading


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

On a v1.2 board, Trinamic TMC2209 stepper drivers can be controlled via UART to alter microstep and RMS current settings. To do this, bridge the 'UART' jumper. This also requires that UART control is enabled in the firmware (which is the default for the pre-built 'v1.2' binary).

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

## About PWM output for spindle

The 'weeny.spindle.set-speed' pin can be assigned a value between 0-65535 to control a 10kHz duty cycle pulse on the PWM pin, where 65535 means the pulse will be fully on. Unfortunately it's only a 3.3V signal though, so it has limited usefulness for controlling a VFD where commonly a voltage of 0-10V represents the full speed range. You would also want some smoothing of the pulse (eg. a capacitor at least) which will lower the pulse voltage even further. Some VFDs allow for re-assigning the end points, so perhaps a lower max voltage might be still be feasible, but see below about using a digital potentiometer for this purpose.

<br>

## About I2C digital potentiometer (DS3502)

On a v1.2 board, you can control a [DS3502](https://www.adafruit.com/product/4286) I2C digital potentiometer, which can be used to control the spindle speed on some VFDs. To do this, bridge ***both*** the 'I2C' jumpers, which will connect pullups to pins D13 and D14. This also requires that DS3502 usage is enabled in the firmware (in config.h). The output value of the potentiometer will be the same as for the PWM output (but with only 128 actual steps of resolution).

You can then connect the DS3502 sensor with:

 - D13 --> SCL
 - D14 --> SDA

<br>

## About I2C pressure sensor

On a v1.2 board, you can read values from a XGZP I2C pressure sensor. To do this, bridge ***both*** the 'I2C' jumpers, which will connect pullups to pins D13 and D14. This also requires that XGZP usage is enabled in the firmware (in config.h).

You can then connect your XGZP sensor with:

 - D13 --> SCL
 - D14 --> SDA

The pressure value will appear as the HAL pin 'weeny.pressure.0' in units of kPa, with update rate of about 100Hz.

<br>

## LinuxCNC component

To install the LinuxCNC component, invoke halcompile from the components/weeny folder like this:

`sudo halcompile --install weeny.c`

These pins are created by the component:

- weeny.input.00 - weeny.input.15
- weeny.output.00 - weeny.output.15
- weeny.analogN.raw
- weeny.analogN.jogcounts
- weeny.encoder.jogcounts
- weeny.spindle.set-speed
- weeny.rgb.led00.red / green / blue
- weeny.tmc.N.microsteps
- weeny.tmc.N.current
- weeny.pressure.0

See [components](../components/README.md) for more info.

<br>



<br>
<br>
<br>
<br>
<br>

