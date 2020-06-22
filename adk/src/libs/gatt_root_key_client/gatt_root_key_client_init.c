/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <util.h>

#include "gatt_root_key_client_init.h"

#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_discover.h"
#include "gatt_root_key_client_msg_handler.h"


/******************************************************************************/
void gattRootKeyInitSendInitCfm(GATT_ROOT_KEY_CLIENT *instance,
                                gatt_root_key_client_status_t status)
{
    MAKE_ROOT_KEY_MESSAGE(GATT_ROOT_KEY_CLIENT_INIT_CFM);
    message->instance = instance;
    message->status = status;

    MessageSend(instance->app_task, GATT_ROOT_KEY_CLIENT_INIT_CFM, message);

    instance->init_response_needed = FALSE;
}


/****************************************************************************/
bool GattRootKeyClientInit(GATT_ROOT_KEY_CLIENT *instance, 
                           Task app_task,
                           uint16 cid,
                           uint16 start_handle,
                           uint16 end_handle)
{
    gatt_manager_client_registration_params_t registration_params;

    /* Check parameters */
    if ((app_task == NULL) || (instance == NULL))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientInit: Invalid parameters");
        Panic();
        return FALSE;
    }

    memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));
        
    /* Set memory contents to all zeros */
    memset(instance, 0, sizeof(GATT_ROOT_KEY_CLIENT));

    /* Set up library handler for external messages */
    instance->lib_task.handler = rootKeyClientMsgHandler;

    /* Store the Task function parameter.
       All library messages need to be sent here */
    instance->app_task = app_task;

    /* Set the challenge control characteristic end at the end of the service 
       The memset above ensured the handle is unknown (0) */
    instance->handle_challenge_control_end = end_handle;

    /* Setup data required for Battery Service to be registered with the GATT Manager */
    registration_params.client_task = &instance->lib_task;
    registration_params.cid = cid;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;

    /* Register with the GATT Manager and verify the result */
    if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
    {
        /* Discover all characteristics and descriptors after successful registration */
        rootKeyDiscoverAllCharacteristics(instance);
        gattRootKeyClientSetState(instance, root_key_client_finding_handles);

        instance->init_response_needed = TRUE;
        return TRUE;
    }

    gattRootKeyClientSetState(instance, root_key_client_error);
    return FALSE;
}

