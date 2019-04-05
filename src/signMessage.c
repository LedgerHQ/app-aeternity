#include "signMessage.h"
#include "os.h"
#include "utils.h"

static char message[0xFC];
static uint32_t accountNumber;
static uint8_t *data;
static uint32_t dataLength;

static const char SIGN_MAGIC[] = "æternity Signed Message:\n";

static unsigned int io_seproxyhal_touch_signMessage_ok(const bagl_element_t *e) {
    uint8_t message[0xFC + sizeof(SIGN_MAGIC) - 1 + 2];
    uint8_t messageLength = 0;

    const uint8_t signMagicLength = sizeof(SIGN_MAGIC) - 1;
    message[0] = signMagicLength;
    messageLength++;

    os_memmove(message + messageLength, SIGN_MAGIC, signMagicLength);
    messageLength += signMagicLength;

    message[messageLength] = dataLength;
    messageLength++;

    os_memmove(message + messageLength, data, dataLength);
    messageLength += dataLength;

    sign(
        accountNumber,
        message,
        messageLength,
        G_io_apdu_buffer
    );
    sendResponse(64, true);
    return 0; // do not redraw the widget
}

#if defined(TARGET_NANOS)

static const bagl_element_t ui_approval_signMessage_nanos[] = {
    UI_BUTTONS,
    UI_LABELINE(0x01, "Sign the", UI_FIRST,  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x01, "message",  UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x02, "Message",  UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x02, message,    UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
};

static unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            io_seproxyhal_touch_signMessage_ok(NULL);
            break;
    }
    return 0;
}

#elif defined(TARGET_NANOX)

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_sign_flow_1_step, 
    pnn, 
    {
      &C_icon_certificate,
      "Sign",
      "message",
    });
UX_STEP_NOCB(
    ux_sign_flow_2_step, 
    bnnn_paging, 
    {
      .title = "Message",
      .text = message,
    });
UX_STEP_VALID(
    ux_sign_flow_3_step,
    pbb,
    io_seproxyhal_touch_signMessage_ok(NULL),
    {
      &C_icon_validate_14,
      "Sign",
      "message",
    });
UX_STEP_VALID(
    ux_sign_flow_4_step,
    pbb,
    sendResponse(0, false),
    {
      &C_icon_crossmark,
      "Cancel",
      "signature",
    });

UX_FLOW(ux_sign_flow,
  &ux_sign_flow_1_step,
  &ux_sign_flow_2_step,
  &ux_sign_flow_3_step,
  &ux_sign_flow_4_step
);

#endif

void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t workBufferLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    if (p1 != P1_FIRST || p2 != 0) {
        THROW(0x6B00);
    }

    accountNumber = readUint32BE(workBuffer);
    workBuffer += 8;
    workBufferLength -= 8;
    if (workBufferLength >= 0xFD) {
        THROW(0x6A80);
    }
    dataLength = workBufferLength;
    data = workBuffer;

    snprintf(message, sizeof(message), "%.*s", dataLength, workBuffer);
#if defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_approval_signMessage_nanos, ui_prepro);
#elif defined(TARGET_NANOX)
    ux_flow_init(0, ux_sign_flow, NULL);
#endif
    *flags |= IO_ASYNCH_REPLY;
}
