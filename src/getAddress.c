#include "getAddress.h"
#include "utils.h"

static char address[FULL_ADDRESS_LENGTH];

static const bagl_element_t ui_address_nanos[] = {
    UI_BUTTONS,
    UI_LABELINE(0x01, "Confirm", UI_FIRST,  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x01, "address", UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x02, "Address", UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x02, address,   UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
};

static uint8_t set_result_get_address() {
    uint8_t tx = 0;
    const uint8_t address_size = strlen(address);
    G_io_apdu_buffer[tx++] = address_size;
    os_memmove(G_io_apdu_buffer + tx, address, address_size);
    tx += address_size;
    return tx;
}

static unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT: // CANCEL
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: // OK
            sendResponse(set_result_get_address(), true);
            break;
    }
    return 0;
}

void handleGetAddress(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(dataLength);
    UNUSED(p2);
    uint8_t publicKey[32];

    getPublicKey(readUint32BE(dataBuffer), publicKey);
    getAeAddressStringFromBinary(publicKey, address);

    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_address();
        THROW(0x9000);
    } else {
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_address_nanos, ui_prepro);

        *flags |= IO_ASYNCH_REPLY;
    }
}
