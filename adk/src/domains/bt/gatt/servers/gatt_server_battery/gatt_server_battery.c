/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       gatt_server_battery.c
\brief      Implementation of the GATT Battery Server module.
*/

#ifdef INCLUDE_GATT_BATTERY_SERVER

#include "gatt_server_battery.h"

#include "multidevice.h"

#include "gatt_handler.h"
#include "gatt_connect.h"
#include "gatt_handler_db_if.h"

#include <logging.h>
#include <bdaddr.h>
#include <gatt.h>
#include <state_proxy.h>

#include <panic.h>


#define NAMESPACE_BLUETOOTH_SIG             0x01        /*!< Bluetooth SIG Namespace */
        /* Values taken from GATT Namespace Descriptors, in Assigned Numbers */
#define DESCRIPTION_BATTERY_UNKNOWN         0x0000      /*!< Bluetooth SIG description "unknown" */
#define DESCRIPTION_BATTERY_LEFT            0x010D      /*!< Bluetooth SIG description "left" */
#define DESCRIPTION_BATTERY_RIGHT           0x010E      /*!< Bluetooth SIG description "right" */

#define GATT_SERVER_BATTERY_INTERNAL_MESSAGE_RESEND_NOTIFICATION    0

#define BATTERY_NOTIFICATION_INITIAL_DELAY_S                        2
#define BATTERY_NOTIFICATION_UPDATE_DELAY_S                         30

#define BATTERY_LEVEL_UNSET                 0xFFFF

#define NUMBER_OF_ADVERT_DATA_ITEMS         1
#define SIZE_BATTERY_ADVERT                 4

#define HANDLE_BATTERY_SERVICELEFT          HANDLE_BATTERY_SERVICE_N(1)
#define HANDLE_BATTERY_SERVICERIGHT         HANDLE_BATTERY_SERVICE_N(2)
#define HANDLE_BATTERY_SERVICELEFT_END      HANDLE_BATTERY_SERVICE_END_N(1)
#define HANDLE_BATTERY_SERVICERIGHT_END     HANDLE_BATTERY_SERVICE_END_N(2)

gattServerBatteryData gatt_server_battery = {0};

static void gattServerBattery_AddClient(uint16 cid);
static void gattServerBattery_RemoveClient(uint16 cid);
static unsigned int gattServerBattery_NumberOfAdvItems(const le_adv_data_params_t * params);
static le_adv_data_item_t gattServerBattery_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id);
static void gattServerBattery_ReleaseAdvDataItems(const le_adv_data_params_t * params);

static const gatt_connect_observer_callback_t gatt_battery_connect_observer_callback =
{
    .OnConnection = gattServerBattery_AddClient,
    .OnDisconnection = gattServerBattery_RemoveClient
};

static const le_adv_data_callback_t gatt_battery_le_advert_callback =
{
    .GetNumberOfItems = gattServerBattery_NumberOfAdvItems,
    .GetItem = gattServerBattery_GetAdvDataItems,
    .ReleaseItems = gattServerBattery_ReleaseAdvDataItems
};

static const uint8 gatt_battery_advert_data[SIZE_BATTERY_ADVERT] = { \
    SIZE_BATTERY_ADVERT - 1, \
    ble_ad_type_complete_uuid16, \
    GATT_SERVICE_UUID_BATTERY_SERVICE & 0xFF, \
    GATT_SERVICE_UUID_BATTERY_SERVICE >> 8 \
};

static le_adv_data_item_t gatt_battery_advert;

static uint16 gattServerBattery_GetBatteryLevel(const GBASS *battery_server)
{
    uint16 battery_level_left;
    uint16 battery_level_right;
    uint16 battery_percent = 0;

    if (Multidevice_IsLeft())
    {
        StateProxy_GetLocalAndRemoteBatteryLevels(&battery_level_left,&battery_level_right);
    }
    else
    {
        StateProxy_GetLocalAndRemoteBatteryLevels(&battery_level_right,&battery_level_left);
    }

    if (battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_left))
    {
        if (battery_level_left)
        {
            battery_percent = appBatteryConvertLevelToPercentage(battery_level_left);
        }
        else
        {
            battery_percent = 0;
        }
    }
    else if (battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_right))
    {
        if (battery_level_right)
        {
            battery_percent = appBatteryConvertLevelToPercentage(battery_level_right);
        }
        else
        {
            battery_percent = 0;
        }
    }
    else
    {
        DEBUG_LOG("gattServerBattery_GetBatteryLevel. Invalid Battery Server");
        Panic();
    }

    DEBUG_LOG("gattServerBattery_GetBatteryLevel bas=[0x%p] level=[%u]\n", (void *)battery_server, battery_percent);
    
    return battery_percent;
}

static bool gattServerBattery_SendBatteryLevelIfChanged(const gattServerBatteryInstanceInfo *instance)
{
    uint16 client_count = 0;
    uint16 number_cids = 0;
    uint16 cids[NUMBER_BATTERY_CLIENTS];
    const GBASS *battery_server = &instance->gbass;
    uint16 battery_percent = gattServerBattery_GetBatteryLevel(battery_server);
    bool battery_level_sent = FALSE;

    for (client_count = 0; client_count < NUMBER_BATTERY_CLIENTS; client_count++)
    {
        if (instance->client_data[client_count].config & 0x1)
        {
            if (instance->client_data[client_count].sent_battery_level != battery_percent)
            {
                cids[number_cids++] = instance->client_data[client_count].cid;
                instance->client_data[client_count].sent_battery_level = battery_percent;
            }
        }
    }
    if (number_cids)
    {
        battery_level_sent = GattBatteryServerSendLevelNotification(battery_server, number_cids, cids, battery_percent);
        DEBUG_LOG("gattServerBattery_SendBatteryLevelIfChanged sent=[%u] bas=[0x%p] percent=[%u] number_cids={%u]", battery_level_sent, battery_server, battery_percent, number_cids);
    }

    return battery_level_sent;
}

static void gattServerBattery_EndBatteryNotifications(void)
{
    MessageCancelFirst(GetGattServerBatteryTask(), GATT_SERVER_BATTERY_INTERNAL_MESSAGE_RESEND_NOTIFICATION);
}

static void gattServerBattery_SendBatteryNotificationsAfterDelay(uint16 delay_s)
{
    DEBUG_LOG("gattServerBattery_SendBatteryNotificationsAfterDelay. Resend after delay");
    MessageSendLater(GetGattServerBatteryTask(), GATT_SERVER_BATTERY_INTERNAL_MESSAGE_RESEND_NOTIFICATION, 0, D_SEC(delay_s));
}

static void gattServerBattery_ResendBatteryNotificationsIfChanged(void)
{
    uint16 battery_type;
    bool send_battery_level = FALSE;
    bool battery_notified = FALSE;
    
    gattServerBattery_EndBatteryNotifications();
    
    DEBUG_LOG("gattServerBattery_ResendBatteryNotificationsIfChanged");
    
    for (battery_type = 0; battery_type < NUMBER_BATTERY_SERVERS; battery_type++)
    {
        send_battery_level = gattServerBattery_SendBatteryLevelIfChanged(GetGattServerBatteryInstance(battery_type));
        if (send_battery_level)
        {
            battery_notified = TRUE;
        }
    }
    
    if (battery_notified)
    {
        gattServerBattery_SendBatteryNotificationsAfterDelay(BATTERY_NOTIFICATION_UPDATE_DELAY_S);
    }
}

static void gattServerBattery_ReadLevelInd(const GATT_BATTERY_SERVER_READ_LEVEL_IND_T * ind)
{
    uint16 battery_percent = gattServerBattery_GetBatteryLevel(ind->battery_server);
    
    DEBUG_LOG("gattServerBattery_ReadLevelInd bas=[0x%p] cid=[0x%x] level=[%u]\n", 
        (void *)ind->battery_server, ind->cid, battery_percent);

    /* Return requested battery level */
    GattBatteryServerReadLevelResponse(ind->battery_server, ind->cid, battery_percent);
}

static gattServerBatteryClientData *gattServerBattery_GetClientDataByCid(const gattServerBatteryInstanceInfo *instance, uint16 cid)
{
    uint16 client_count = 0;
    gattServerBatteryClientData *client_data = NULL;
    
    for (client_count = 0; client_count < NUMBER_BATTERY_CLIENTS; client_count++)
    {
        if (instance->client_data[client_count].cid == cid)
        {
            client_data = &instance->client_data[client_count];
        }
    }
    
    return client_data;
}

static bool gattServerBattery_ReadClientConfigFromStore(uint16 cid, uint16 battery_type, uint16 *config)
{
    uint16 stored_client_config = 0;
    bdaddr public_addr;
    bool client_config_read = FALSE;
    
    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        if (battery_type == gatt_server_battery_type_left)
        {
            client_config_read = appDeviceGetBatterServerConfigLeft(&public_addr, &stored_client_config);
            DEBUG_LOG("gattServerBattery_ReadClientConfigFromStore. Read persistent left battery, read=[%d] config=[0x%x]", client_config_read, stored_client_config);
        }
        else if (battery_type == gatt_server_battery_type_right)
        {
            client_config_read = appDeviceGetBatterServerConfigRight(&public_addr, &stored_client_config);
            DEBUG_LOG("gattServerBattery_ReadClientConfigFromStore. Read persistent right battery, read=[%d] config=[0x%x]", client_config_read, stored_client_config);
        }
        else
        {
            DEBUG_LOG("gattServerBattery_ReadClientConfigFromStore. Invalid Battery Type");
            Panic();
        }
        if (client_config_read)
        {
            *config = stored_client_config;
        }
    }
    
    return client_config_read;
}

static void gattServerBattery_WriteClientConfigToStore(uint16 cid, uint16 battery_type, uint16 client_config)
{
    bdaddr public_addr;
    
    if (appGattGetPublicAddrFromCid(cid, &public_addr))
    {
        /* Write client config to persistent store */
        if (battery_type == gatt_server_battery_type_left)
        {
            DEBUG_LOG("gattServerBattery_WriteClientConfigToStore. Persistent left battery");
            appDeviceSetBatterServerConfigLeft(&public_addr, client_config);
        }
        else if (battery_type == gatt_server_battery_type_right)
        {
            DEBUG_LOG("gattServerBattery_WriteClientConfigToStore. Persistent right battery");
            appDeviceSetBatterServerConfigRight(&public_addr, client_config);
        }
        else
        {
            DEBUG_LOG("gattServerBattery_WriteClientConfigToStore. Invalid Battery Type");
            Panic();
        }
    }
    else
    {
        DEBUG_LOG("gattServerBattery_WriteClientConfigToStore. Not persistent data");
    }
}

static void gattServerBattery_ReadPresentationInd(const GATT_BATTERY_SERVER_READ_PRESENTATION_IND_T * ind)
{
    uint16 description = DESCRIPTION_BATTERY_UNKNOWN;
    
    if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_left))
    {
        description = DESCRIPTION_BATTERY_LEFT;
    }
    else if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_right))
    {
        description = DESCRIPTION_BATTERY_RIGHT;
    }
    else
    {
        DEBUG_LOG("gattServerBattery_ReadPresentationInd. Invalid Battery Server");
        Panic();
    }

    DEBUG_LOG("gattServerBattery_ReadPresentationInd bas=[0x%p] cid=[0x%x]\n",
                    ind->battery_server,ind->cid);

    GattBatteryServerReadPresentationResponse(ind->battery_server,
                                              ind->cid,
                                              NAMESPACE_BLUETOOTH_SIG,
                                              description);

    DEBUG_LOG("   Return: desc=[0x%x]\n", description);
}

static void gattServerBattery_ReadClientConfig(const GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND_T * ind)
{
    uint16 client_config = 0;
    gattServerBatteryInstanceInfo *instance = NULL;
    gattServerBatteryClientData *client_data = NULL;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerBattery_ReadClientConfig bas=[0x%p] cid=[0x%x]\n", ind->battery_server, ind->cid);

    if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_left))
    {
        instance = GetGattServerBatteryInstance(gatt_server_battery_type_left);
    }
    else if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_right))
    {
        instance = GetGattServerBatteryInstance(gatt_server_battery_type_right);
    }
    else
    {
        DEBUG_LOG("gattServerBattery_ReadClientConfig. Invalid Battery Server");
        Panic();
    }
    
    client_data = gattServerBattery_GetClientDataByCid(instance, ind->cid);
    if (client_data != NULL)
    {
        client_config = client_data->config;
    }

    GattBatteryServerReadClientConfigResponse(ind->battery_server, ind->cid, client_config);
    DEBUG_LOG("  client_config=[0x%x]\n", client_config);
}

static void gattServerBattery_WriteClientConfig(const GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND_T * ind)
{
    gattServerBatteryInstanceInfo *instance = NULL;
    gattServerBatteryClientData *client_data = NULL;
    uint16 battery_type = gatt_server_battery_type_left;

    /* Return the current value of the client configuration descriptor for the device */
    DEBUG_LOG("gattServerBattery_WriteClientConfig bas=[0x%p] cid=[0x%x] value=[0x%x]\n",
        ind->battery_server, ind->cid, ind->config_value);

    if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_left))
    {
        battery_type = gatt_server_battery_type_left;
        instance = GetGattServerBatteryInstance(battery_type);
    }
    else if (ind->battery_server == GetGattServerBatteryGbass(gatt_server_battery_type_right))
    {
        battery_type = gatt_server_battery_type_right;
        instance = GetGattServerBatteryInstance(battery_type);
    }
    else
    {
        DEBUG_LOG("gattServerBattery_WriteClientConfig. Invalid Battery Server");
        Panic();
    }
    
    client_data = gattServerBattery_GetClientDataByCid(instance, ind->cid);
    if (client_data != NULL)
    {
        client_data->config = ind->config_value;
        
        /* Write client config to persistent store */
        gattServerBattery_WriteClientConfigToStore(ind->cid, battery_type, client_data->config);
        
        gattServerBattery_EndBatteryNotifications();
        gattServerBattery_SendBatteryNotificationsAfterDelay(BATTERY_NOTIFICATION_INITIAL_DELAY_S);
    }
}

static void gattServerBattery_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gattServerBattery_MessageHandler id:%d 0x%x", id, id);

    switch (id)
    {
        case GATT_BATTERY_SERVER_READ_LEVEL_IND:
            gattServerBattery_ReadLevelInd((const GATT_BATTERY_SERVER_READ_LEVEL_IND_T *) message);
            break;

        case GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND:
            gattServerBattery_ReadClientConfig((const GATT_BATTERY_SERVER_READ_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND:
            gattServerBattery_WriteClientConfig((const GATT_BATTERY_SERVER_WRITE_CLIENT_CONFIG_IND_T *) message);
            break;

        case GATT_BATTERY_SERVER_READ_PRESENTATION_IND:
            gattServerBattery_ReadPresentationInd((const GATT_BATTERY_SERVER_READ_PRESENTATION_IND_T *)message);
            break;
            
        case GATT_SERVER_BATTERY_INTERNAL_MESSAGE_RESEND_NOTIFICATION:
            gattServerBattery_ResendBatteryNotificationsIfChanged();
            break;

        default:
            DEBUG_LOG("appGattBatteryMessageHandler. Unhandled message id:0x%x", id);
            break;
    }
}

static void gattServerBattery_SetupAdvertising(void)
{
    gatt_battery_advert.size = SIZE_BATTERY_ADVERT;
    gatt_battery_advert.data = gatt_battery_advert_data;

    LeAdvertisingManager_Register(NULL, &gatt_battery_le_advert_callback);
}


static void gattServerBattery_init(void)
{
    int index = 0;
    uint16 handle_start = 0;
    uint16 handle_end = 0;
    gatt_battery_server_init_params_t battery_server_params = {.enable_notifications = TRUE};
    
    gatt_server_battery.gatt_battery_task.handler = gattServerBattery_MessageHandler;
    
    for (index = 0; index < NUMBER_BATTERY_SERVERS; index++)
    {
        switch (index)
        {
            case gatt_server_battery_type_left:
                handle_start = HANDLE_BATTERY_SERVICELEFT;
                handle_end = HANDLE_BATTERY_SERVICELEFT_END;
                break;
            case gatt_server_battery_type_right:
                handle_start = HANDLE_BATTERY_SERVICERIGHT;
                handle_end = HANDLE_BATTERY_SERVICERIGHT_END;
                break;
            default:
                DEBUG_LOG("gattServerBattery_init. No handles for server(%d)", index);
                Panic();
                break;
        }
            
        if (!GattBatteryServerInit(GetGattServerBatteryGbass(index),
                                    GetGattServerBatteryTask(), &battery_server_params,
                                    handle_start, handle_end))
        {
            DEBUG_LOG("gattServerBattery_init Server (%d) failed", index);
            Panic();
        }
    }
    
    GattConnect_RegisterObserver(&gatt_battery_connect_observer_callback);
    
    gattServerBattery_SetupAdvertising();
}

static void gattServerBattery_AddClient(uint16 cid)
{
    uint16 battery_type;
    uint16 stored_client_config = 0;
    gattServerBatteryInstanceInfo *instance = NULL;
    gattServerBatteryClientData *client_data = NULL;
    
    if (GetGattServerBatteryTask())
    {
        for (battery_type = 0; battery_type < NUMBER_BATTERY_SERVERS; battery_type++)
        {
            instance = GetGattServerBatteryInstance(battery_type);
            client_data = gattServerBattery_GetClientDataByCid(instance, 0);

            if (client_data != NULL)
            {
                client_data->cid = cid;
                client_data->sent_battery_level = BATTERY_LEVEL_UNSET;
                /* Read client config from persistent store */
                if (gattServerBattery_ReadClientConfigFromStore(cid, battery_type, &stored_client_config))
                {
                    client_data->config = stored_client_config;
                }
            }
        }
        
        gattServerBattery_ResendBatteryNotificationsIfChanged();
    }
}


static void gattServerBattery_RemoveClient(uint16 cid)
{
    uint16 battery_type;
    gattServerBatteryInstanceInfo *instance = NULL;
    gattServerBatteryClientData *client_data = NULL;
    
    if (GetGattServerBatteryTask())
    {
        for (battery_type = 0; battery_type < NUMBER_BATTERY_SERVERS; battery_type++)
        {
            instance = GetGattServerBatteryInstance(battery_type);
            client_data = gattServerBattery_GetClientDataByCid(instance, cid);
            
            if (client_data != NULL)
            {
                client_data->cid = 0;
                client_data->config = 0;
                client_data->sent_battery_level = BATTERY_LEVEL_UNSET;
            }
        }
        
        gattServerBattery_ResendBatteryNotificationsIfChanged();
    }
}

static bool gattBatteryServer_CanAdvertiseService(const le_adv_data_params_t * params)
{
    if(params->data_set != le_adv_data_set_handset_identifiable)
    {
        return FALSE;
    }
    
    if(params->completeness != le_adv_data_completeness_full)
    {
        return FALSE;
    }
    
    if(params->placement != le_adv_data_placement_advert)
    {
        return FALSE;
    }
    
    return TRUE;
}

static unsigned int gattServerBattery_NumberOfAdvItems(const le_adv_data_params_t * params)
{
    if(gattBatteryServer_CanAdvertiseService(params))
    {
        return NUMBER_OF_ADVERT_DATA_ITEMS;
    }
    
    return 0;
}

static le_adv_data_item_t gattServerBattery_GetAdvDataItems(const le_adv_data_params_t * params, unsigned int id)
{
    UNUSED(id);

    if(gattBatteryServer_CanAdvertiseService(params))
    {
        return gatt_battery_advert;
    }
    
    Panic();
    return gatt_battery_advert;
}

static void gattServerBattery_ReleaseAdvDataItems(const le_adv_data_params_t * params)
{
    UNUSED(params);

    return;
}

/*****************************************************************************/
bool GattServerBattery_Init(Task init_task)
{
    UNUSED(init_task);

    gattServerBattery_init();
    
    DEBUG_LOG("GattServerBattery_Init. Server initialised");

    return TRUE;
}

#endif /* INCLUDE_GATT_BATTERY_SERVER */
