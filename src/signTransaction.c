#include "signTransaction.h"
#include "os.h"
#include "utils.h"
#include "blake2b.h"
#define NETWORK_ID_MAX_LENGTH 32

static char recipientAddress[FULL_ADDRESS_LENGTH];
static char fullAmount[80];
static char fee[80];
static char payload[80];
static uint32_t accountNumber;
static uint32_t remainTransactionLength;
static char networkId[NETWORK_ID_MAX_LENGTH + 1];
cx_blake2b_t hash;
static txType transactionType;

static void singAndSend() {
    const uint8_t networkIdLength = strlen(networkId);
    uint8_t dataToSign[networkIdLength + 32];

    blake2b_final(&hash, dataToSign + networkIdLength, 32);
    os_memmove(dataToSign, networkId, networkIdLength);
    sign(accountNumber, dataToSign, networkIdLength + 32, G_io_apdu_buffer);
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
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT:
            singAndSend();
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
UX_STEP_NOCB(
    ux_confirm_full_flow_5_step,
    bnnn_paging,
    {
      .title = "Payload",
      .text = payload,
    });
UX_STEP_VALID(
    ux_confirm_full_flow_6_step,
    pbb,
    singAndSend(),
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

    if (p1 == P1_FIRST) {
        blake2b_init(&hash);
        accountNumber = readUint32BE(workBuffer);
        workBuffer += 4;
        workBufferLength -= 4;
        remainTransactionLength = readUint32BE(workBuffer);
        workBuffer += 4;
        workBufferLength -= 4;
        const uint8_t networkIdLength = *(workBuffer++);
        if (networkIdLength > NETWORK_ID_MAX_LENGTH) {
            PRINTF("Network id is to long\n");
            THROW(0x6A80);
        }
        os_memmove(networkId, workBuffer, networkIdLength);
        networkId[networkIdLength] = 0;
        workBufferLength--;
        workBuffer += networkIdLength;
        workBufferLength -= networkIdLength;
        parseTx(recipientAddress, fullAmount, fee, payload, workBuffer, workBufferLength, remainTransactionLength, &transactionType);
    }
    else if (p1 != P1_MORE) {
        THROW(0x6B00);
    }

    blake2b_update(&hash, workBuffer, workBufferLength);

    remainTransactionLength -= workBufferLength;
    if (remainTransactionLength < 0) {
        PRINTF("Extra data passed\n");
        THROW(0x6A80);
    }
    if (remainTransactionLength != 0) {
        THROW(0x9000);
    }

#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = transactionType == TX_SPEND ? 5 : 1;
    UX_DISPLAY(ui_approval_nanos, ui_prepro);
#elif defined(TARGET_NANOX)
    ux_flow_init(0, ux_confirm_full_flow, NULL);
#endif

    *flags |= IO_ASYNCH_REPLY;
}
