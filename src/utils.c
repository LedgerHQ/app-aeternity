#include "os.h"
#include "cx.h"
#include <stdbool.h>
#include <stdlib.h>
#include "utils.h"
#include "menu.h"
#include "uint256.h"

#define SPEND_TRANSACTION_PREFIX 12
#define ACCOUNT_ADDRESS_PREFIX 1
#define ACCOUNT_NAMEHASH_PREFIX 2
#define MAX_INT256 32
#define DECIMALS 18

static const char BASE_58_ALPHABET[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                        'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q',
                                        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                                        'h', 'i', 'j', 'k', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                        'w', 'x', 'y', 'z'};

static unsigned char encodeBase58(unsigned char WIDE *in, unsigned char length,
                           unsigned char *out, unsigned char maxoutlen) {
    unsigned char tmp[164];
    unsigned char buffer[164];
    unsigned char j;
    unsigned char startAt;
    unsigned char zeroCount = 0;
    if (length > sizeof(tmp)) {
        THROW(INVALID_PARAMETER);
    }
    os_memmove(tmp, in, length);
    while ((zeroCount < length) && (tmp[zeroCount] == 0)) {
        ++zeroCount;
    }
    j = 2 * length;
    startAt = zeroCount;
    while (startAt < length) {
        unsigned short remainder = 0;
        unsigned char divLoop;
        for (divLoop = startAt; divLoop < length; divLoop++) {
            unsigned short digit256 = (unsigned short)(tmp[divLoop] & 0xff);
            unsigned short tmpDiv = remainder * 256 + digit256;
            tmp[divLoop] = (unsigned char)(tmpDiv / 58);
            remainder = (tmpDiv % 58);
        }
        if (tmp[startAt] == 0) {
            ++startAt;
        }
        buffer[--j] = (unsigned char)BASE_58_ALPHABET[remainder];
    }
    while ((j < (2 * length)) && (buffer[j] == BASE_58_ALPHABET[0])) {
        ++j;
    }
    while (zeroCount-- > 0) {
        buffer[--j] = BASE_58_ALPHABET[0];
    }
    length = 2 * length - j;
    if (maxoutlen < length) {
        THROW(EXCEPTION_OVERFLOW);
    }
    os_memmove(out, (buffer + j), length);
    return length;
}

static void getAeEncodedString (uint8_t *publicKey, char *address, char *prefix) {
    uint8_t buffer[36];
    uint8_t hashAddress[32];

    os_memmove(buffer, publicKey, 32);
    cx_hash_sha256(buffer, 32, hashAddress, 32);
    cx_hash_sha256(hashAddress, 32, hashAddress, 32);
    os_memmove(buffer + 32, hashAddress, 4);

    snprintf(address, sizeof(address), prefix);
    address[encodeBase58(buffer, 36, (unsigned char*)address + 3, 51) + 3] = '\0';
}

void getAeAddressStringFromBinary(uint8_t *publicKey, char *address) {
    getAeEncodedString(publicKey, address, "ak_");
}

void getPublicKey(uint32_t accountNumber, uint8_t *publicKeyArray) {
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;

    getPrivateKey(accountNumber, &privateKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));

    for (int i = 0; i < 32; i++) {
        publicKeyArray[i] = publicKey.W[64 - i];
    }
    if ((publicKey.W[32] & 1) != 0) {
        publicKeyArray[31] |= 0x80;
    }
}

uint32_t readUint32BE(uint8_t *buffer) {
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3]);
}

static const uint32_t HARDENED_OFFSET = 0x80000000;

static const uint32_t derivePath[BIP32_PATH] = {
  44 | HARDENED_OFFSET,
  457 | HARDENED_OFFSET,
  0 | HARDENED_OFFSET,
  0 | HARDENED_OFFSET,
  0 | HARDENED_OFFSET
};

void getPrivateKey(uint32_t accountNumber, cx_ecfp_private_key_t *privateKey) {
    uint8_t privateKeyData[32];
    uint32_t bip32Path[BIP32_PATH];

    os_memmove(bip32Path, derivePath, sizeof(derivePath));
    bip32Path[2] = accountNumber | HARDENED_OFFSET;
    os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10, CX_CURVE_Ed25519, bip32Path, BIP32_PATH, privateKeyData, NULL, NULL, 0);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
}

void sign(uint32_t accountNumber, uint8_t *data, uint32_t dataLength, uint8_t *out) {
    cx_ecfp_private_key_t privateKey;
    uint8_t signature[64];

    getPrivateKey(accountNumber, &privateKey);
    unsigned int info = 0;
    cx_eddsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA512,
                         data,
                         dataLength,
                         NULL, 0, signature, 64, &info);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memmove(out, signature, 64);
}

void sendResponse(uint8_t tx, bool approve) {
    G_io_apdu_buffer[tx++] = approve? 0x90 : 0x69;
    G_io_apdu_buffer[tx++] = approve? 0x00 : 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
}

unsigned int ui_prepro(const bagl_element_t *element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid - 1);
        if (display) {
            if (element->component.userid == 1) {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
    }
    return display;
}

static bool rlpCanDecode(uint8_t *buffer, uint32_t bufferLength, bool *valid) {
    if (*buffer <= 0x7f) {
    } else if (*buffer <= 0xb7) {
    } else if (*buffer <= 0xbf) {
        if (bufferLength < (1 + (*buffer - 0xb7))) {
            return false;
        }
        if (*buffer > 0xbb) {
            *valid = false; // arbitrary 32 bits length limitation
            return true;
        }
    } else if (*buffer <= 0xf7) {
    } else {
        if (bufferLength < (1 + (*buffer - 0xf7))) {
            return false;
        }
        if (*buffer > 0xfb) {
            *valid = false; // arbitrary 32 bits length limitation
            return true;
        }
    }
    *valid = true;
    return true;
}

static bool rlpDecodeLength(uint8_t *buffer, uint32_t *fieldLength, uint32_t *offset, bool *list) {
    if (*buffer <= 0x7f) {
        *offset = 0;
        *fieldLength = 1;
        *list = false;
    } else if (*buffer <= 0xb7) {
        *offset = 1;
        *fieldLength = *buffer - 0x80;
        *list = false;
    } else if (*buffer <= 0xbf) {
        *offset = 1 + (*buffer - 0xb7);
        *list = false;
        switch (*buffer) {
        case 0xb8:
            *fieldLength = *(buffer + 1);
            break;
        case 0xb9:
            *fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
            break;
        case 0xba:
            *fieldLength =
                (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
            break;
        case 0xbb:
            *fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) +
                           (*(buffer + 3) << 8) + *(buffer + 4);
            break;
        default:
            return false; // arbitrary 32 bits length limitation
        }
    } else if (*buffer <= 0xf7) {
        *offset = 1;
        *fieldLength = *buffer - 0xc0;
        *list = true;
    } else {
        *offset = 1 + (*buffer - 0xf7);
        *list = true;
        switch (*buffer) {
        case 0xf8:
            *fieldLength = *(buffer + 1);
            break;
        case 0xf9:
            *fieldLength = (*(buffer + 1) << 8) + *(buffer + 2);
            break;
        case 0xfa:
            *fieldLength =
                (*(buffer + 1) << 16) + (*(buffer + 2) << 8) + *(buffer + 3);
            break;
        case 0xfb:
            *fieldLength = (*(buffer + 1) << 24) + (*(buffer + 2) << 16) +
                           (*(buffer + 3) << 8) + *(buffer + 4);
            break;
        default:
            return false; // arbitrary 32 bits length limitation
        }
    }

    return true;
}

static void convertUint256BE(uint8_t *data, uint32_t length, uint256_t *target) {
    uint8_t tmp[32];
    os_memset(tmp, 0, 32);
    os_memmove(tmp + 32 - length, data, length);
    readu256BE(tmp, target);
}

static bool adjustDecimals(char *src, uint32_t srcLength, char *target,
                    uint32_t targetLength, uint8_t decimals) {
    uint32_t startOffset;
    uint32_t lastZeroOffset = 0;
    uint32_t offset = 0;
    if ((srcLength == 1) && (*src == '0')) {
        if (targetLength < 2) {
                return false;
        }
        target[0] = '0';
        target[1] = '\0';
        return true;
    }
    if (srcLength <= decimals) {
        uint32_t delta = decimals - srcLength;
        if (targetLength < srcLength + 1 + 2 + delta) {
            return false;
        }
        target[offset++] = '0';
        target[offset++] = '.';
        for (uint32_t i = 0; i < delta; i++) {
            target[offset++] = '0';
        }
        startOffset = offset;
        for (uint32_t i = 0; i < srcLength; i++) {
            target[offset++] = src[i];
        }
        target[offset] = '\0';
    } else {
        uint32_t sourceOffset = 0;
        uint32_t delta = srcLength - decimals;
        if (targetLength < srcLength + 1 + 1) {
            return false;
        }
        while (offset < delta) {
            target[offset++] = src[sourceOffset++];
        }
        if (decimals != 0) {
            target[offset++] = '.';
        }
        startOffset = offset;
        while (sourceOffset < srcLength) {
            target[offset++] = src[sourceOffset++];
        }
	target[offset] = '\0';
    }
    for (uint32_t i = startOffset; i < offset; i++) {
        if (target[i] == '0') {
            if (lastZeroOffset == 0) {
                lastZeroOffset = i;
            }
        } else {
            lastZeroOffset = 0;
        }
    }
    if (lastZeroOffset != 0) {
        target[lastZeroOffset] = '\0';
        if (target[lastZeroOffset - 1] == '.') {
                target[lastZeroOffset - 1] = '\0';
        }
    }
    return true;
}

static void rlpParseInt(uint8_t **workBuffer, uint32_t fieldLength, uint32_t offset, char *buffer) {
    if (fieldLength > MAX_INT256) {
        PRINTF("Invalid length of tokens");
        THROW(0x6A80);
    }
    if (offset != 0) {
        *workBuffer += offset - 1;
    } else {
        (*workBuffer)--;
    }
    uint256_t uint256;
    char tmp[80];
    convertUint256BE(*workBuffer, fieldLength, &uint256);
    tostring256(&uint256, 10, tmp, 80);
    adjustDecimals(tmp, strlen(tmp), buffer, 80, DECIMALS);
    *workBuffer += fieldLength;
}

static void readRecipient(uint8_t **data, uint8_t *publicKey, uint32_t fieldLength) {
    if (**data != ACCOUNT_ADDRESS_PREFIX && **data != ACCOUNT_NAMEHASH_PREFIX || fieldLength != 33) {
        PRINTF("Wrong type of publicKey or publicKey length: %d %d\n", **data, fieldLength);
        THROW(0x6A80);
    }
    (*data)++;
    if (publicKey != NULL) os_memmove(publicKey, *data, 32);
    *data += 32;
}

void parseTx(char *recipientAddress, char *amount, char *fee, char *payload, uint8_t *data, uint16_t dataLength, uint32_t remainLength, txType *transactionType) {
    uint8_t recipientPublicKey[32];
    uint8_t buffer[5];
    uint8_t bufferPos = 0;
    uint32_t fieldLength;
    uint32_t offset = 0;
    rlpTxType type = TX_LENGTH;
    bool validLength = true;
    bool isList = true;
    bool valid = false;
    bool isAddress;
    while (type != TX_PAYLOAD) {
        do {
            buffer[bufferPos++] = *data++;
            dataLength--;
        } while (!rlpCanDecode(buffer, bufferPos, &valid));
        if (!rlpDecodeLength(data - bufferPos,
                                &fieldLength, &offset,
                                &isList)) {
            PRINTF("Invalid RLP Length\n");
            THROW(0x6A80);
        } else if (type != TX_LENGTH && fieldLength != 1) {
            validLength &= dataLength > fieldLength;
            dataLength -= fieldLength;
        } else if (!valid || bufferPos == sizeof(buffer)) {
            PRINTF("RLP pre-decode error\n");
            THROW(0x6A80);
        }
        if (type == TX_LENGTH) {
            if (remainLength - bufferPos < dataLength || !isList) {
                PRINTF("Invalid RLP Length\n");
                THROW(0x6A80);
            }
        }
        type++;
        if (type != TX_TYPE && !validLength) {
            THROW(0x6A80);
        }
        switch (type) {
            case TX_TYPE:
                *transactionType = *data++ != SPEND_TRANSACTION_PREFIX ? TX_OTHER : TX_SPEND;
                if (*transactionType == TX_OTHER) {
                    return;
                }
                data++;
                break;
            case TX_SENDER:
                readRecipient(&data, NULL, fieldLength);
                break;
            case TX_RECIPIENT:
                isAddress = (*data == ACCOUNT_ADDRESS_PREFIX);
                readRecipient(&data, recipientPublicKey, fieldLength);
                getAeEncodedString(recipientPublicKey, recipientAddress, isAddress? "ak_": "nm_");
                break;
            case TX_AMOUNT:
                rlpParseInt(&data, fieldLength, offset, amount);
                break;
            case TX_FEE:
                rlpParseInt(&data, fieldLength, offset, fee);
                break;
            case TX_TTL:
            case TX_NONCE:
                if (offset != 0) {
                    data += offset - 1 + fieldLength;
                }
                break;
            case TX_PAYLOAD:
                if (offset != 0) {
                    data += offset - 1;
                } else {
                    data--;
                }
                snprintf(payload, 80, "%.*s", fieldLength, data);
                break;
        }
        bufferPos = 0;
        fieldLength = 0;
        valid = false;
    }
}
