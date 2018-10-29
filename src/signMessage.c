#include "signMessage.h"
#include "utils.h"

static const bagl_element_t ui_approval_signMessage_nanos[] = {
    UI_BUTTONS,
    UI_LABELINE(0x01, "Sign the",               UI_FIRST,  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x01, "message",                UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x02, "Message hash",           UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x02, strings.recipientAddress, UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
};

static unsigned int ui_approval_signMessage_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        switch (element->component.userid) {
            case 1:
                UX_CALLBACK_SET_INTERVAL(2000);
                break;
            case 2:
                UX_CALLBACK_SET_INTERVAL(3000);
                break;
        }
        return (ux_step == element->component.userid - 1);
    }
    return 1;
}

static const char const SIGN_MAGIC[] = "æternity Signed Message:\n";

static unsigned int io_seproxyhal_touch_signMessage_ok(const bagl_element_t *e) {
    uint8_t message[0xFD + 26 + 2];
    uint8_t messageLength = 0;

    uint8_t signMagicLength = sizeof(SIGN_MAGIC) - 1;
    message[0] = signMagicLength;
    messageLength++;

    os_memmove(message + messageLength, SIGN_MAGIC, signMagicLength);
    messageLength += signMagicLength;

    message[messageLength] = tmpCtx.signingContext.dataLength;
    messageLength++;

    os_memmove(message + messageLength, tmpCtx.signingContext.data, tmpCtx.signingContext.dataLength);
    messageLength += tmpCtx.signingContext.dataLength;

    sign(
        message,
        messageLength,
        G_io_apdu_buffer
    );
    sendResponse(64, true);
    return 0; // do not redraw the widget
}

static unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
            io_seproxyhal_touch_signMessage_ok(NULL);
            break;
        }
    }
    return 0;
}

void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    if (p1 != P1_FIRST || p2 != 0) {
        THROW(0x6B00);
    }

    tmpCtx.signingContext.accountNumber = readUint32BE(workBuffer);
    workBuffer += 8;
    dataLength -= 8;
    if (dataLength >= 0xFD) {
        THROW(0x6A80);
    }
    tmpCtx.signingContext.dataLength = dataLength;
    tmpCtx.signingContext.data = workBuffer;

    strings.recipientAddress[0] = '\0';
    ux_step = 0;
    ux_step_count = 2;
    UX_DISPLAY(ui_approval_signMessage_nanos, ui_approval_signMessage_prepro);
    *flags |= IO_ASYNCH_REPLY;
}
