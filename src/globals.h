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
    uint8_t address[FULL_ADDRESS_LENGTH - 3];
} addressContext_t;

extern ux_state_t ux;
// display stepped screens
extern unsigned int ux_step;
extern unsigned int ux_step_count;

typedef struct transactionContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[BIP32_PATH];
    uint8_t *data;
    uint8_t dataLength;
} transactionContext_t;

typedef struct messageSigningContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[BIP32_PATH];
    uint8_t data[0xFD + 26 + 2];
    uint32_t remainingLength;
    uint32_t dataLength;
} messageSigningContext_t;

extern union tempContext{
    addressContext_t addressContext;
    transactionContext_t transactionContext;
    messageSigningContext_t messageSigningContext;
} tmpCtx;

typedef struct internalStorage_t {
    unsigned char dataAllowed;
    unsigned char contractDetails;
    uint8_t initialized;
} internalStorage_t;

extern const uint32_t HARDENED_OFFSET;
extern const uint32_t derivePath[BIP32_PATH];

static const char const SIGN_MAGIC[] = "æternity Signed Message:\n";

extern WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t*) PIC(&N_storage_real))
#endif
