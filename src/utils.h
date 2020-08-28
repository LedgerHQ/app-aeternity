#include "os.h"
#include "cx.h"
#include "globals.h"

#ifndef _UTILS_H_
#define _UTILS_H_

typedef enum rlpTxType {
    TX_LENGTH = 0,
    TX_TYPE,
    TX_SENDER,
    TX_RECIPIENT,
    TX_AMOUNT,
    TX_FEE,
    TX_TTL,
    TX_NONCE,
    TX_PAYLOAD
} rlpTxType;

typedef enum txType {
    TX_SPEND = 0,
    TX_OTHER
} txType;

unsigned int ui_prepro(const bagl_element_t *element);

void getAeAddressStringFromBinary(uint8_t *publicKey, char *address);

void getPublicKey(uint32_t accountNumber, uint8_t *publicKeyArray);

uint32_t readUint32BE(uint8_t *buffer);

void sign(uint32_t accountNumber, uint8_t *data, uint32_t dataLength, uint8_t *signature);

void getPrivateKey(uint32_t accountNumber, cx_ecfp_private_key_t *privateKey);

void parseTx(char *recipientAddress, char *amount, char *fee, char *payload, uint8_t *data, uint16_t dataLength, uint32_t remainLength, txType *transactionType);

void sendResponse(uint8_t tx, bool approve);

    // type            userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
#define UI_BUTTONS \
    {{BAGL_RECTANGLE   , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},\
    {{BAGL_ICON        , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },\
    {{BAGL_ICON        , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL }

#define UI_FIRST 1
#define UI_SECOND 0

#define UI_LABELINE(userId, text, isFirst, font, horizontalScrollSpeed)    \
    {                                                                      \
        {                                                                  \
            BAGL_LABELINE,                                                 \
            (userId),                                                      \
            23,                                                            \
            (isFirst) ? 12 : 26,                                           \
            82,                                                            \
            12,                                                            \
            (horizontalScrollSpeed) ? BAGL_STROKE_FLAG_ONESHOT | 10 : 0,   \
            0, 0, 0xFFFFFF, 0x000000,                                      \
            (font)|BAGL_FONT_ALIGNMENT_CENTER,                             \
            horizontalScrollSpeed                                          \
        },                                                                 \
        (text), 0, 0, 0, NULL, NULL, NULL                                  \
    }

#endif
