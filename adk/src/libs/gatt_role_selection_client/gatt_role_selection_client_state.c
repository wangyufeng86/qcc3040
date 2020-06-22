/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_role_selection_client_state.h"

#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_debug.h"

void gattRoleSelectionClientSetState(GATT_ROLE_SELECTION_CLIENT *instance, role_selection_client_state_t state)
{
    role_selection_client_state_t old_state = gattRoleSelectionClientGetState(instance);

    if (state == old_state)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("gattRoleSelectionClientSetState. ERROR TRANSITION TO SAME STATE:%d ??? ",state);
        return;
    }

    if (role_selection_client_error == old_state)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("gattRoleSelectionClientSetState. Attempt to leave error state (to state %d)",
                                         state);
        GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC();
        return;
    }

    GATT_ROLE_SELECTION_CLIENT_DEBUG("gattRoleSelectionClientSetState %d==>%d",old_state,state);

    switch (state)
    {
        case role_selection_client_error:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("gattRoleSelectionClientSetState. Entered error state (from state %d)", 
                                       old_state);

            gattRoleSelectionInitSendInitCfm(instance, 
                                             gatt_role_selection_client_status_failed);
            gattRoleSelectionSendClientCommandCfmMsg(instance, gatt_status_failure);
            break;

        case role_selection_client_initialised:
            gattRoleSelectionInitSendInitCfm(instance, 
                                             gatt_role_selection_client_status_success);
            instance->active_procedure = FALSE;
            break;

        case role_selection_client_finding_handles:
        case role_selection_client_uninitialised:
            break;

        default:
            /* Keep the procedure flag set until we return to initialised */
            instance->active_procedure = TRUE;
            break;
    }

    instance->state = state;
}

