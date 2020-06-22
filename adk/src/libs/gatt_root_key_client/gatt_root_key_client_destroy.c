/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include "gatt_root_key_client_private.h"

#include "gatt_root_key_client_msg_handler.h"


/****************************************************************************/
bool GattRootKeyClientDestroy(GATT_ROOT_KEY_CLIENT *instance)
{
    bool result = FALSE;

    /* Check parameters */
    if (instance == NULL)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("GattRootKeyClientDestroy: Invalid parameters");
        Panic();
        return FALSE;
    }

    /* Register with the GATT Manager and verify the result */
    result = (GattManagerUnregisterClient(&instance->lib_task) == gatt_manager_status_success);
        
    /* Clear pending messages */
    MessageFlushTask(&instance->lib_task);
    
    return result;
}
