/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Implementation of the GATT Server module.
*/

#include "gatt_server_gatt.h"

#include "gatt_handler.h"
#include "gatt_handler_db_if.h"
#include "gatt_connect.h"
#include "bt_device.h"
#include "device_properties.h"

#include <logging.h>
#include <bdaddr.h>
#include <device_list.h>
#include <gatt.h>

#include <panic.h>
#include <stdlib.h>


gattServerGattData gatt_server_gatt = {0};


static void gattServerGatt_AddClient(uint16 cid);
static void gattServerGatt_RemoveClient(uint16 cid);


static const gatt_connect_observer_callback_t gatt_server_connect_observer_callback =
{
    .OnConnection = gattServerGatt_AddClient,
    .OnDisconnection = gattServerGatt_RemoveClient
};


static gattServerGattClientData *gattServerGatt_GetClientDataByCid(uint16 cid)
{
    uint16 client_count = 0;
    gattServerGattClientData *client_data = NULL;
    
    for (client_count = 0; client_count < NUMBER_GATT_CLIENTS; client_count++)
    {
        if (gatt_server_gatt.client_data[client_count].cid == cid)
        {
            client_data = &gatt_server_gatt.client_data[client_count];
        }
    }
    
    return client_data;
}

static bool gattServerGatt_ReadClientConfigFromStore(uint16 cid, uint16 *config)
{
    uint16 stored_client_config = 0;
    bdaddr public_addr;
    bool client_config_read = FALSE;
    
    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        client_config_read = appDeviceGetGattServerConfig(&public_addr, &stored_client_config);
        DEBUG_LOG("gattServerGatt_ReadClientConfigFromStore. Read persistent store, read=[%d] config=[0x%x]", client_config_read, stored_client_config);
        DEBUG_LOG("  cid=[0x%x] addr=[%x:%x:%x]", cid, public_addr.nap, public_addr.uap, public_addr.lap);
        
        if (client_config_read)
        {
            *config = stored_client_config;
        }
    }
    
    return client_config_read;
}

static void gattServerGatt_WriteClientConfigToStore(uint16 cid, uint16 client_config)
{
    bdaddr public_addr;

    UNUSED(client_config);
    
    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        DEBUG_LOG("gattServerGatt_WriteClientConfigToStore. Found public addr");
        DEBUG_LOG("  cid=[0x%x] addr=[%x:%x:%x]", cid, public_addr.nap, public_addr.uap, public_addr.lap);
        appDeviceSetGattServerConfig(&public_addr, client_config);
    }
    else
    {
        DEBUG_LOG("gattServerGatt_WriteClientConfigToStore. Not persistent data");
    }
}

static void gattServerGatt_ReadClientConfig(const GATT_SERVER_READ_CLIENT_CONFIG_IND_T * ind)
{
    uint16 client_config = 0;
    gattServerGattClientData *client_data = NULL;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerGatt_ReadClientConfig cid=[0x%x]", ind->cid);

    client_data = gattServerGatt_GetClientDataByCid(ind->cid);
    if (client_data != NULL)
    {
        client_config = client_data->config;
    }

    GattServerReadClientConfigResponse(GetGattServerGattGgatts(), ind->cid, ind->handle, client_config);
    DEBUG_LOG("  client_config=[0x%x]\n", client_config);
}

static void gattServerGatt_WriteClientConfig(const GATT_SERVER_WRITE_CLIENT_CONFIG_IND_T * ind)
{
    gattServerGattClientData *client_data = NULL;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerGatt_WriteClientConfig cid=[0x%x] value=[0x%x]\n",
        ind->cid, ind->config_value);

    client_data = gattServerGatt_GetClientDataByCid(ind->cid);
    if (client_data != NULL)
    {
        client_data->config = ind->config_value;
        
        /* Write client config to persistent store */
        gattServerGatt_WriteClientConfigToStore(ind->cid, client_data->config);
    }
}

static void gattServerGatt_ResetServicesChangedInStore(uint16 cid)
{
    bdaddr public_addr;

    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        DEBUG_LOG("gattServerGatt_ResetServicesChangedInStore. Found public addr");
        appDeviceSetGattServerServicesChanged(&public_addr, 0);
    }
    else
    {
        DEBUG_LOG("gattServerGatt_ResetServicesChangedInStore. Not persistent data");
    }
}

static bool gattServerGatt_ReadServicesChangedFromStore(uint16 cid, uint8 *value)
{
    uint8 stored_service_changed = 0;
    bdaddr public_addr;
    bool config_read = FALSE;
    
    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        config_read = appDeviceGetGattServerServicesChanged(&public_addr, &stored_service_changed);
        DEBUG_LOG("gattServerGatt_ReadServicesChangedFromStore. Read persistent store, read=[%d] value=[0x%x]", config_read, stored_service_changed);
        
        if (config_read)
        {
            *value = stored_service_changed;
        }
    }
    
    return config_read;
}

static void gattServerGatt_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gattServerGatt_MessageHandler id:%d 0x%x", id, id);

    switch (id)
    {
        case GATT_SERVER_READ_CLIENT_CONFIG_IND:
            gattServerGatt_ReadClientConfig((const GATT_SERVER_READ_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_SERVER_WRITE_CLIENT_CONFIG_IND:
            gattServerGatt_WriteClientConfig((const GATT_SERVER_WRITE_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_SERVER_SERVICE_CHANGED_INDICATION_CFM:
            break;

        default:
            DEBUG_LOG("gattServerGatt_MessageHandler. Unhandled message id:0x%x", id);
            break;
    }
}

static void gattServerGatt_init(void)
{
    gatt_server_gatt.gatt_task.handler = gattServerGatt_MessageHandler;
    
    if (GattServerInit(GetGattServerGattGgatts(),
                        GetGattServerGattTask(),
                        HANDLE_GATT_SERVICE, 
                        HANDLE_GATT_SERVICE_END) != gatt_server_status_success)
    {
        DEBUG_LOG("gattServerGatt_init Server failed");
        Panic();
    }
    
    GattConnect_RegisterObserver(&gatt_server_connect_observer_callback);
}

static void gattServerGatt_IndicateServicesChangedByCid(uint16 cid)
{
    uint8 stored_service_changed = 0;
    
    if (gattServerGatt_ReadServicesChangedFromStore(cid, &stored_service_changed))
    {
        if (stored_service_changed != 0)
        {
            DEBUG_LOG("gattServerGatt_IndicateServicesChangedByCid. cid=[0x%x]", cid);
            GattServerSendServiceChangedIndication(GetGattServerGattGgatts(), cid);

            gattServerGatt_ResetServicesChangedInStore(cid);
        }
    }
}

static void gattServerGatt_IndicateServicesChanged(void)
{
    uint16 client_count = 0;

    for (client_count = 0; client_count < NUMBER_GATT_CLIENTS; client_count++)
    {
        if (gatt_server_gatt.client_data[client_count].cid)
        {
            DEBUG_LOG("gattServerGatt_IndicateServicesChanged. cid=[0x%x]", 
                        gatt_server_gatt.client_data[client_count].cid);
            gattServerGatt_IndicateServicesChangedByCid(
                        gatt_server_gatt.client_data[client_count].cid);
        }
    }
}

static void gattServerGatt_AddClient(uint16 cid)
{
    uint16 stored_client_config = 0;
    gattServerGattClientData *client_data = NULL;
    
    PanicNull(GetGattServerGattTask());
    
    client_data = gattServerGatt_GetClientDataByCid(0);

    if (client_data != NULL)
    {
        client_data->cid = cid;
        /* Read client config from persistent store */
        if (gattServerGatt_ReadClientConfigFromStore(cid, &stored_client_config))
        {
            client_data->config = stored_client_config;
        }
        
        /* Inform of any service changes on connection */
        gattServerGatt_IndicateServicesChangedByCid(cid);
    }
}


static void gattServerGatt_RemoveClient(uint16 cid)
{
    gattServerGattClientData *client_data = NULL;
    
    PanicNull(GetGattServerGattTask());
    
    client_data = gattServerGatt_GetClientDataByCid(cid);
    
    if (client_data != NULL)
    {
        client_data->cid = 0;
        client_data->config = 0;
    }
}


/*****************************************************************************/
bool GattServerGatt_Init(Task init_task)
{
    UNUSED(init_task);

    gattServerGatt_init();
    
    DEBUG_LOG("GattServerGatt_Init. Server initialised");

    return TRUE;
}


void GattServerGatt_SetGattDbChanged(void)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    uint16 property_value = 0x2;
    bdaddr addr;

    DEBUG_LOG("GattServerGatt_SetGattDbChanged");
    
    DeviceList_GetAllDevicesWithPropertyValue(device_property_gatt_server_config, &property_value, sizeof(uint16), &devices, &num_devices);
    if (devices && num_devices)
    {
        DEBUG_LOG("GattServerGatt_SetGattDbChanged num_devices=[0x%x]", num_devices);
        for (int i=0; i< num_devices; i++)
        {
            addr = DeviceProperties_GetBdAddr(devices[i]);
            DEBUG_LOG("GattServerGatt_SetGattDbChanged addr=[%x:%x:%x]", addr.uap, addr.lap, addr.nap);
            appDeviceSetGattServerServicesChanged(&addr, 1);
        }
        
        /* Indicate changes to any connected clients */
        gattServerGatt_IndicateServicesChanged();
    }
    free(devices);
    devices = NULL;
}
