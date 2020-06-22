/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_role_selection_client_discover.h"

#include "gatt_role_selection_client_init.h"
#include "gatt_role_selection_client_state.h"
#include "gatt_role_selection_client_notification.h"
#include "gatt_role_selection_client_write.h"
#include "gatt_role_selection_client_debug.h"

#include "gatt_role_selection_server_uuids.h"

#include <uuid.h>

#include <hydra_macros.h>

/*******************************************************************************
 * Helper function to perform next function after discovering all characteristics of the service.
 */ 
static void roleSelectionNextAfterDiscoverCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance) 
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if (role_selection_client_finding_handles == state)
    {
        if (   !instance->handles.handle_state
            || !instance->handles.handle_figure_of_merit
            || !instance->handles.handle_role_control)
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionNextAfterDiscoverCharacteristics: Did not find all expected handles %d %d %d",
                                                instance->handles.handle_state,
                                                instance->handles.handle_figure_of_merit,
                                                instance->handles.handle_role_control);
            gattRoleSelectionClientSetState(instance, role_selection_client_error);
        }
        else
        {
            gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
        }
    }
    else
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionNextAfterDiscoverCharacteristics: discover characteristics in bad state [0x%x]",
                                          state);
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
    }
}


/*******************************************************************************
 * process a discovered characteristic descriptor, saving if we have none
 */ 
static void roleSelectionProcessDiscoveredDescriptor(GATT_ROLE_SELECTION_CLIENT *instance, 
                                               const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

    if (   gatt_status_success == cfm->status
        && !instance->handles.handle_state_config
        && gatt_uuid16 == cfm->uuid_type
        && GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID == cfm->uuid[0])
    {
        if (role_selection_client_finding_notification_handle == state)
        {
            instance->handles.handle_state_config = cfm->handle;
        }
        else if (role_selection_client_finding_notification_handle_fom == state)
        {
            instance->handles.handle_figure_of_merit_config = cfm->handle;
        }
        else
        {
            GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionProcessDiscoveredDescriptor: unexpected cfm in state:%d",
                                             state);
        }
    }
    else
    {
        /* We don't cause an error here as it is feasible that additional
           descriptors could be added. So... we wait for the "end of descriptors" */
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionProcessDiscoveredDescriptor: unexpected cfm");
    }
}


/****************************************************************************/
void roleSelectionDiscoverAllCharacteristics(GATT_ROLE_SELECTION_CLIENT *instance)
{
    GattManagerDiscoverAllCharacteristics(&instance->lib_task);
}

static const struct {
    gatt_uuid_t expected_uuid[4];
    size_t      offset_in_instance;
} expectedCharacteristics[] = {
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROLE_SEL_MIRRORING_STATE)},     offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_state) },
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROLE_SEL_CONTROL_POINT)},       offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_role_control) },
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROLE_SEL_FIGURE_OF_MERIT)},     offsetof(GATT_ROLE_SELECTION_CLIENT, handles.handle_figure_of_merit) } };

static bool roleSelectionUuidCompare(const gatt_uuid_t *uuid_a, const gatt_uuid_t *uuid_b)
{
    return 0 == memcmp(uuid_a,uuid_b,sizeof(*uuid_a)*4);
}


static void roleSelectionMatchAndStoreHandle(GATT_ROLE_SELECTION_CLIENT *instance, const gatt_uuid_t *found_uuid, uint16 found_handle)
{
    unsigned i;

    for (i = 0; i< ARRAY_DIM(expectedCharacteristics); i++)
    {
        if (roleSelectionUuidCompare(found_uuid,expectedCharacteristics[i].expected_uuid))
        {
            uint16 *handle = (uint16*)(((uint8 *)instance) + expectedCharacteristics[i].offset_in_instance);
            if (0 == *handle)
            {
                *handle = found_handle;
            }
            else
            {
                GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionMatchAndStoreHandle. duplicate UUID %8x%8x%8x%8x",
                                found_uuid[0],found_uuid[1],found_uuid[2],found_uuid[3]);
            }
            return;
        }
    }
}

/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicsResp(GATT_ROLE_SELECTION_CLIENT *instance, 
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    if (role_selection_client_finding_handles != gattRoleSelectionClientGetState(instance))
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicsResp. Not expecting in state:%d",
                                         gattRoleSelectionClientGetState(instance));

        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    if (cfm->status != gatt_status_success)
    {
        GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicsResp. Gatt error:%d.",
                                         cfm->status);
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRoleSelectionClientSetState(instance, role_selection_client_error);
        return;
    }

    /* The challenge control point can have indications enabled, and we
        apparently need a handle for that. Limit the range we need to look
        in by saving the last possible handle it could be*/
    if (    instance->handles.handle_state
        && (instance->handles.handle_state < cfm->handle)
        && (cfm->handle <= instance->handles.handle_state_end))
    {
        instance->handles.handle_state_end = cfm->handle - 1;
    }

    if (    instance->handles.handle_figure_of_merit
        && (instance->handles.handle_figure_of_merit < cfm->handle)
        && (cfm->handle <= instance->handles.handle_figure_of_merit_end))
    {
        instance->handles.handle_figure_of_merit_end = cfm->handle - 1;
    }

    /* We only have 128 bit uuids */
    if (cfm->uuid_type == gatt_uuid128)
    {
        roleSelectionMatchAndStoreHandle(instance, cfm->uuid, cfm->handle);
    }

    if (!cfm->more_to_come)
    {
        roleSelectionNextAfterDiscoverCharacteristics(instance);
    }
}


/****************************************************************************/
void roleSelectionDiscoverAllCharacteristicDescriptors(GATT_ROLE_SELECTION_CLIENT *instance, uint16 start_handle, uint16 end_handle)
{
    GattManagerDiscoverAllCharacteristicDescriptors(&instance->lib_task,
                                                   start_handle,
                                                   end_handle);
}


/****************************************************************************/
void roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp(GATT_ROLE_SELECTION_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    roleSelectionProcessDiscoveredDescriptor(instance, cfm);

    if (!cfm->more_to_come)
    {
        role_selection_client_state_t state = gattRoleSelectionClientGetState(instance);

        switch (state)
        {
            case role_selection_client_finding_notification_handle:
                if (instance->handles.handle_state_config)
                {
                    roleSelectionWriteStateClientConfigValue(instance);

                    gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification);
                }
                else
                {
                    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. Ended search with no descriptor");

                    gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
                }
                break;

            case role_selection_client_finding_notification_handle_fom:
                if (instance->handles.handle_figure_of_merit_config)
                {
                    roleSelectionWriteFigureOfMeritClientConfigValue(instance);

                    gattRoleSelectionClientSetState(instance, role_selection_client_setting_notification_fom);
                }
                else 
                {
                    GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. Ended search with no FOM descriptor");

                    gattRoleSelectionClientSetState(instance, role_selection_client_initialised);
                }
                break;

            default:
                GATT_ROLE_SELECTION_CLIENT_DEBUG("roleSelectionHandleDiscoverAllCharacteristicDescriptorsResp. descriptor in unxpected state:%d",
                                                 state);
                GATT_ROLE_SELECTION_CLIENT_DEBUG_PANIC();
                break;
        }
    }
}

