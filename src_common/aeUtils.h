#include "os.h"
#include "cx.h"

unsigned char encodeBase58(unsigned char WIDE *in, unsigned char length,
                           unsigned char *out, unsigned char maxoutlen);

void getAeAddressStringFromKey(cx_ecfp_public_key_t *publicKey, uint8_t *address);
