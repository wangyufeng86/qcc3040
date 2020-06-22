/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#include <string.h>
#include <stdio.h>

#include <gatt.h>

#include "gatt_root_key_client_discover.h"

#include "gatt_root_key_client_init.h"
#include "gatt_root_key_client_state.h"
#include "gatt_root_key_client_indication.h"
#include "gatt_root_key_client_write.h"

#include "gatt_root_key_server_uuids.h"

#include <uuid.h>

#include <hydra_macros.h>

/*******************************************************************************
 * Helper function to perform next function after discovering all characteristics of the service.
 */ 
static void rootKeyNextAfterDiscoverCharacteristics(GATT_ROOT_KEY_CLIENT *instance)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (root_key_client_finding_handles == state)
    {
        if (   !instance->handle_challenge_control 
            || !instance->handle_features
            || !instance->handle_keys_control
            || !instance->handle_status)
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: Did not find all expected handled",
                                        state);
            gattRootKeyClientSetState(instance, root_key_client_error);
        }
        else if (!instance->handle_challenge_control_end)
        {
            GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: Failed to find challenge_control extent",
                                        state);
            gattRootKeyClientSetState(instance, root_key_client_error);
        }
        else
        {
            rootKeyDiscoverAllCharacteristicDescriptors(instance, 
                                                        instance->handle_challenge_control+1, 
                                                        instance->handle_challenge_control_end);
            gattRootKeyClientSetState(instance, root_key_client_finding_indication_handle);
        }
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverCharacteristics: discover characteristics in bad state [0x%x]",
                                    state);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


/*******************************************************************************
 * Helper function to process a discovered characteristic descriptor.
 */ 
static void rootKeyProcessDiscoveredDescriptor(GATT_ROOT_KEY_CLIENT *instance, 
                                               const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_finding_indication_handle == state
        && gatt_status_success == cfm->status
        && gatt_uuid16 == cfm->uuid_type
        && GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_UUID == cfm->uuid[0])
    {
        instance->handle_challenge_control_config = cfm->handle;
        instance->handle_challenge_control_config_found = TRUE;
    }
    else
    {
        /* We don't cause an error here as it is feasible that additional
           descriptors could be added. So... we wait for the "end of descriptors" */
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyProcessDiscoveredDescriptor: unexpected state [0x%x] sts:%d handle:%d", 
                                    state, cfm->status, cfm->handle);
    }
}


/*******************************************************************************
 * Helper function to perform next function after discovering all descriptors of a characteristic.
 */ 
static void rootKeyNextAfterDiscoverDescriptors(GATT_ROOT_KEY_CLIENT *instance)
{
    root_key_client_state_t state = gattRootKeyClientGetState(instance);

    if (   root_key_client_finding_indication_handle == state 
        && instance->handle_challenge_control_config_found)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverDescriptors. Enabling indications");
        rootKeyWriteClientConfigValue(instance);

        gattRootKeyClientSetState(instance, root_key_client_enabling_indications);
    }
    else
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyNextAfterDiscoverDescriptors: Bad state:%d or no config handle found:%d", 
                                    state, instance->handle_challenge_control_config_found);
        gattRootKeyClientSetState(instance, root_key_client_error);
    }
}


/****************************************************************************/
void rootKeyDiscoverAllCharacteristics(GATT_ROOT_KEY_CLIENT *instance)
{
    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyDiscoverAllCharacteristics called");
    GattManagerDiscoverAllCharacteristics(&instance->lib_task);
}

static const struct {
    gatt_uuid_t expected_uuid[4];
    size_t      offset_in_instance;
} expectedCharacteristics[] = {
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROOT_KEY_FEATURES)},     offsetof(GATT_ROOT_KEY_CLIENT, handle_features) },
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROOT_KEY_STATUS)},       offsetof(GATT_ROOT_KEY_CLIENT, handle_status) },
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROOT_KEY_CHALLENGE_CONTROL)}, offsetof(GATT_ROOT_KEY_CLIENT, handle_challenge_control) },
    { {UUID_128_FORMAT_gatt_uuid_t(UUID128_ROOT_KEY_KEYS_CONTROL)},  offsetof(GATT_ROOT_KEY_CLIENT, handle_keys_control) }};

static bool rootKeyUuidCompare(const gatt_uuid_t *uuid_a, const gatt_uuid_t *uuid_b)
{
    return 0 == memcmp(uuid_a,uuid_b,sizeof(*uuid_a)*4);
}


static void rootKeyMatchAndStoreHandle(GATT_ROOT_KEY_CLIENT *instance, const gatt_uuid_t *found_uuid, uint16 found_handle)
{
    unsigned i;

    for (i = 0; i< ARRAY_DIM(expectedCharacteristics); i++)
    {
        if (rootKeyUuidCompare(found_uuid,expectedCharacteristics[i].expected_uuid))
        {
            uint16 *handle = (uint16*)(((uint8 *)instance) + expectedCharacteristics[i].offset_in_instance);
            if (0 == *handle)
            {
                *handle = found_handle;
            }
            else
            {
                GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyMatchAndStoreHandle. duplicate UUID %8x%8x%8x%8x",
                                found_uuid[0],found_uuid[1],found_uuid[2],found_uuid[3]);
            }
            return;
        }
    }

    GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyMatchAndStoreHandle. Failed to find handle for %8x-%8x-%8x-%8x",
                                    found_uuid[0],
                                    found_uuid[1],
                                    found_uuid[2],
                                    found_uuid[3]);
}

/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicsResp(GATT_ROOT_KEY_CLIENT *instance, 
                                                 const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    if (root_key_client_finding_handles != gattRootKeyClientGetState(instance))
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicsResp. Not expecting in state %d.",
                                   gattRootKeyClientGetState(instance));
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    if (cfm->status != gatt_status_success)
    {
        GATT_ROOT_KEY_CLIENT_DEBUG("rootKeyHandleDiscoverAllCharacteristicsResp. Not expecting gatt status:%d.",
                                   cfm->status);
        /* Jump to the error state as we are not tolerant of errors in this service */
        gattRootKeyClientSetState(instance, root_key_client_error);
        return;
    }

    /* The challenge control point can have indications enabled, and we
        apparently need a handle for that. Limit the range we need to look
        in by saving the last possible handle it could be*/
    if (    instance->handle_challenge_control
        && (instance->handle_challenge_control < cfm->handle) 
        && (cfm->handle <= instance->handle_challenge_control_end))
    {
        instance->handle_challenge_control_end = cfm->handle - 1;
    }

    /* We only have 128 bit uuids */
    if (cfm->uuid_type == gatt_uuid128)
    {
        rootKeyMatchAndStoreHandle(instance, cfm->uuid, cfm->handle);
    }

    if (!cfm->more_to_come)
    {
        rootKeyNextAfterDiscoverCharacteristics(instance);
    }
}


/****************************************************************************/
void rootKeyDiscoverAllCharacteristicDescriptors(GATT_ROOT_KEY_CLIENT *instance, uint16 start_handle, uint16 end_handle)
{
    GattManagerDiscoverAllCharacteristicDescriptors(&instance->lib_task,
                                                   start_handle,
                                                   end_handle);
}


/****************************************************************************/
void rootKeyHandleDiscoverAllCharacteristicDescriptorsResp(GATT_ROOT_KEY_CLIENT *instance, const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    if (cfm->status == gatt_status_success)
    {
        rootKeyProcessDiscoveredDescriptor(instance, cfm);
    }
    
    if (!cfm->more_to_come)
    {
        rootKeyNextAfterDiscoverDescriptors(instance);
    }
}
