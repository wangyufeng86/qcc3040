/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <gatt_manager.h>

#include "gatt_role_selection_client.h"
#include "gatt_role_selection_client_read.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_debug.h"


/****************************************************************************/
/*! \todo What is the best approach to errors here ?

    Problem      Current response
    Wrong state  Return an IND of last state read, possibly 'unknown'
    Wrong handle Return an IND of last state read, possibly 'unknown'
    Gatt error   Return an IND of last state read, possibly 'unknown'
*/
void handleRoleSelectionReadValueResp(GATT_ROLE_SELECTION_CLIENT *instance, 
                                      const GATT_MANAGER_READ_CHARACTERISTIC_VALUE_CFM_T *read_cfm)
{
    if (role_selection_client_waiting_read == gattRoleSelectionClientGetState(instance))
    {
        GattRoleSelectionServiceMirroringState prev_state = instance->peer_state;
        GattRoleSelectionServiceMirroringState state_to_report = prev_state;

        if (instance->handles.handle_state == read_cfm->handle)
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);

            if (gatt_status_success == read_cfm->status)
            {
                GattRoleSelectionServiceMirroringState new_state = read_cfm->value[0];

                if (prev_state != new_state)
                {
                    GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionReadValueResp: peer state changed %d to %d",
                                                     prev_state, new_state);

                    instance->peer_state = state_to_report = new_state;
                }
            }
        }

        /* This will report the last state if we did not get a new one.
            This can (theoretically) be GrssMirrorStateUnknown if out of touch with 
            our peer */
        makeRoleSelectionClientStateIndMsg(instance, state_to_report);
    }
    else if (role_selection_client_waiting_read_fom == gattRoleSelectionClientGetState(instance))
    {
        grss_figure_of_merit_t prev_merit = instance->figure_of_merit;
        grss_figure_of_merit_t merit_to_report = prev_merit;

        if (instance->handles.handle_figure_of_merit == read_cfm->handle)
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);

            if (gatt_status_success == read_cfm->status)
            {
                grss_figure_of_merit_t new_merit = read_cfm->value[0] + (read_cfm->value[1]<<8);

                if (prev_merit != new_merit)
                {
                    GATT_ROLE_SELECTION_CLIENT_DEBUG("handleRoleSelectionReadValueResp: merit changed %d to %d",
                                                     prev_merit, new_merit);

                    instance->figure_of_merit = merit_to_report = new_merit;
                }
            }
        }
        /* This will report the last figure if we did not get a new one.
            This can (theoretically) be 0 if out of touch with our peer */
        makeRoleSelectionClientFigureOfMeritIndMsg(instance, merit_to_report);
    }
}


void makeRoleSelectionClientStateIndMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                        GattRoleSelectionServiceMirroringState state)
{
    MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_CLIENT_STATE_IND);

    message->instance = instance;
    message->state = state;

    MessageSend(instance->app_task, GATT_ROLE_SELECTION_CLIENT_STATE_IND, message);
}


void makeRoleSelectionClientFigureOfMeritIndMsg(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                grss_figure_of_merit_t figure_of_merit)
{
    MAKE_ROLE_SELECTION_MESSAGE(GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND);

    message->instance = instance;
    message->figure_of_merit = figure_of_merit;

    MessageSend(instance->app_task, GATT_ROLE_SELECTION_CLIENT_FIGURE_OF_MERIT_IND, message);
}


bool GattRoleSelectionClientReadPeerState(GATT_ROLE_SELECTION_CLIENT *instance)
{
    role_selection_client_state_t state;

    if (!instance)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientReadPeerState NULL client");
        return FALSE;
    }

    state = gattRoleSelectionClientGetState(instance);

    if (role_selection_client_initialised == state)
    {
        GattManagerReadCharacteristicValue(&instance->lib_task, instance->handles.handle_state);
        gattRoleSelectionClientSetState(instance,role_selection_client_waiting_read);
    }
        /* If we are already reading, give the benefit of the doubt 
           and return TRUE */
    else if (role_selection_client_waiting_read != state)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientReadPeerState attempted in bad state: %d",
                                         state);
        return FALSE;
    }

    return TRUE;
}


bool GattRoleSelectionClientReadFigureOfMerit(GATT_ROLE_SELECTION_CLIENT *instance)
{
    role_selection_client_state_t state;

    if (!instance)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientReadFigureOfMerit NULL client");
        return FALSE;
    }

    state = gattRoleSelectionClientGetState(instance);

    if (role_selection_client_initialised == state)
    {
        GattManagerReadCharacteristicValue(&instance->lib_task, instance->handles.handle_figure_of_merit);
        gattRoleSelectionClientSetState(instance,role_selection_client_waiting_read_fom);
    }
        /* If we are already reading, give the benefit of the doubt 
           and return TRUE */
    else if (role_selection_client_waiting_read_fom != state)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("GattRoleSelectionClientReadFigureOfMerit attempted in bad state: %d",
                                         state);
        return FALSE;
    }

    return TRUE;
}

