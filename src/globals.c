#include "globals.h"
#include "os.h"

#ifdef HAVE_UX_FLOW
#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
#else // HAVE_UX_FLOW
ux_state_t ux;
#endif // HAVE_UX_FLOW

// display stepped screens
unsigned int ux_step;
unsigned int ux_step_count;


const internalStorage_t N_storage_real;
