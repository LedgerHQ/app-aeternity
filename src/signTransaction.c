#include "signTransaction.h"
#include "utils.h"

static const bagl_element_t ui_approval_nanos[] = {
    UI_BUTTONS,
    UI_LABELINE(0x01, "Confirm",                UI_FIRST,  BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x01, "transaction",            UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 0),
    UI_LABELINE(0x02, "Amount",                 UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x02, strings.fullAmount,       UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
    UI_LABELINE(0x03, "Address",                UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x03, strings.recipientAddress, UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 50),
    UI_LABELINE(0x04, "Fees",                   UI_FIRST,  BAGL_FONT_OPEN_SANS_REGULAR_11px,   0),
    UI_LABELINE(0x04, strings.fee,              UI_SECOND, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px, 26),
};

static unsigned int ui_approval_prepro(const bagl_element_t* element) {
    unsigned int display = 1;
    if (element->component.userid > 0) {
        display = (ux_step == element->component.userid - 1);
        if (display) {
            if (element->component.userid == 1) {
                UX_CALLBACK_SET_INTERVAL(2000);
            }
            else {
                UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000+bagl_label_roundtrip_duration_ms(element, 7)));
            }
        }
    }
    return display;
}

static unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT:
            sendResponse(0, false);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: {
            sign(
                tmpCtx.signingContext.data,
                tmpCtx.signingContext.dataLength,
                G_io_apdu_buffer
            );
            sendResponse(64, true);
            break;
        }
    }
    return 0;
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    if (p1 != P1_FIRST || p2 != 0) {
        THROW(0x6B00);
    }

    tmpCtx.signingContext.accountNumber = readUint32BE(workBuffer);
    workBuffer += 4;
    dataLength -= 4;
    tmpCtx.signingContext.dataLength = dataLength;
    tmpCtx.signingContext.data = workBuffer;

    ux_step = 0;
    ux_step_count = 4;
    parseTx(strings.recipientAddress, strings.fullAmount, strings.fee, tmpCtx.signingContext.data);
    UX_DISPLAY(ui_approval_nanos, ui_approval_prepro);
    *flags |= IO_ASYNCH_REPLY;
}
