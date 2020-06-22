/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_root_key_server_indicate.h"
#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_state.h"

#include "gatt_root_key_server_db.h"


/****************************************************************************/
bool gattRootKeyServerSendChallengeIndication(const GATT_ROOT_KEY_SERVER *instance,
                                              uint16 cid, uint8 opcode,
                                              const GRKS_KEY_T *key)
{
    bool result = FALSE;
    uint8 indication[GRKS_SIZE_CONTROL_WITH_KEY_OCTETS];

    if (instance != NULL)
    {
        result = TRUE;

        indication[0] = opcode;
        memcpy(&indication[1], key->key, sizeof(indication)-1);

        GattManagerRemoteClientIndicate((Task)&instance->lib_task, 
                                        cid, 
                                        HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT, 
                                        GRKS_SIZE_CONTROL_WITH_KEY_OCTETS, 
                                        indication);
    }

    return result;
}


static void handleRootKeyChallengeIndicationDelivered(GATT_ROOT_KEY_SERVER *instance)
{
    root_key_server_state_t state = gattRootKeyServerGetState(instance);

    switch (state)
    {
        case root_key_server_responded_random:
            gattRootKeyServerSetState(instance, root_key_server_awaiting_hash);
            break;

        case root_key_server_responded_hash:
            gattRootKeyServerSetState(instance, root_key_server_authenticated);
            break;

        default:
            gattRootKeyServerSetState(instance, root_key_server_error);
            break;
    }

}


static void handleRootKeyIndicationDelivered(GATT_ROOT_KEY_SERVER *instance,
                                             uint16 handle)
{
    switch (handle)
    {
        case HANDLE_ROOT_TRANSFER_SERVICE_MIRROR_CONTROL_POINT:
            handleRootKeyChallengeIndicationDelivered(instance);
            break;

        default:
            gattRootKeyServerSetState(instance, root_key_server_error);
            break;
    }
}


void handleRootKeyIndicationCfm(GATT_ROOT_KEY_SERVER *instance,
                                const GATT_MANAGER_REMOTE_CLIENT_INDICATION_CFM_T *payload)
{
    switch (payload->status)
    {
        case gatt_status_success_sent:
            /* Indicates that has been sent successfully. Ignore */
            GATT_ROOT_KEY_SERVER_DEBUG("handleRootKeyIndicationCfm. Indication handle:%d sent",
                                       payload->handle);
            break;

        case gatt_status_success:
            handleRootKeyIndicationDelivered(instance, payload->handle);
            break;

        default:
            GATT_ROOT_KEY_SERVER_DEBUG_PANIC();
            break;
    }
}

