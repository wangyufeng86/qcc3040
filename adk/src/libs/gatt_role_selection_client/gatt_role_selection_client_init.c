/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdio.h>

#include <util.h>

#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_discover.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_debug.h"

#include "gatt_role_selection_client_msg_handler.h"


/******************************************************************************/
void gattRoleSelectionInitSendInitCfm(GATT_ROLE_SELECTION_CLIENT *instance,
                                      gatt_role_selection_client_status_t status)
{
    if (instance->init_response_needed)
    {
        MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_CLIENT_INIT_CFM);
        message->instance = instance;
        message->status = status;

        MessageSend(instance->app_task, GATT_ROLE_SELECTION_CLIENT_INIT_CFM, message);

        instance->init_response_needed = FALSE;
    }
}

/****************************************************************************/
bool GattRoleSelectionClientDestroy(GATT_ROLE_SELECTION_CLIENT *instance)
{
    PanicNull(instance);

    instance->active_procedure = FALSE;
    instance->state = role_selection_client_uninitialised;

    return GattManagerUnregisterClient(&instance->lib_task) == gatt_manager_status_success; 
}

/****************************************************************************/
bool GattRoleSelectionClientInit(GATT_ROLE_SELECTION_CLIENT *instance, 
                                 Task app_task,
                                 gatt_role_selection_handles_t *cached_handles,
                                 uint16 cid,
                                 uint16 start_handle,
                                 uint16 end_handle)
{
    gatt_manager_client_registration_params_t registration_params;

    /* Check parameters */
    if ((app_task == NULL) || (instance == NULL))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientInit: Invalid parameters");
        Panic();
        return FALSE;
    }

    memset(&registration_params, 0, sizeof(gatt_manager_client_registration_params_t));
        
    /* Set memory contents to all zeros */
    memset(instance, 0, sizeof(GATT_ROLE_SELECTION_CLIENT));

    instance->lib_task.handler = roleSelectionClientMsgHandler;
    instance->app_task = app_task;

    instance->peer_state = GrssMirrorStateUnknown;



    /* Setup data required for Battery Service to be registered with the GATT Manager */
    registration_params.client_task = &instance->lib_task;
    registration_params.cid = cid;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;

    if(cached_handles)
    {
        instance->handles = *cached_handles;
    }
    else
    {/* Set the role control characteristic end at the end of the service 
        The memset above ensured the handle is unknown (0) */
        instance->handles.handle_state_end = end_handle;
        instance->handles.handle_figure_of_merit_end = end_handle;
    }
    

    /* Register with the GATT Manager and verify the result */
    if (GattManagerRegisterClient(&registration_params) == gatt_manager_status_success)
    {
        instance->init_response_needed = TRUE;

        if(cached_handles)
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        }
        else
        {   /* Discover all characteristics and descriptors after successful registration */
            roleSelectionDiscoverAllCharacteristics(instance);
            gattRoleSelectionClientSetState(instance, role_selection_client_finding_handles);
        }
        return TRUE;
    }

    gattRoleSelectionClientSetState(instance, role_selection_client_error);
    return FALSE;
}

