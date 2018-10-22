#include "signMessage.h"
#include "aeUtils.h"

const bagl_element_t ui_approval_signMessage_nanos[] = {
    // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

    //{{BAGL_ICON                           , 0x01,  28,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Sign the", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "message", 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE                       , 0x02,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Message hash", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x02,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },
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

static unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            io_seproxyhal_touch_signMessage_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
            io_seproxyhal_touch_signMessage_ok(NULL);
            break;
        }
    }
    return 0;
}


unsigned int io_seproxyhal_touch_signMessage_ok(const bagl_element_t *e) {
    uint8_t privateKeyData[32];
    uint8_t signature[100];
    uint8_t signatureLength;
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, tmpCtx.messageSigningContext.bip32Path,
                               BIP32_PATH, privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    unsigned int info = 0;
    signatureLength =
        cx_eddsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA512,
                      tmpCtx.messageSigningContext.data,
                      tmpCtx.messageSigningContext.dataLength + sizeof(SIGN_MAGIC) + 1,
                      NULL, 0, signature, &info);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memmove(G_io_apdu_buffer, signature, 64);
    tx = 64;
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_signMessage_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    uint8_t messageLength;
    if (p1 == P1_FIRST) {
        os_memmove(tmpCtx.messageSigningContext.bip32Path, derivePath, BIP32_PATH * sizeof(uint32_t));
        uint32_t accoutNumber = readUint32BE(workBuffer);
        workBuffer += 4;
        dataLength -= 4;
        tmpCtx.messageSigningContext.bip32Path[2] += accoutNumber;
        tmpCtx.messageSigningContext.remainingLength = readUint32BE(workBuffer);
        workBuffer += 4;
        dataLength -= 4;
        uint8_t signMagicLength = sizeof(SIGN_MAGIC) - 1;
        tmpCtx.messageSigningContext.dataLength = 0;
        tmpCtx.messageSigningContext.data[0] = signMagicLength;
        os_memmove(tmpCtx.messageSigningContext.data + 1, SIGN_MAGIC, signMagicLength);
        messageLength = 1 + signMagicLength;
        tmpCtx.messageSigningContext.data[messageLength] = tmpCtx.messageSigningContext.remainingLength;
        messageLength++;
    }
    else if (p1 != P1_MORE) {
        THROW(0x6B00);
    }
    if (p2 != 0) {
        THROW(0x6B00);
    }
    if (dataLength > tmpCtx.messageSigningContext.remainingLength
        || tmpCtx.messageSigningContext.remainingLength >= 0xFD) {
        THROW(0x6A80);
    }

    os_memmove(tmpCtx.messageSigningContext.data + messageLength
                + tmpCtx.messageSigningContext.dataLength,
                workBuffer, dataLength);
    tmpCtx.messageSigningContext.dataLength += dataLength;
    tmpCtx.messageSigningContext.remainingLength -= dataLength;
    if (tmpCtx.messageSigningContext.remainingLength == 0) {
        strings.common.fullAddress[0] = '\0';
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_approval_signMessage_nanos,
                   ui_approval_signMessage_prepro);
        *flags |= IO_ASYNCH_REPLY;
    } else {
        THROW(0x9000);
    }
}
