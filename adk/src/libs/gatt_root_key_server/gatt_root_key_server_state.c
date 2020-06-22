/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_state.h"

#include "gatt_root_key_server_challenge.h"




void gattRootKeyServerSetState(GATT_ROOT_KEY_SERVER *instance, root_key_server_state_t state)
{
    root_key_server_state_t old_state = gattRootKeyServerGetState(instance);

    if (state == old_state)
    {
        GATT_ROOT_KEY_SERVER_DEBUG("gattRootKeyServerSetState. ERROR TRANSITION TO SAME STATE:%d ??? ",state);
        return;
    }

    switch (old_state)
    {
        case root_key_server_error:
            if (state != root_key_server_disabled)
            {
                GATT_ROOT_KEY_SERVER_DEBUG("gattRootKeyServerSetState. Attempted to exit error state. Destination %d",state);
                Panic();
            }
            break;

        default:
            break;
    }

    switch (state)
    {
        case root_key_server_authenticated:
            /* Always send a message when we authenticate. Flag should always be set */
            if (!instance->challenge_response_needed)
            {
                GATT_ROOT_KEY_SERVER_DEBUG("gattRootKeyServerSetState. Got to authenticate with no message needed?");
            }
            gattRootKeySendChallengeInd(instance, gatt_root_key_challenge_status_success);
            break;

        case root_key_server_error:
            GATT_ROOT_KEY_SERVER_DEBUG("gattRootKeyServerSetState. Entered error state (from state %d)", 
                                       old_state);

            if (instance->challenge_response_needed)
            {
                gattRootKeySendChallengeInd(instance, gatt_root_key_challenge_status_fatal);
            }
            break;

        default:
            break;
    }

    GATT_ROOT_KEY_SERVER_DEBUG("gattRootKeyServerSetState. Transition %d ==> %d",old_state,state);

    instance->state = state;
}

