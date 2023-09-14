#ifndef SRC_COMMS_H_
#define SRC_COMMS_H_

#include "main.h"
#include "messages.h"

extern volatile rxData_t rxData;
extern volatile txData_t txData;

extern volatile unsigned int packetCount;

void commsStart();

#endif /* SRC_COMMS_H_ */
