/*******************************************************************************
Copyright (c) 2015-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_enabled_state.c

DESCRIPTION
    Event handling functions for the ANC Enabled State.
*/

#include "anc_enabled_state.h"
#include "anc_common_state.h"
#include "anc_data.h"
#include "anc_debug.h"
#include "anc_configure.h"
#include "anc_config_write.h"

static bool disableAncEventHandler(void)
{
    bool disabled = FALSE;
    if(ancConfigure(FALSE))
    {
        ancDataSetState(anc_state_disabled);
        disabled = TRUE;
    }
    return disabled;
}

static bool setFilterPathGainEventHandler(anc_state_event_t event)
{
    bool gain_set = FALSE;

    gain_set = ancConfigureFilterPathGain(((anc_state_event_set_path_gain_args_t *)event.args)->instance,
                                          ((anc_state_event_set_path_gain_args_t *)event.args)->path,
                                          ((anc_state_event_set_path_gain_args_t *)event.args)->gain);

    return gain_set;
}

static bool writeFineGainToPsEventHandler(anc_state_event_t event)
{
    bool gain_set = FALSE;

    gain_set = ancWriteFineGain(ancDataGetMode(),
                                          ((anc_state_event_write_gain_args_t *)event.args)->path,
                                          ((anc_state_event_write_gain_args_t *)event.args)->gain);

    return gain_set;
}


/******************************************************************************/
bool ancStateEnabledHandleEvent(anc_state_event_t event)
{
    bool success = FALSE;

    switch (event.id)
    {
        case anc_state_event_disable:
        {
            success = disableAncEventHandler();
        }
        break;

        case anc_state_event_set_mode:
        {
            if (ancCommonStateHandleSetMode(event))
            {
                success = ancConfigureAfterModeChange();
            }
        }
        break;

        case anc_state_event_set_filter_path_gain:
        {
            success = setFilterPathGainEventHandler(event);
        }
        break;
               
        case anc_state_event_write_fine_gain:
        {
            success = writeFineGainToPsEventHandler(event);
        }
        break;


        default:
        {
            ANC_DEBUG_INFO(("Unhandled event [%d]\n", event.id));
            ANC_PANIC();
        }
        break;
    }
    return success;
}
