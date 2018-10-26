#include "os.h"
#include "cx.h"
#include "globals.h"

typedef enum rlpTxType {
    TX_TYPE = 0,
    TX_SENDER,
    TX_RECIPENT,
    TX_AMOUNT,
    TX_FEE
} rlpTxType;

unsigned char encodeBase58(unsigned char WIDE *in, unsigned char length,
                           unsigned char *out, unsigned char maxoutlen);

void getAeAddressStringFromKey(cx_ecfp_public_key_t *publicKey, uint8_t *address);

void getAeAddressStringFromBinary(uint8_t *publicKey, uint8_t *address);

uint32_t readUint32BE(uint8_t *buffer);

void sign(uint8_t *data, uint32_t dataLength, uint8_t *signature);

void getPrivateKey(uint32_t accountNumber, cx_ecfp_private_key_t *privateKey);

void parseTx(char *address, char *amount, char *fee, uint8_t *data);

void sendResponse(uint8_t tx, bool approve);
