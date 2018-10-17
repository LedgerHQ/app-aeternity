#include "os.h"
#include "cx.h"
#include <stdbool.h>
#include <stdlib.h>
#include "aeUtils.h"

#define SPEND_TRANSACTION_PREFIX 12
#define ACCOUNT_ADDRESS_PREFIX 1

static const char BASE_58_ALPHABET[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
                                        'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q',
                                        'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                                        'h', 'i', 'j', 'k', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                        'w', 'x', 'y', 'z'};

unsigned char encodeBase58(unsigned char WIDE *in, unsigned char length,
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

void getAeAddressStringFromKey(cx_ecfp_public_key_t *publicKey, uint8_t *address) {
    uint8_t buffer[32];

    for (int i = 0; i < 32; i++) {
        buffer[i] = publicKey->W[64 - i];
    }
    if ((publicKey->W[32] & 1) != 0) {
        buffer[31] |= 0x80;
    }
    getAeAddressStringFromBinary(buffer, address);
}

void getAeAddressStringFromBinary(uint8_t *publicKey, uint8_t *address) {
    uint8_t buffer[36];
    uint8_t hashAddress[32];

    os_memmove(buffer, publicKey, 32);
    cx_hash_sha256(buffer, 32, hashAddress);
    cx_hash_sha256(hashAddress, 32, hashAddress);
    os_memmove(buffer + 32, hashAddress, 4);

    address[encodeBase58(buffer, 36, address, 51)] = '\0';
}

void rlpParseInt(uint8_t *workBuffer, uint8_t fieldLength, uint32_t offset, char* buffer) {
    uint64_t amount = 0;
    if (offset == 0) {
        workBuffer--;
        amount = *workBuffer++;
    }
    else{
        workBuffer += offset - 1;
        for (uint8_t i = 0; i < fieldLength; i++) {
            amount += *workBuffer++ << (8 * (fieldLength - 1 - i));
        }
    }
    if (amount == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    uint8_t digits;
    uint64_t temp = amount;
    //int nDigits = floor(log10(abs(amount))) + 1;
    for (digits = 0; temp != 0; ++digits, temp /= 10 );

    for (uint8_t i = 0; amount != 0; ++i, amount /= 10 )
    {
        buffer[digits - 1 - i] = amount % 10 + '0';
    }
    buffer[digits] = '\0';
}

void parseTx(char *address, char *amount, char *fee, uint8_t *data) {
    uint8_t publicKey[32];
    uint8_t buffer[5];
    uint8_t bufferPos = 0;
    uint32_t fieldLength;
    uint32_t offset = 0;
    rlpTxType type = -1;
    bool isList = true;
    bool valid = false;
    while (type != TX_FEE) {
        do {
            buffer[bufferPos++] = *data++;
        } while (!rlpCanDecode(buffer, bufferPos, &valid));
        if (!rlpDecodeLength(data - bufferPos,
                                &fieldLength, &offset,
                                &isList)) {
            PRINTF("Invalid RLP Length\n");
            THROW(0x6800);
        }
        type++;
        switch (type) {
            case TX_TYPE:
                if (*data++ != SPEND_TRANSACTION_PREFIX) {
                    PRINTF("Wrong type of transaction\n");
                    THROW(0x6A80);
                }
                data++;
                break;
            case TX_SENDER:
                if (*data != ACCOUNT_ADDRESS_PREFIX) {
                    PRINTF("Wrong type of sender: %d\n", *data);
                    THROW(0x6A80);
                }
                data++;
                data += 32;
                break;
            case TX_RECIPENT:
                if (*data != ACCOUNT_ADDRESS_PREFIX) {
                    PRINTF("Wrong type of recipent: %d\n", *data);
                    THROW(0x6A80);
                }
                data++;
                address[0] = 'a';
                address[1] = 'k';
                address[2] = '_';
                os_memmove(publicKey, data, 32);
                getAeAddressStringFromBinary(publicKey, address + 3);
                data += 32;
                break;
            case TX_AMOUNT:
                rlpParseInt(data, fieldLength, offset, amount);
                data += fieldLength + offset - 1;
                break;
            case TX_FEE:
                rlpParseInt(data, fieldLength, offset, fee);
                break;
        }
        bufferPos = 0;
        fieldLength = 0;
        valid = false;
    }
}
