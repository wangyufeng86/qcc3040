/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT clients
*/
/* Internal info: See https://confluence.qualcomm.com/confluence/display/ADKAPP/Gatt+Client+Design for design details */

#include <stdlib.h>
#include "gatt_client.h"
#include "gatt_client_protected.h"
#include <panic.h>

typedef struct
{
    gatt_client_t** table;
    uint8           count;
}encoded_gatt_clients_t;
static encoded_gatt_clients_t encoded_gatt_clients = { 0 };

/* -----------------------------------------------------------------------------*/
/* Private functions                                                            */
/* -----------------------------------------------------------------------------*/
static inline bool gattClient_IsGattClientInitialised(void)
{
    return encoded_gatt_clients.table != NULL;
}

inline static gatt_client_t* gattClient_DeobfuscateClient(gatt_client_t* encoded_client)
{
    return (gatt_client_t *)(*(client_id_t*)encoded_client - GATT_CLIENT_ID_ENCODING);
}

static gatt_client_t* gattClient_GetClient(client_id_t client_id)
{
    /* This indicates a programming error (GattClient_Init must be called on startup) */
    PanicFalse(gattClient_IsGattClientInitialised());

    gatt_client_t *client = NULL;
    uint8 index = 0;
    for(; index < encoded_gatt_clients.count; index++)
    {
        if(*(client_id_t*)encoded_gatt_clients.table[index] == client_id)
        {
            client = gattClient_DeobfuscateClient(encoded_gatt_clients.table[index]);
            break;
        }
    }

    /* should be debug panic */
    PanicFalse(client != NULL);

    return client;
}

static inline void gattClient_SetClientStatus(gatt_client_t *client, gatt_client_status status)
{
    client->common_data.dynamic_data->status = status;
}

static inline void gattClient_SetClientConnectionID(gatt_client_t *client, uint16 cid)
{
    client->common_data.dynamic_data->cid = cid;
}

static void gattClient_ValidateClientConfiguration(gatt_client_t *client)
{
    PanicFalse(client->common_if.initialise);
    PanicFalse(client->common_if.deinitialise);
    PanicFalse(client->common_if.allocateMemory);
    PanicFalse(client->common_if.deallocateMemory);
    PanicFalse(client->common_if.getDiscoveryStopRequest);
    PanicFalse(client->common_if.getServiceForDiscovery);
    if(client->common_if.initialise == GattClient_Protected_ClientLibInit)
    {
        /* Must have client specific lib init if using the default initialiser */
        PanicFalse(client->client_specific_if.clientLibInit);
    }
    if(client->common_if.deinitialise == GattClient_Protected_ClientLibDeinit)
    {
        /* Must have client specific lib deinit if using the default deinitialiser */
        PanicFalse(client->client_specific_if.clientLibDeinit);
    }
    PanicFalse(client->common_data.task.handler);
}

/* -----------------------------------------------------------------------------*/
/* Protected overrideable functions (concrete gatt clients only)                */
/* -----------------------------------------------------------------------------*/
bool GattClient_Protected_AllocateMemory(gatt_client_t* instance)
{
    /* This indicates a programming error (GattClient_Init must be called on startup) */
    PanicZero(gattClient_IsGattClientInitialised());

    gatt_client_specific_data_t *client_specific_data = &instance->client_specific_data;
    *client_specific_data->lib_data_dynamic = calloc(client_specific_data->size_lib_data_dynamic, sizeof(char));

    return *client_specific_data->lib_data_dynamic != NULL;
}

void GattClient_Protected_DeallocateMemory(gatt_client_t* instance)
{
    /* This indicates a programming error (GattClient_Init must be called on startup) */
    PanicZero(gattClient_IsGattClientInitialised());

    gatt_client_specific_data_t *client_specific_data = &instance->client_specific_data;
    free(*client_specific_data->lib_data_dynamic);
    *client_specific_data->lib_data_dynamic = NULL;
}

gatt_client_service_t GattClient_Protected_GetServiceForDiscovery(gatt_client_t *instance)
{
    /* This indicates a programming error (GattClient_Init must be called on startup) */
    PanicZero(gattClient_IsGattClientInitialised());

    return instance->common_data.service_for_discovery;
}

bool GattClient_Protected_GetDiscoveryStopRequest(gatt_client_t* instance)
{
    /* This indicates a programming error (GattClient_Init must be called on startup) */
    PanicZero(gattClient_IsGattClientInitialised());

    return instance->common_data.stop_discovery_after_finding_service;
}

bool GattClient_Protected_ClientLibInit(gatt_client_t*instance, uint16 cid, uint16 start_handle, uint16 end_handle)
{
    /* This shouldn't be possible unless code is incorrectly modified */
    PanicZero(gattClient_IsGattClientInitialised());

    gatt_client_specific_data_t *client_specific_data = &instance->client_specific_data;
    return instance->client_specific_if.clientLibInit(
        (TaskData*)&instance->common_data.task,
        cid,
        start_handle,
        end_handle,
        *client_specific_data->lib_data_dynamic,
        client_specific_data->lib_data_const);
}

bool GattClient_Protected_ClientLibDeinit(gatt_client_t*instance)
{
    /* This shouldn't be possible unless code is incorrectly modified */
    PanicZero(gattClient_IsGattClientInitialised());

    gatt_client_specific_data_t *client_specific_data = &instance->client_specific_data;
    return instance->client_specific_if.clientLibDeinit(*client_specific_data->lib_data_dynamic);
}

/* -----------------------------------------------------------------------------*/
/* Protected non-overrideable functions (concrete gatt clients only)            */
/* -----------------------------------------------------------------------------*/
gatt_client_status GattClient_Protected_GetStatus(gatt_client_t*instance)
{
    return instance->common_data.dynamic_data->status;
}

uint16 GattClient_Protected_GetConnectionID(gatt_client_t*instance)
{
    return instance->common_data.dynamic_data->cid;
}

/* -----------------------------------------------------------------------------*/
/* Public API                                                                   */
/* -----------------------------------------------------------------------------*/
void GattClient_Init(const client_id_t *gatt_client_id[], uint8 num_elements)
{
    encoded_gatt_clients.table = (gatt_client_t **)gatt_client_id;
    encoded_gatt_clients.count = num_elements;    

    /* Set clients status */
    for(uint8 client_index = 0; client_index < encoded_gatt_clients.count; client_index++)
    {
        gatt_client_t *client = gattClient_DeobfuscateClient(encoded_gatt_clients.table[client_index]);
        gattClient_ValidateClientConfiguration(client);
        gattClient_SetClientStatus(client, gatt_client_status_invalid);
    }
}

void GattClient_Deinit(void)
{
    encoded_gatt_clients.table = NULL;
}

bool GattClient_AttachClient(client_id_t client_id, uint16 cid, uint16 start_handle, uint16 end_handle)
{
    bool status = FALSE;

    gatt_client_t *client = gattClient_GetClient(client_id);
    if(GattClient_Protected_GetStatus(client) != gatt_client_status_service_attached)
    {
        gattClient_SetClientConnectionID(client, cid);
        if(client->common_if.allocateMemory(client))
        {
            status = client->common_if.initialise(client, cid, start_handle, end_handle);
        }

        if(status)
        {
            gattClient_SetClientStatus(client, gatt_client_status_service_attached);
        }
    }
    return status;
}


bool GattClient_DetachClient(client_id_t client_id, uint16 cid)
{
    bool status = FALSE;

    gatt_client_t *client = gattClient_GetClient(client_id);
    uint16 client_cid = GattClient_Protected_GetConnectionID(client);
    gatt_client_status client_status = GattClient_Protected_GetStatus(client);
    if((client_cid == cid) && (client_status == gatt_client_status_service_attached))
    {
        status = client->common_if.deinitialise(client);
        if(status)
        {
            client->common_if.deallocateMemory(client);
            gattClient_SetClientStatus(client, gatt_client_status_service_detached);
            gattClient_SetClientConnectionID(client, 0);
        }
    }
    return status;
}

gatt_client_service_t GattClient_GetServiceForDiscovery(client_id_t client_id)
{
    gatt_client_t *client = gattClient_GetClient(client_id);
    return client->common_if.getServiceForDiscovery(client);
}

bool GattClient_GetDiscoveryStopRequest(client_id_t client_id)
{
    gatt_client_t *client = gattClient_GetClient(client_id);
    return client->common_if.getDiscoveryStopRequest(client);
}

