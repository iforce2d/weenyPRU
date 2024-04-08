# weenyPRU hardware

weenyPRU v1.1 [https://oshwlab.com/iforce2d/stm32-pru_copy](https://oshwlab.com/iforce2d/stm32-pru_copy)

weenyPRU v1.2: [https://oshwlab.com/iforce2d/stm32-pru_copy_copy](https://oshwlab.com/iforce2d/stm32-pru_copy_copy)

## Versions

 - v1.0 : with TXB0108 and errors needing bodge wire. Obsolete
 - v1.1 : with 74HCT541, no provision for TMC2209 UART
 - v1.2 : with jumper and pullup for TMC2209 UART (untested)

To use TMC2209 UART control with a v1.1 board, connect a 1k resistor between VCC and the UART pin next to of one of the stepstick drivers. All the UART pins are connected together in a 'rail', so any will do. Then also connect D11 to the same UART rail. Of course, D11 will no longer be available as a regular GPIO in this configuration.

