#include "signTransaction.h"
#include "utils.h"

static char recipientAddress[FULL_ADDRESS_LENGTH];
static char fullAmount[50];
static char fee[50];
static uint32_t accountNumber;
static uint16_t dataLength;
static uint8_t *data;

static const bagl_element_t ui_approval_nanos[] = {
    UI_BUTTONS,
    UI_LABELINE(0x01, "Confirm",        UI_FIRST,  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x01, "transaction",    UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x02, "Amount",         UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x02, fullAmount,       UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
    UI_LABELINE(0x03, "Address",        UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x03, recipientAddress, UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 50),
    UI_LABELINE(0x04, "Fees",           UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x04, fee,              UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
};

static unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT:
            sign(accountNumber, data, dataLength, G_io_apdu_buffer);
            sendResponse(64, true);
            break;
    }
    return 0;
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t workBufferLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    uint8_t senderPublicKey[32];
    uint8_t publicKeyArray[32];

    if (p1 != P1_FIRST || p2 != 0) {
        THROW(0x6B00);
    }

    accountNumber = readUint32BE(workBuffer);
    workBuffer += 4;
    workBufferLength -= 4;

    getPrivateKey(accountNumber, &privateKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, &publicKey, &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));
    getPublicKeyArray(&publicKey, publicKeyArray);

    const uint8_t networkIdLength = *(workBuffer++);
    workBufferLength--;
    dataLength = workBufferLength;
    data = workBuffer;

    ux_step = 0;
    ux_step_count = 4;
    parseTx(senderPublicKey, recipientAddress, fullAmount, fee, data + networkIdLength, dataLength - networkIdLength);

    if (os_memcmp(publicKeyArray, senderPublicKey, sizeof(publicKeyArray)) != 0) {
        PRINTF("Sender is not correct\n");
        THROW(0x6A80);
    }
    UX_DISPLAY(ui_approval_nanos, ui_prepro);
    *flags |= IO_ASYNCH_REPLY;
}
