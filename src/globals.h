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

extern struct strData_t {
    char recipientAddress[FULL_ADDRESS_LENGTH];
    char fullAmount[50];
    char fee[50];
} strings;

typedef struct addressContext_t {
    char address[FULL_ADDRESS_LENGTH];
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
