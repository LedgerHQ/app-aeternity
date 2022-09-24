#include "signMessage.h"
#include "os.h"
#include "utils.h"

static char message[0xFC];
static uint32_t accountNumber;
static uint8_t *data;
static uint32_t dataLength;

static const char SIGN_MAGIC[] = "aeternity Signed Message:\n";

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
    ux_flow_init(0, ux_sign_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
}
