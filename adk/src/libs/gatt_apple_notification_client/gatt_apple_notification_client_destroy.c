/* Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_apple_notification_client.h"
#include "gatt_apple_notification_client_private.h"
#include "gatt_apple_notification_client_ready_state.h"
#include <gatt_manager.h>

bool GattAncsDestroy(void *ancs_dynamic)
{
    GANCS* ancs = (GANCS*)ancs_dynamic;

    if (ancs == NULL)
        return FALSE;

    if (GattManagerUnregisterClient(&ancs->lib_task) != gatt_manager_status_success)
        return FALSE;

    gattAncsReadyStateUpdate(ancs, FALSE);

    MessageFlushTask(&ancs->lib_task);

    PRINT(("ANCS: Destroyed instance [%p] with cid [0x%02x]\n", (void *) ancs, ancs->cid));

    return TRUE;
}
