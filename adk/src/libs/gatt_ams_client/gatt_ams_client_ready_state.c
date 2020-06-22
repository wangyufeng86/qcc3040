/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include "gatt_ams_client_ready_state.h"

static gatt_ams_ready_state_observer_t observer_list;

void gattAmsReadyStateUpdate(const GAMS *ams, bool is_ready)
{
    if (observer_list)
        observer_list(ams, is_ready);
}

bool GattAmsAddReadyStateObserver(gatt_ams_ready_state_observer_t observer)
{
    if ((observer_list != NULL) || (observer == NULL))
        return FALSE;

    observer_list = observer;

    return TRUE;
}
