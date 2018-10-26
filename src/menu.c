#include "menu.h"

volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;

#ifdef HAVE_U2F

static const ux_menu_entry_t menu_main[];
static const ux_menu_entry_t menu_settings[];
static const ux_menu_entry_t menu_settings_data[];
static const ux_menu_entry_t menu_settings_details[];

// change the setting
static void menu_settings_data_change(unsigned int enabled) {
    dataAllowed = enabled;
    nvm_write(&N_storage.dataAllowed, (void*)&dataAllowed, sizeof(uint8_t));
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

static void menu_settings_details_change(unsigned int enabled) {
    contractDetails = enabled;
    nvm_write(&N_storage.contractDetails, (void*)&contractDetails, sizeof(uint8_t));
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

// show the currently activated entry
static void menu_settings_data_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_storage.dataAllowed?1:0, menu_settings_data, NULL);
}

static static void menu_settings_details_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_storage.contractDetails?1:0, menu_settings_details, NULL);
}

static const ux_menu_entry_t menu_settings_data[] = {
    {NULL, menu_settings_data_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_data_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
};

static const ux_menu_entry_t menu_settings_details[] = {
    {NULL, menu_settings_details_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_details_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
};

static const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_data_init, 0, NULL, "Contract data", NULL, 0, 0},
    {NULL, menu_settings_details_init, 0, NULL, "Display data", NULL, 0, 0},
    {menu_main, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};
#endif // HAVE_U2F

static const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION , 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
};

static const ux_menu_entry_t menu_main[] = {
    //{NULL, NULL, 0, &NAME3(C_nanos_badge_, CHAINID, ), "Use wallet to", "view accounts", 33, 12},
    {NULL, NULL, 0, NULL, "Use wallet to", "view accounts", 0, 0},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
};

void ui_idle(void) {
    UX_MENU_DISPLAY(0, menu_main, NULL);
}
