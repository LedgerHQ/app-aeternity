#include "menu.h"
#include "os.h"

volatile uint8_t dataAllowed;
volatile uint8_t contractDetails;

void display_settings(void);
void switch_settings_contract_data(void);
void switch_settings_display_data(void);

//////////////////////////////////////////////////////////////////////


const char* settings_submenu_getter(unsigned int idx);
void settings_submenu_selector(unsigned int idx);


//////////////////////////////////////////////////////////////////////////////////////
// Enable contract data submenu:

void settings_contract_data_change(unsigned int enabled) {
    nvm_write((void *)&N_storage.dataAllowed, &enabled, 1);
    ui_idle();
}

const char* const settings_contract_data_getter_values[] = {
  "No",
  "Yes",
  "Back"
};

const char* settings_contract_data_getter(unsigned int idx) {
  if (idx < ARRAYLEN(settings_contract_data_getter_values)) {
    return settings_contract_data_getter_values[idx];
  }
  return NULL;
}

void settings_contract_data_selector(unsigned int idx) {
  switch(idx) {
    case 0:
      settings_contract_data_change(0);
      break;
    case 1:
      settings_contract_data_change(1);
      break;
    default:
      ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector);
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Display contract data submenu:

void settings_display_data_change(unsigned int enabled) {
    nvm_write((void *)&N_storage.contractDetails, &enabled, 1);
    ui_idle();
}

const char* const settings_display_data_getter_values[] = {
  "No",
  "Yes",
  "Back"
};

const char* settings_display_data_getter(unsigned int idx) {
  if (idx < ARRAYLEN(settings_display_data_getter_values)) {
    return settings_display_data_getter_values[idx];
  }
  return NULL;
}

void settings_display_data_selector(unsigned int idx) {
  switch(idx) {
    case 0:
      settings_display_data_change(0);
      break;
    case 1:
      settings_display_data_change(1);
      break;
    default:
      ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector);
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Settings menu:

const char* const settings_submenu_getter_values[] = {
  "Contract data",
  "Display data",
  "Back",
};

const char* settings_submenu_getter(unsigned int idx) {
  if (idx < ARRAYLEN(settings_submenu_getter_values)) {
    return settings_submenu_getter_values[idx];
  }
  return NULL;
}

void settings_submenu_selector(unsigned int idx) {
  switch(idx) {
    case 0:
      ux_menulist_init_select(0, settings_contract_data_getter, settings_contract_data_selector, N_storage.dataAllowed);
      break;
    case 1:
      ux_menulist_init_select(0, settings_display_data_getter, settings_display_data_selector, N_storage.contractDetails);
      break;
    default:
      ui_idle();
  }
}

//////////////////////////////////////////////////////////////////////
UX_STEP_NOCB(
    ux_idle_flow_1_step,
    nn,
    {
      "Application",
      "is ready",
    });
UX_STEP_VALID(
    ux_idle_flow_2_step,
    pb,
    ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
    {
      &C_icon_coggle,
      "Settings",
    });
UX_STEP_NOCB(
    ux_idle_flow_3_step,
    bn,
    {
      "Version",
      APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_4_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard_x,
      "Quit",
    });
UX_FLOW(ux_idle_flow,
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step,
  &ux_idle_flow_4_step
);


void ui_idle(void) {
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}
