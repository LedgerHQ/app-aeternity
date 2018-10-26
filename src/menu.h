#include "globals.h"
#include "glyphs.h"

#ifndef _MENU_H_
#define _MENU_H_

extern const ux_menu_entry_t menu_main[];
extern const ux_menu_entry_t menu_settings[];
extern const ux_menu_entry_t menu_settings_data[];
extern const ux_menu_entry_t menu_settings_details[];

extern volatile uint8_t dataAllowed;
extern volatile uint8_t contractDetails;

void ui_idle(void);

#endif
