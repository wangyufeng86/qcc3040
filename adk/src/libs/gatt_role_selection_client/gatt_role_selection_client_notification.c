/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */


#include <message.h>

#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_private.h"
#include "gatt_role_selection_client_read.h"
#include "gatt_role_selection_client_discover.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_debug.h"


/****************************************************************************
Internal functions
****************************************************************************/

/***************************************************************************/
void handleRoleSelectionNotification(GATT_ROLE_SELECTION_CLIENT *instance, 
                                     const GATT_MANAGER_REMOTE_SERVER_NOTIFICATION_IND_T *ind)
{
    if (    GRSS_SIZE_MIRROR_STATE_PDU_OCTETS == ind->size_value
        && (ind->handle == instance->handles.handle_state))
    {
        GattRoleSelectionServiceMirroringState state = (GattRoleSelectionServiceMirroringState)ind->value[0];

        if (state != instance->peer_state)
        {
            makeRoleSelectionClientStateIndMsg(instance, state);
            instance->peer_state = state;
        }
    }
    else if (    GRSS_SIZE_FIGURE_OF_MERIT_PDU_OCTETS == ind->size_value
             && (ind->handle == instance->handles.handle_figure_of_merit))
    {
        grss_figure_of_merit_t figure_of_merit = (grss_figure_of_merit_t)(ind->value[0])
                                                    + ((grss_figure_of_merit_t)(ind->value[1])<<8);

        if (figure_of_merit != instance->figure_of_merit)
        {
            makeRoleSelectionClientFigureOfMeritIndMsg(instance, figure_of_merit);
            instance->figure_of_merit = figure_of_merit;
        }
    }
    else
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionNotification. Unexpected handle:%d or size:%d",
                                    ind->handle, ind->size_value);
    }
}


bool GattRoleSelectionClientEnablePeerStateNotifications(GATT_ROLE_SELECTION_CLIENT *instance)
{
    role_selection_client_state_t state;

    if (!instance)
    {
        return FALSE;
    }

    state = gattRoleSelectionClientGetState(instance);
    switch (state)
    {
        case role_selection_client_initialised:
        case role_selection_client_finding_notification_handle:
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientEnablePeerStateNotifications called in bad state:%d",
                                             state);
            return FALSE;
    }

    if (role_selection_client_finding_notification_handle == state)
    {
        /* We are already in the process of finding a handle */
        return TRUE;
    }

    if (instance->handles.handle_state_config)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification);
        roleSelectionWriteStateClientConfigValue(instance);
    }
    else if (instance->handles.handle_state && instance->handles.handle_state_end)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_finding_notification_handle);
        roleSelectionDiscoverAllCharacteristicDescriptors(instance,
                                                          instance->handles.handle_state,
                                                          instance->handles.handle_state_end);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

bool GattRoleSelectionClientEnablePeerFigureOfMeritNotifications(GATT_ROLE_SELECTION_CLIENT *instance)
{
    role_selection_client_state_t state;

    if (!instance)
    {
        return FALSE;
    }

    state = gattRoleSelectionClientGetState(instance);
    switch (state)
    {
        case role_selection_client_initialised:
        case role_selection_client_finding_notification_handle_fom:
            break;

        default:
            GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientEnablePeerFigureOfMeritNotifications called in bad state:%d",
                                             state);
            return FALSE;
    }

    if (role_selection_client_finding_notification_handle_fom == state)
    {
        /* We are already in the process of finding a handle */
        return TRUE;
    }

    if (instance->handles.handle_figure_of_merit_config)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification_fom);
        roleSelectionWriteFigureOfMeritClientConfigValue(instance);
    }
    else if (instance->handles.handle_figure_of_merit && instance->handles.handle_figure_of_merit_end)
    {
        gattRoleSelectionClientSetState(instance, role_selection_client_finding_notification_handle_fom);
        roleSelectionDiscoverAllCharacteristicDescriptors(instance,
                                                          instance->handles.handle_figure_of_merit,
                                                          instance->handles.handle_figure_of_merit_end);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

