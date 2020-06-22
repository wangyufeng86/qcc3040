/* Only for downloadable builds */
#ifdef CAPABILITY_DOWNLOAD_BUILD
#include "cbops_c.h"

/**
 * configure cbops_mute for muting/unmuting
 *
 * \param op Pointer to cbops_mute operator structure
 * \param enable Any non-zero value will mute
 * \param no_ramp if enabled it will be immediate mute/unmute
 *   else a ramping will apply during first processed block of samples.
 */
void cbops_mute_enable(cbops_op *op, bool enable, bool no_ramp)
{
    cbops_mute *params = CBOPS_PARAM_PTR(op, cbops_mute);
    params->mute_enable = enable;
    if(no_ramp)
    {
        /* no ramping, apply mute config immediately */
        params->mute_state = enable;
    }
}
#endif
