# weenyPRU LinuxCNC component

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

### weeny.pressure.0

Returns the pressure sensor reading in kPa.

### weeny.loadcell.0

Returns the load cell reading as a raw signed 24 bit value (range −8,388,608 to 8,388,607).

<br>
