/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "os.h"
#include "cx.h"
#include "aeUtils.h"
#include "chainConfig.h"
#include "os_io_seproxyhal.h"

#include "glyphs.h"


unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];


unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_signMessage_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_signMessage_cancel(const bagl_element_t *e);
void ui_idle(void);

uint32_t set_result_get_publicKey(void);

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_PERSONAL_MESSAGE 0x08
#define P1_CONFIRM 0x01
#define P1_NON_CONFIRM 0x00
#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

#define FULL_ADDRESS_LENGTH 54
#define BIP32_PATH 5


typedef struct publicKeyContext_t {
    cx_ecfp_public_key_t publicKey;
    uint8_t address[FULL_ADDRESS_LENGTH - 3];
    uint8_t chainCode[32];
    bool getChaincode;
} publicKeyContext_t;

typedef struct transactionContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[BIP32_PATH];
    uint8_t *data;
    uint8_t dataLength;
} transactionContext_t;

typedef struct messageSigningContext_t {
    uint8_t pathLength;
    uint32_t bip32Path[BIP32_PATH];
    uint8_t data[0xFD + 26 + 2];
    uint32_t remainingLength;
    uint32_t dataLength;
} messageSigningContext_t;

union {
    publicKeyContext_t publicKeyContext;
    transactionContext_t transactionContext;
    messageSigningContext_t messageSigningContext;
} tmpCtx;

volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;
volatile bool dataPresent;


ux_state_t ux;
// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;

typedef struct internalStorage_t {
    unsigned char dataAllowed;
    unsigned char contractDetails;
    uint8_t initialized;
} internalStorage_t;

typedef struct strData_t {
    char fullAddress[FULL_ADDRESS_LENGTH];
    char fullAmount[50];
    char maxFee[50];
} strData_t;

typedef struct strDataTmp_t {
    char tmp[100];
    char tmp2[40];
} strDataTmp_t;

union {
    strData_t common;
    strDataTmp_t tmp;
} strings;

WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t*) PIC(&N_storage_real))

static const char const SIGN_MAGIC[] = "æternity Signed Message:\n";

const unsigned char hex_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

const uint32_t HARDENED_OFFSET = 0x80000000;
const uint32_t derivePath[BIP32_PATH] = {44 | HARDENED_OFFSET, 457 | HARDENED_OFFSET, 0 | HARDENED_OFFSET,
                                         0 | HARDENED_OFFSET, 0 | HARDENED_OFFSET};
chain_config_t *chainConfig;

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
//const ux_menu_entry_t menu_settings_browser[];
const ux_menu_entry_t menu_settings_data[];
const ux_menu_entry_t menu_settings_details[];

#ifdef HAVE_U2F

// change the setting
void menu_settings_data_change(unsigned int enabled) {
    dataAllowed = enabled;
    nvm_write(&N_storage.dataAllowed, (void*)&dataAllowed, sizeof(uint8_t));
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

void menu_settings_details_change(unsigned int enabled) {
    contractDetails = enabled;
    nvm_write(&N_storage.contractDetails, (void*)&contractDetails, sizeof(uint8_t));
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_data_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_storage.dataAllowed?1:0, menu_settings_data, NULL);
}

void menu_settings_details_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_storage.contractDetails?1:0, menu_settings_details, NULL);
}

const ux_menu_entry_t menu_settings_data[] = {
    {NULL, menu_settings_data_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_data_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
};

const ux_menu_entry_t menu_settings_details[] = {
    {NULL, menu_settings_details_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_details_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
};

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_data_init, 0, NULL, "Contract data", NULL, 0, 0},
    {NULL, menu_settings_details_init, 0, NULL, "Display data", NULL, 0, 0},
    {menu_main, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};
#endif // HAVE_U2F

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION , 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
    //{NULL, NULL, 0, &NAME3(C_nanos_badge_, CHAINID, ), "Use wallet to", "view accounts", 33, 12},
    {NULL, NULL, 0, NULL, "Use wallet to", "view accounts", 0, 0},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};


const bagl_element_t ui_address_nanos[] = {
    // type                               userid    x    y   w    h  str rad fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE                      , 0x00,   0,   0, 128,  32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON                           , 0x00,   3,  12,   7,   7, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_ICON                           , 0x00, 117,  13,   8,   6, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK  }, NULL, 0, 0, 0, NULL, NULL, NULL },

    //{{BAGL_ICON                           , 0x01,  31,   9,  14,  14, 0, 0, 0        , 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_EYE_BADGE  }, NULL, 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  12, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x01,   0,  26, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "address", 0, 0, 0, NULL, NULL, NULL },

    {{BAGL_LABELINE                       , 0x02,   0,  12, 128,  12, 0, 0, 0        , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px|BAGL_FONT_ALIGNMENT_CENTER, 0}, "Address", 0, 0, 0, NULL, NULL, NULL },
    {{BAGL_LABELINE                       , 0x02,  23,  26,  82,  12, 0x80|10, 0, 0  , 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px|BAGL_FONT_ALIGNMENT_CENTER, 26  }, (char*)strings.common.fullAddress, 0, 0, 0, NULL, NULL, NULL },
};


unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);


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

unsigned int ui_approval_prepro(const bagl_element_t* element) {
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

unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

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

unsigned int
ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);

unsigned int ui_approval_signMessage_prepro(const bagl_element_t *element) {
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


const bagl_element_t ui_data_selector_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "selector", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Selector", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26}, (char *)strings.tmp.tmp, 0, 0, 0, NULL, NULL, NULL},
};

unsigned int ui_data_selector_nanos_button(unsigned int button_mask,
                                           unsigned int button_mask_counter);


const bagl_element_t ui_data_parameter_nanos[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS}, NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, "parameter", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x02, 0, 12, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0}, (char*)strings.tmp.tmp2, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 23, 26, 82, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26}, (char *)strings.tmp.tmp, 0, 0, 0, NULL, NULL, NULL},
};


void ui_idle(void) {
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    uint32_t tx = set_result_get_publicKey();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch(button_mask) {
        case BUTTON_EVT_RELEASED|BUTTON_LEFT: // CANCEL
            io_seproxyhal_touch_address_cancel(NULL);
            break;

        case BUTTON_EVT_RELEASED|BUTTON_RIGHT: { // OK
            io_seproxyhal_touch_address_ok(NULL);
            break;
        }
    }
    return 0;
}

void io_seproxyhal_send_status(uint32_t sw) {
    G_io_apdu_buffer[0] = ((sw >> 8) & 0xff);
    G_io_apdu_buffer[1] = (sw & 0xff);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
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

unsigned int ui_approval_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
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


unsigned int ui_approval_signMessage_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
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


unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx
                        // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                            sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}

uint32_t set_result_get_publicKey() {
    uint32_t tx = 0;
    uint8_t address_size = strlen(tmpCtx.publicKeyContext.address);
    G_io_apdu_buffer[tx++] = address_size;
    os_memmove(G_io_apdu_buffer + tx, tmpCtx.publicKeyContext.address, address_size);
    tx += address_size;
    return tx;
}


void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(dataLength);
    UNUSED(p1);
    UNUSED(p2);
    uint8_t privateKeyData[32];
    uint32_t bip32Path[BIP32_PATH];
    cx_ecfp_private_key_t privateKey;

    os_memmove(bip32Path, derivePath, BIP32_PATH * sizeof(uint32_t));
    uint32_t accoutNumber =
        (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
        (dataBuffer[2] << 8) | (dataBuffer[3]);
    dataBuffer += 4;
    bip32Path[2] += accoutNumber;
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip32Path, BIP32_PATH, privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, &tmpCtx.publicKeyContext.publicKey, &privateKey, 1);
    os_memset(&privateKey, 0, sizeof(privateKey));
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    getAeAddressStringFromKey(&tmpCtx.publicKeyContext.publicKey, tmpCtx.publicKeyContext.address);

    *tx = set_result_get_publicKey();
    THROW(0x9000);
}

void handleSign(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    if (p1 == P1_FIRST) {
        tmpCtx.transactionContext.pathLength = BIP32_PATH;
        os_memmove(tmpCtx.transactionContext.bip32Path, derivePath, BIP32_PATH * sizeof(uint32_t));
        uint32_t accoutNumber =
            (workBuffer[0] << 24) | (workBuffer[1] << 16) |
            (workBuffer[2] << 8) | (workBuffer[3]);
        workBuffer += 4;
        dataLength -= 4;
        tmpCtx.transactionContext.bip32Path[2] += accoutNumber;
        dataPresent = false;
        tmpCtx.transactionContext.dataLength = dataLength;
        tmpCtx.transactionContext.data = workBuffer;
    }
    else
        if (p1 != P1_MORE) {
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

void handleGetAppConfiguration(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(p2);
    UNUSED(workBuffer);
    UNUSED(dataLength);
    UNUSED(flags);
    G_io_apdu_buffer[0] = (N_storage.dataAllowed ? 0x01 : 0x00);
    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    *tx = 4;
    THROW(0x9000);
}

void handleSignPersonalMessage(uint8_t p1, uint8_t p2, uint8_t *workBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(tx);
    uint8_t messageLength;
    if (p1 == P1_FIRST) {
        os_memmove(tmpCtx.messageSigningContext.bip32Path, derivePath, BIP32_PATH * sizeof(uint32_t));
        uint32_t accoutNumber =
            (workBuffer[0] << 24) | (workBuffer[1] << 16) |
            (workBuffer[2] << 8) | (workBuffer[3]);
        workBuffer += 4;
        dataLength -= 4;
        tmpCtx.messageSigningContext.bip32Path[2] += accoutNumber;
        tmpCtx.messageSigningContext.remainingLength =
            (workBuffer[0] << 24) | (workBuffer[1] << 16) |
            (workBuffer[2] << 8) | (workBuffer[3]);
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

void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
            THROW(0x6E00);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_PUBLIC_KEY:
                    handleGetPublicKey(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                    break;

                case INS_SIGN:
                    handleSign(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                    break;

                case INS_GET_APP_CONFIGURATION:
                    handleGetAppConfiguration(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                    break;

                case INS_SIGN_PERSONAL_MESSAGE:
                    handleSignPersonalMessage(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
                    break;

                #if 0
                    case 0xFF: // return to dashboard
                    goto return_to_dashboard;
                #endif

                default:
                    THROW(0x6D00);
                    break;
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
        switch (e & 0xF000) {
            case 0x6000:
                sw = e;
                break;
            case 0x9000:
                // All is well
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

void sample_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                handleApdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
              THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                    case 0x6000:
                        // Wipe the transaction context and report the exception
                        sw = e;
                        break;
                    case 0x9000:
                        // All is well
                        sw = e;
                        break;
                    default:
                        // Internal error
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
                if (e != 0x9000) {
                    flags &= ~IO_ASYNCH_REPLY;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
            }
            // no break is intentional
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer,
            {
            if (UX_ALLOWED) {
                if (ux_step_count) {
                // prepare next screen
                ux_step = (ux_step+1)%ux_step_count;
                // redisplay screen
                UX_REDISPLAY();
                }
            }
            });
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {

    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {

        }
    }
    END_TRY_L(exit);
}

chain_config_t const C_chain_config = {
    .coinName = CHAINID_COINNAME " ",
    .chainId = CHAIN_ID,
    .kind = CHAIN_KIND,
};

__attribute__((section(".boot"))) int main(int arg0) {
#ifdef USE_LIB_ETHEREUM
    chain_config_t local_chainConfig;
    os_memmove(&local_chainConfig, &C_chain_config, sizeof(chain_config_t));
    unsigned int libcall_params[3];
    unsigned char coinName[sizeof(CHAINID_COINNAME)];
    strcpy(coinName, CHAINID_COINNAME);
    local_chainConfig.coinName = coinName;
    BEGIN_TRY {
        TRY {
            // ensure syscall will accept us
            check_api_level(CX_COMPAT_APILEVEL);
            // delegate to Ethereum app/lib
            libcall_params[0] = "Ethereum";
            libcall_params[1] = 0x100; // use the Init call, as we won't exit
            libcall_params[2] = &local_chainConfig;
            os_lib_call(&libcall_params);
        }
        FINALLY {
            app_exit();
        }
    }
    END_TRY;
#else
    // exit critical section
    __asm volatile("cpsie i");

    if (arg0) {
        if (((unsigned int *)arg0)[0] != 0x100) {
            os_lib_throw(INVALID_PARAMETER);
        }
        chainConfig = (chain_config_t *)((unsigned int *)arg0)[1];
    }
    else {
        chainConfig = (chain_config_t *)PIC(&C_chain_config);
    }

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                if (N_storage.initialized != 0x01) {
                    internalStorage_t storage;
                    storage.dataAllowed = 0x00;
                    storage.contractDetails = 0x00;
                    storage.initialized = 0x01;
                    nvm_write(&N_storage, (void*)&storage, sizeof(internalStorage_t));
                }
                dataAllowed = N_storage.dataAllowed;
                contractDetails = N_storage.contractDetails;

                USB_power(0);
                USB_power(1);

                ui_idle();

                sample_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX before continuing
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();
#endif
    return 0;
}
