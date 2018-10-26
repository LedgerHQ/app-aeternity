#include "globals.h"

#ifndef _SIGN_TRANSACTION_H_
#define _SIGN_TRANSACTION_H_

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

#endif
