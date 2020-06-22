/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_client_state.h"

#include "gatt_root_key_client_init.h"
#include "gatt_root_key_client_write.h"

void gattRootKeyClientSetState(GATT_ROOT_KEY_CLIENT *instance, root_key_client_state_t state)
{
    root_key_client_state_t old_state = gattRootKeyClientGetState(instance);

    if (state == old_state)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("gattRootKeyClientSetState. ERROR TRANSITION TO SAME STATE:%d ??? ",state);
        return;
    }

    switch (old_state)
    {
        case root_key_client_error:
            if (state != root_key_client_disabled)
            {
                GATT_ROOT_KEY_CLIENT_DEBUG("gattRootKeyClientSetState. Attempted to exit error state. Destination %d",state);
                Panic();
            }
            break;

        default:
            break;
    }

    switch (state)
    {
        case root_key_client_error:
            GATT_ROOT_KEY_CLIENT_DEBUG("gattRootKeyClientSetState. Entered error state (from state %d)", 
                                       old_state);

            if (instance->init_response_needed)
            {
                gattRootKeyInitSendInitCfm(instance, gatt_root_key_client_status_failed);
            }
            else if (instance->challenge_response_needed)
            {
                gattRootKeySendChallengePeerInd(instance, gatt_root_key_challenge_status_fatal);
            }
            else if (instance->writekeys_response_needed)
            {
                gattRootKeySendWriteKeyInd(instance, gatt_root_key_client_write_keys_fatal);
            }
            break;

        case root_key_client_initialised:
            gattRootKeyInitSendInitCfm(instance, gatt_root_key_client_status_success);
            break;

        default:
            break;
    }

    GATT_ROOT_KEY_CLIENT_DEBUG("gattRootKeyClientSetState. Transition %d ==> %d",old_state,state);

    instance->state = state;
}

