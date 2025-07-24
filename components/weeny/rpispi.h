
#ifndef RPISPI_H
#define RPISPI_H

bool checkPiType();
bool rpispi_init();
void rpispi_transfernb(char* tbuf, char* rbuf, uint32_t len);
bool rpispi_close();

#endif









