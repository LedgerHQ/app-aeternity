#include "signTransaction.h"
#include "utils.h"

volatile bool dataPresent;

const bagl_element_t ui_approval_nanos[] = {
    // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

    //{{BAGL_ICON                           , 0x01,  21,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_TRANSACTION_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "transaction", 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "WARNING", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Data present", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE                       , 0x03,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Amount", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x03,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.fullAmount, 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE                       , 0x04,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Address", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x04,  23,  26,  82,  12, 0x80|10, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 50   }, (char*)strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE                       , 0x05,   0,  12, 128,  32, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Fees", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x05,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.maxFee, 0, 0, 0, NULL, NULL, NULL },
};

static unsigned int ui_approval_prepro(const bagl_element_t* element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid-1);
        if(display) {
            switch(element->component.userid) {
                case 1:
                    UX_CALLBACK_SET_INTERVAL(2000);
                    break;
                case 2:
                    if (dataPresent && !N_storage.contractDetails) {
                        UX_CALLBACK_SET_INTERVAL(3000);
                    }
                    else {
                        display = 0;
                        ux_step++; // display the next step
                    }
                    break;
                case 3:
                    UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
                case 4:
                    UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
                case 5:
                    UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
                    break;
            }
        }
    }
    return display;
}

static unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            io_seproxyhal_touch_tx_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: {
            io_seproxyhal_touch_tx_ok(NULL);
            break;
        }
    }
    return 0;
}

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    uint8_t privateKeyData[32];
    uint8_t signature[100];
    uint8_t signatureLength;
    cx_ecfp_private_key_t privateKey;
    uint32_t tx = 0;
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, tmpCtx.transactionContext.bip32Path,
                               tmpCtx.transactionContext.pathLength,
                               privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    unsigned int info = 0;
    signatureLength =
        cx_eddsa_sign(&privateKey, CX_RND_RFC6979 | CX_LAST, CX_SHA512,
                      tmpCtx.transactionContext.data,
                      tmpCtx.transactionContext.dataLength, NULL, 0, signature, &info);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memmove(G_io_apdu_buffer, signature, 64);
    tx = 64;
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
send:
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    if (p1 == P1_FIRST) {
        tmpCtx.transactionContext.pathLength = BIP32_PATH;
        os_memmove(tmpCtx.transactionContext.bip32Path, derivePath, BIP32_PATH * sizeof(uint32_t));
        uint32_t accoutNumber = readUint32BE(workBuffer);
        workBuffer += 4;
        dataLength -= 4;
        tmpCtx.transactionContext.bip32Path[2] += accoutNumber;
        dataPresent = false;
        tmpCtx.transactionContext.dataLength = dataLength;
        tmpCtx.transactionContext.data = workBuffer;
    }
    else if (p1 != P1_MORE) {
        THROW(0x6B00);
    }
    if (p2 != 0) {
        THROW(0x6B00);
    }

    ux_step = 0;
    ux_step_count = 5;
    parseTx(strings.common.fullAddress, strings.common.fullAmount, strings.common.maxFee, tmpCtx.transactionContext.data);
    UX_DISPLAY(ui_approval_nanos, ui_approval_prepro);
    *flags |= IO_ASYNCH_REPLY;
}
