#include "signTransaction.h"
#include "os.h"
#include "utils.h"
#include "blake2b.h"

static char recipientAddress[FULL_ADDRESS_LENGTH];
static char fullAmount[80];
static char fee[80];
static char payload[80];
static uint32_t accountNumber;
static uint16_t dataLength;
static uint8_t *data;
static uint8_t networkIdLength;

static void io_seproxyhal_touch_signTx_ok(const bagl_element_t *e) {
    sign(accountNumber, data, dataLength, G_io_apdu_buffer);
    sendResponse(64, true);
}

#if defined(TARGET_NANOS)

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
    UI_LABELINE(0x05, "Payload",        UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x05, payload,          UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
};

static unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    uint8_t dataToSign[networkIdLength + 32];

    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT:
            blake2b(dataToSign + networkIdLength, 32, data + networkIdLength, dataLength - networkIdLength);
            os_memmove(dataToSign, data, networkIdLength);
            sign(accountNumber, dataToSign, networkIdLength + 32, G_io_apdu_buffer);
            sendResponse(64, true);
            break;
    }
    return 0;
}

#elif defined(TARGET_NANOX)

//////////////////////////////////////////////////////////////////////

UX_STEP_NOCB(ux_confirm_full_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_2_step,
    bnnn_paging,
    {
      .title = "Amount",
      .text = fullAmount
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_3_step,
    bnnn_paging,
    {
      .title = "Address",
      .text = recipientAddress,
    });
UX_STEP_NOCB(
    ux_confirm_full_flow_4_step,
    bnnn_paging,
    {
      .title = "Fees",
      .text = fee,
    });
UX_STEP_VALID(
    ux_confirm_full_flow_5_step,
    bnnn_paging,
    {
      .title = "Payload",
      .text = payload,
    });
UX_STEP_VALID(
    ux_confirm_full_flow_6_step,
    pbb,
    io_seproxyhal_touch_signTx_ok(NULL),
    {
      &C_icon_validate_14,
      "Accept",
      "and send",
    });
UX_STEP_VALID(
    ux_confirm_full_flow_7_step,
    pb,
    sendResponse(0, false),
    {
      &C_icon_crossmark,
      "Reject",
    });
// confirm_full: confirm transaction / Amount: fullAmount / Address: fullAddress / Fees: feesAmount
UX_FLOW(ux_confirm_full_flow,
  &ux_confirm_full_flow_1_step,
  &ux_confirm_full_flow_2_step,
  &ux_confirm_full_flow_3_step,
  &ux_confirm_full_flow_4_step,
  &ux_confirm_full_flow_5_step,
  &ux_confirm_full_flow_6_step,
  &ux_confirm_full_flow_7_step
);

#endif

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t workBufferLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    uint8_t senderPublicKey[32];
    uint8_t publicKey[32];

    if (p1 != P1_FIRST || p2 != 0) {
        THROW(0x6B00);
    }

    accountNumber = readUint32BE(workBuffer);
    workBuffer += 4;
    workBufferLength -= 4;
    networkIdLength = *(workBuffer++);
    workBufferLength--;
    dataLength = workBufferLength;
    data = workBuffer;

    parseTx(senderPublicKey, recipientAddress, fullAmount, fee, payload, data + networkIdLength, dataLength - networkIdLength);
    getPublicKey(accountNumber, publicKey);

    if (os_memcmp(publicKey, senderPublicKey, sizeof(publicKey)) != 0) {
        PRINTF("Sender is not correct\n");
        THROW(0x6A80);
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 5;
    UX_DISPLAY(ui_approval_nanos, ui_prepro);
#elif defined(TARGET_NANOX)
    ux_flow_init(0, ux_confirm_full_flow, NULL);
#endif

    *flags |= IO_ASYNCH_REPLY;
}
