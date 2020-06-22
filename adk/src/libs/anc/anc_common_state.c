/*******************************************************************************
Copyright (c) 2015-2020 Qualcomm Technologies International, Ltd.


FILE NAME
    anc_common_state.c

DESCRIPTION
    Event handling functions that are common to more than one state.
*/

#include "anc_common_state.h"
#include "anc_data.h"
#include "anc_debug.h"

#include <stdlib.h>

/******************************************************************************/
bool ancCommonStateHandleSetMode(anc_state_event_t event)
{
    /* Ensure we have a set mode event */
    ANC_ASSERT(event.id == anc_state_event_set_mode);
    if (event.id != anc_state_event_set_mode) return FALSE;

    /* Set Mode event must have args */
    ANC_ASSERT(event.args != NULL);
    if (event.args == NULL) return FALSE;

    /* Store new value */
    return (ancDataSetMode(((anc_state_event_set_mode_args_t *) event.args)->mode));
}
