#include "signMessage.h"
#include "utils.h"

const bagl_element_t ui_approval_signMessage_nanos[] = {
    // type            userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE   , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON        , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_ICON        , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE    , 0x01,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Sign the", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE    , 0x01,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "message", 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE    , 0x02,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Message hash", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE    , 0x02,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, strings.recipientAddress, 0, 0, 0, NULL, NULL, NULL },
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
