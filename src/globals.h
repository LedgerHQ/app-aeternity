#include "os.h"
#include "os_io_seproxyhal.h"

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P1_FIRST 0x00
#define P1_MORE 0x80

#define FULL_ADDRESS_LENGTH 54
#define BIP32_PATH 5

typedef struct strData_t {
    char fullAddress[FULL_ADDRESS_LENGTH];
    char fullAmount[50];
    char maxFee[50];
} strData_t;

typedef struct strDataTmp_t {
    char tmp[100];
    char tmp2[40];
} strDataTmp_t;

extern union stringData{
    strData_t common;
    strDataTmp_t tmp;
} strings;

typedef struct addressContext_t {
    char address[FULL_ADDRESS_LENGTH - 3];
} addressContext_t;

extern ux_state_t ux;
// display stepped screens
extern unsigned int ux_step;
extern unsigned int ux_step_count;

typedef struct signingContext_t {
    uint32_t accountNumber;
    uint8_t *data;
    uint32_t dataLength;
} signingContext_t;

extern union tempContext{
    addressContext_t addressContext;
    signingContext_t signingContext;
} tmpCtx;

typedef struct internalStorage_t {
    unsigned char dataAllowed;
    unsigned char contractDetails;
    uint8_t initialized;
} internalStorage_t;

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t*) PIC(&N_storage_real))
#endif
