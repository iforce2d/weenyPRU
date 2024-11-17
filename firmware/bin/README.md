# weenyPRU firmware binaries

The 'legacy' pre-built binary was made before TMC2209 UART support was added, and will only function correctly on a pre-v1.2 board. It shares the 14 digital I/O equally between input and output. That is, pins D1-D7 are inputs, and D8-D14 are outputs.

The 'v1.2' pre-built binary enables UART TMC and XGZP. 

The 'v1.3' pre-built binary enables UART TMC, XGZP, and DS3502.

The 'v1.4' pre-built binary enables UART TMC, XGZP, DS3502 and HX711 (instead of RGB LEDs).

(These version numbers are for the *firmware*, not the board hardware.)

<span style="color:red">From v1.4 (Nov 16 2024) the communication data between RPi and PRU changed. Make sure you update both the LinuxCNC component and the firmware!</span>

#### weenyPRU-default
(Only for pre-v1.2 board) Digital ins on D1-7, digital outs on D8-14 (no UART for Trinamic drivers)

#### weenyPRU-v1.2
(v1.1 board) Digital ins on D1-7, digital outs on D8-10 and D12-14. UART for Trinamic drivers on D11.

(v1.2 board) Digital ins on D1-7, digital outs on D8-13. UART pin for Trinamic drivers now labeled 'UART'.

Enables UART TMC and XGZP.

#### weenyPRU-v1.3

(v1.2 board) Digital ins on D1-7, digital outs on D8-13.

Enables UART TMC, XGZP and DS3502.

#### weenyPRU-v1.4

(v1.2 board) Digital ins on D1-7, digital outs on D8,D11,RGB. 
Enables UART TMC, XGZP, DS3502, and HX711 loadcell. Disables RGB LEDs!
