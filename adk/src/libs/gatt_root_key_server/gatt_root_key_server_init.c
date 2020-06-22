/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_root_key_server_private.h"

#include "gatt_root_key_server_state.h"
#include "gatt_root_key_server_msg_handler.h"
#include "gatt_root_key_server_db.h"


/****************************************************************************/
bool GattRootKeyServerInit(GATT_ROOT_KEY_SERVER *instance, 
                           Task app_task,
                           const gatt_root_key_server_init_params_t *init_params,
                           uint16 start_handle,
                           uint16 end_handle)
{
    gatt_manager_server_registration_params_t registration_params;

    if ((app_task == NULL) || (instance == NULL) || (init_params == NULL))
    {
        GATT_ROOT_KEY_SERVER_DEBUG("GattRootKeyServerInit: Invalid Initialisation parameters");
        Panic();
        return FALSE;
    }

    memset(instance, 0, sizeof(*instance));

    /* Set up library handler for external messages */
    instance->lib_task.handler = rootKeyServerMsgHandler;

    /* Store the Task function parameter.
       All library messages need to be sent here */
    instance->app_task = app_task;

    instance->features = init_params->features;

    instance->status = 0;

    /*! \todo Can the initial mirror client config sensibly come in init_params
                And can it sensibly be stored (for this server use case) */
    instance->mirror_client_config = 0;
    
    instance->commit_cid = INVALID_CID;

    /* Register the service with gatt manager */
    registration_params.task = &instance->lib_task;
    registration_params.start_handle = start_handle;
    registration_params.end_handle = end_handle;
        
    if (GattManagerRegisterServer(&registration_params) == gatt_manager_status_success)
    {
        gattRootKeyServerSetState(instance, root_key_server_idle);
        return TRUE;
    }
    return FALSE;
}
