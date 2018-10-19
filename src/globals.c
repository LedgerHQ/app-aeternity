#include "globals.h"

const uint32_t HARDENED_OFFSET = 0x80000000;
const uint32_t derivePath[BIP32_PATH] = {44 | HARDENED_OFFSET, 457 | HARDENED_OFFSET, 0 | HARDENED_OFFSET,
                                         0 | HARDENED_OFFSET, 0 | HARDENED_OFFSET};

ux_state_t ux;
unsigned int ux_step;
unsigned int ux_step_count;

union tempContext tmpCtx;
WIDE internalStorage_t N_storage_real;
union stringData strings;
