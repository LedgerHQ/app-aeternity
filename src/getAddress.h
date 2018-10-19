#include "os.h"
#include "cx.h"
#include "globals.h"

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e);

uint32_t set_result_get_publicKey(void);

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);
