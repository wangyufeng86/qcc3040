/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the Device Test Service

\implementation

The normal flow of control is shown below

@startuml

participant "Application" as app
participant "Device Test Service" as dts
participant "Connection Library" as conn
participant "Pairing" as pairing
participant "SPP Server" as spps

app -> dts : DeviceTestService_Start()
dts -> spps : SppStartService
dts <- spps : SPPS_START_SERVICE_CFM\nsuccess
dts -> pairing : Pairing_Pair()
note right: When pairing known devices can also connect to us
alt
dts <- pairing : PAIRING_CFM\nsuccess
note right : Should then proceed with connection
else pairing failed
dts <- pairing : PAIRING_CFM\nAuthenticationFailed, NoLinkKey, Timeout, Unknown, Failed
dts -> pairing : Pairing_Pair()
else connection
dts <- pairing  : PAIRING_CFM\nstopped
note right : Should then proceed with connection
end

dts <- spps : SPP_CONNECT_IND
dts -> spps : SppConnectResponse()

dts <- spps : SPP_SERVER_CONNECT_CFM\nsuccess

loop until session closed
dts <- spps : SPPS_MESSAGE_MORE_DATA
app <- dts  : mapped function
app -> dts  : function response/OK
note right : response sent to the Sink for the session
end

alt normal disconnect (no reboot)
dts <- spps : SPPS_MESSAGE_MORE_DATA
app <- dts  : DeviceTestServiceCommand_HandleDtsEndTesting
app -> dts  : OK
dts -> spps : SppDisconnectRequest()
dts <- spps : SPPS_DISCONNECT_CFM
dts <- conn : CON_MANAGER_TP_DISCONNECT_IND
app <- dts  : DEVICE_TEST_SERVICE_ENDED

else connection drop
group spp
    dts <- spps : SPPS_DISCONNECT_IND
    dts -> spps : SppStartService
end
group link
    dts <- conn : CON_MANAGER_TP_DISCONNECT_IND
    dts -> pairing : Pairing_Pair()
    note right : already paired devices can reconnect at this point
end

else reset after test
dts <- spps : SPPS_MESSAGE_MORE_DATA
app <- dts  : DeviceTestServiceCommand_HandleDtsEndTesting
app -> dts  : OK
note right : Device reboots
end

@enduml

Use of device database

The device test service cleans up after itself if possible. The details 
of the devices used for driving the device test service should be
removed from the device database.

This is done by adding a property, device_property_device_test_service,
to an entry in the device database when we pair with the device.

*/

//#define DEVELOPMENT_DEBUG

#include "device_test_service.h"
#include "device_test_service_config.h"
#include "device_test_service_data.h"
#include "device_test_service_messages.h"
#include "device_test_service_test.h"
#include "device_test_parse.h"

#include <stdlib.h>

#include <connection_manager.h>
#include <logging.h>
#include <panic.h>
#include <sdp_parse.h>
#include <spps.h>
#include <pairing.h>
#include <bt_device.h>
#include <ps.h>
#include <power_manager.h>
#include <bdaddr.h>
#include <sdp.h>
#include <device_properties.h>
#include <device_list.h>
#include <phy_state.h>

#ifdef INCLUDE_DEVICE_TEST_SERVICE

#ifdef DEVELOPMENT_DEBUG
#include <ctype.h>
#include <vmtypes.h>
#endif


static void deviceTestService_MessageHandler(Task task, MessageId id, Message message);
static void deviceTestService_SppsMessageHandler(Task task, MessageId id, Message message);

device_test_service_data_t device_test_service
                    = { .device_test_service_handler_task = deviceTestService_MessageHandler,
                        .spps_handler_task = deviceTestService_SppsMessageHandler,
                      };


/*! Make the device connectable and remember previous setting */
static void deviceTestService_SaveAndSetConnectableState(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    dts->bredr_connections_allowed = ConManagerIsConnectionAllowed(cm_transport_bredr);
    dts->handset_connections_allowed = ConManagerIsHandsetConnectAllowed();
    
    ConManagerAllowConnection(cm_transport_bredr, TRUE);
    ConManagerAllowHandsetConnect(TRUE);
}


/*! Restore the device to the connectable state before service started */
static void deviceTestService_RestoreConnectableState(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    ConManagerAllowConnection(cm_transport_bredr, dts->bredr_connections_allowed);
    ConManagerAllowHandsetConnect(dts->handset_connections_allowed);
}


static void deviceTestService_TidyAfterConnectionLoss(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    dts->spp = NULL;

    if (dts->authenticated != device_test_service_authentication_not_required)
    {
        dts->authenticated = device_test_service_not_authenticated;
    }
}


static void deviceTestService_SetConnectedAddress(const bdaddr *address)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    if (BdaddrIsSame(&dts->connected_address, address))
    {
        return;
    }
    if (!BdaddrIsZero(&dts->connected_address))
    {
        DEBUG_LOG_WARN("deviceTestService_SetConnectedAddress replacing address. Old:x%06lx New:x%06lx",
                        dts->connected_address.lap, address->lap);
    }
    dts->connected_address = *address;

    /* We only deal with connection drops when we know the connected device */
    ConManagerRegisterTpConnectionsObserver(cm_transport_bredr, 
                                            DeviceTestServiceGetTask());
}


static void deviceTestService_ClearConnectedAddress(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    BdaddrSetZero(&dts->connected_address);

    /* We only deal with connection drops when we know the connected device */
    ConManagerUnregisterTpConnectionsObserver(cm_transport_bredr, 
                                              DeviceTestServiceGetTask());
}


static void deviceTestService_MarkDeviceAsDts(const bdaddr *address)
{
    device_t device = BtDevice_GetDeviceForBdAddr(address);
    uint8 dts_property = TRUE;

    if (!device)
    {
        DEBUG_LOG_ERROR("deviceTestService_MarkDeviceAsDts. Device not yet created x%06lx", 
                                address->lap);
        Panic();
    }

    Device_SetProperty(device, device_property_device_test_service, 
                       (void *)&dts_property, sizeof(dts_property));
}


unsigned deviceTestService_GetDtsDevices(device_t **devices)
{
    uint8 dts_device = TRUE;
    device_t* local_devices = NULL;
    unsigned num_devices = 0;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_device_test_service, 
                                        (void*)&dts_device, sizeof(dts_device), 
                                        &local_devices, &num_devices);

    if (devices)
    {
        *devices = local_devices;
    }
    else if (local_devices)
    {
        free(local_devices);
    }
    return num_devices;
}

static void deviceTestService_DeleteAllDtsDevicesIfNeeded(void)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;

    if (!DeviceTestService_TestMode())
    {
        num_devices = deviceTestService_GetDtsDevices(&devices);

        DEBUG_LOG_FN_ENTRY("deviceTestService_DeleteAllDtsDevices num_devices:%d",
                           num_devices);

        if (devices && num_devices)
        {
            for (unsigned i=0; i< num_devices; i++)
            {
                bdaddr bd_addr = DeviceProperties_GetBdAddr(devices[i]);

                appDeviceDelete(&bd_addr);
            }
            free(devices);
        }
    }
}


static bool deviceTestService_ContinueDisconnect(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_ContinueDisconnect. Disconnecting:%d Active:%d Auth:%d Registered:%d SPP:%d",
                    dts->disconnecting, dts->active, dts->authenticated,
                    dts->spp_service_registered, !!dts->spp);

    if (!dts->disconnecting)
    {
        DEBUG_LOG_VERBOSE("deviceTestService_ContinueDisconnect. Not disconnecting !");
        return FALSE;
    }

    if (dts->spp_service_registered)
    {
        DEBUG_LOG_DEBUG("deviceTestService_ContinueDisconnect. Remove SPP registration (stop connection)");
        SppStopService(DeviceTestServiceGetTask());
        return TRUE;
    }

    if (dts->spp)
    {
        DEBUG_LOG_DEBUG("deviceTestService_ContinueDisconnect. Disconnect SPPS");
        SppDisconnectRequest(dts->spp);
        return TRUE;
    }

    if (!BdaddrIsZero(&dts->connected_address))
    {
        if (ConManagerIsConnected(&dts->connected_address))
        {
            DEBUG_LOG_DEBUG("deviceTestService_ContinueDisconnect. ACL connected. Waiting.");
            return TRUE;
        }
        deviceTestService_ClearConnectedAddress();
    }

    dts->active = FALSE;
    dts->disconnecting = FALSE;
    if (dts->app_task)
    {
        DEBUG_LOG_WARN("deviceTestService_ContinueDisconnect. Fully disconnected. Inform app.");

        MessageSend(dts->app_task, DEVICE_TEST_SERVICE_ENDED, NULL);

        dts->app_task = NULL;
    }
    else
    {
        DEBUG_LOG_WARN("deviceTestService_ContinueDisconnect. Fully disconnected. Nobody to notify.");
    }

    /* Now we are disconnected we can remove any devices that were added 
       to the device database */
    deviceTestService_DeleteAllDtsDevicesIfNeeded();

    return TRUE;
}


static void deviceTestService_Disconnect(void)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_Disconnect. Disconnecting:%d Active:%d Auth:%d Registered:%d SPP:%d",
                    dts->disconnecting, dts->active, dts->authenticated,
                    dts->spp_service_registered, !!dts->spp);

    dts->disconnecting = TRUE;

    deviceTestService_RestoreConnectableState();

    deviceTestService_ContinueDisconnect();
}


static void deviceTestService_HandlePairingPairCfm(const PAIRING_PAIR_CFM_T *pair_cfm)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandlePairingPairCfm Sts:%d", pair_cfm->status);

    switch (pair_cfm->status)
    {
        case pairingSuccess:
            /* We have no information about the device, so assume it is TWS Standard.
               Without this, a subsequent connection will not work (connection
               manager blocks) */
            deviceTestService_SetConnectedAddress(&pair_cfm->device_bd_addr);
            dts->pairing_requested = FALSE;

            deviceTestService_MarkDeviceAsDts(&pair_cfm->device_bd_addr);
            break;

        case pairingAuthenticationFailed:
        case pairingtNoLinkKey:
        case pairingTimeout:
        case pairingUnknown:
        case pairingFailed:
            DEBUG_LOG_WARN("deviceTestService_HandlePairingPairCfm Failure status, retry");

            if (dts->spp_service_registered)
            {
                Pairing_Pair(DeviceTestServiceGetTask(), FALSE);
            }
            else
            {
                Panic();
            }
            break;

        case pairingStopped:
            /* We "stop" if we connect to a device. Remember the device we connected to. */
            DEBUG_LOG_DEBUG("deviceTestService_HandlePairingPairCfm Stopped. %06lx", 
                            pair_cfm->device_bd_addr.lap);
            deviceTestService_SetConnectedAddress(&pair_cfm->device_bd_addr);

            dts->pairing_requested = FALSE;
            break;

        /* Activity statuses that can be ignored */
        case pairingInProgress:
        case pairingNotInProgress:
        case pairingCompleteVersionChanged:
        case pairingLinkKeyReceived:
            break;

        default:
            DEBUG_LOG_WARN("deviceTestService_HandlePairingPairCfm Failure UNEXPECTED STATUS %d (x%x)",
                           pair_cfm->status, pair_cfm->status);
            break;
    }
}


static void deviceTestService_HandleInternalReboot(void)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleInternalReboot");

    appPowerReboot();
}


static void deviceTestService_HandleInternalEndTesting(void)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleInternalEndTesting");

    deviceTestService_Disconnect();
}


static void deviceTestService_HandleInternalCleanup(void)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleInternalCleanup");

    deviceTestService_DeleteAllDtsDevicesIfNeeded();
}


static void deviceTestService_HandleConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T *disc)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleConManagerDisconnectInd %06lx",
                disc->tpaddr.taddr.addr.lap);

    DEBUG_LOG_VERBOSE("\t Connected: %x:%x:%x",dts->connected_address.uap,dts->connected_address.nap,dts->connected_address.lap);
    DEBUG_LOG_VERBOSE("\tDisconnect: %x:%x:%x",disc->tpaddr.taddr.addr.uap,disc->tpaddr.taddr.addr.nap,disc->tpaddr.taddr.addr.lap);

    if (BdaddrIsSame(&disc->tpaddr.taddr.addr, &dts->connected_address))
    {
        DEBUG_LOG_INFO("deviceTestService_HandleConManagerDisconnectInd %06lx DISCONNECTED",
                    disc->tpaddr.taddr.addr.lap);

        deviceTestService_ClearConnectedAddress();

        deviceTestService_TidyAfterConnectionLoss();

        /*! See if the disconnection was part of a disconnect and if not
            make sure we are still connectable (i.e. pairing) */
        if (!deviceTestService_ContinueDisconnect())
        {
            if (!dts->pairing_requested)
            {
                Pairing_Pair(DeviceTestServiceGetTask(), FALSE);
                dts->pairing_requested = TRUE;
                return;
            }
        }
    }
    else
    {
        DEBUG_LOG_VERBOSE("deviceTestService_HandleConManagerDisconnectInd %06lx (not me)",
                    disc->tpaddr.taddr.addr.lap);
    }
}


static void deviceTestService_HandlePhyState(PHY_STATE_CHANGED_IND_T *change)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandlePhyState PhyState:%d",
                                                    change->new_state);

    if (   change->new_state == PHY_STATE_IN_CASE
        && DeviceTestService_TestMode())
    {
        MessageSendLater(DeviceTestServiceGetTask(), 
                         DEVICE_TEST_SERVICE_INTERNAL_REBOOT, NULL,
                         DeviceTestService_InCaseRebootTimeOutMs());
    }
}


static void deviceTestService_MessageHandler(Task task, MessageId id, Message message)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("deviceTestService_MessageHandler Active:%d x%x (%d) %d",
                                        dts->active, id, id, id&0xFF);

    /* These message can be received when test mode is inactive/disabled */
    switch (id)
    {
        case DEVICE_TEST_SERVICE_INTERNAL_REBOOT:
            deviceTestService_HandleInternalReboot();
            break;
            
        case DEVICE_TEST_SERVICE_INTERNAL_CLEAN_UP:
            deviceTestService_HandleInternalCleanup();
            break;

        case PHY_STATE_CHANGED_IND:
            deviceTestService_HandlePhyState((PHY_STATE_CHANGED_IND_T *)message);
            break;

        default:
            break;
    }

    if (!dts->active)
    {
        return;
    }

    switch (id)
    {
        case PAIRING_PAIR_CFM:
            deviceTestService_HandlePairingPairCfm((const PAIRING_PAIR_CFM_T *)message);
            break;

        case DEVICE_TEST_SERVICE_INTERNAL_END_TESTING:
            deviceTestService_HandleInternalEndTesting();
            break;

        case CON_MANAGER_TP_CONNECT_IND:
            /* We monitor disconnections only */
            break;

        case CON_MANAGER_TP_DISCONNECT_IND:
            deviceTestService_HandleConManagerDisconnectInd((const CON_MANAGER_TP_DISCONNECT_IND_T *)message);
            break;

        default:
#ifdef DEVELOPMENT_DEBUG
            DEBUG_LOG_DEBUG("deviceTestService_MessageHandler Unhandled x%x (%d) %d",
                                                id, id, id&0xFF);
#endif
            break;
    }
}


static void deviceTestService_HandleSppsStartCfm(const SPP_START_SERVICE_CFM_T *spps_start)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsStartCfm  sts:%d", spps_start->status);

    if (spps_start->status == spp_start_success)
    {
        Pairing_Pair(DeviceTestServiceGetTask(), FALSE);

        dts->pairing_requested = TRUE;
        dts->spp_service_registered = TRUE;
    }
    else
    {
        DEBUG_LOG_ERROR("deviceTestService_HandleSppsStartCfm. SPP Server failed to start. sts:%d", spps_start->status);
        Panic();
    }
}


static void deviceTestService_HandleSppsStopCfm(const SPP_STOP_SERVICE_CFM_T *spps_stop)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsStopCfm sts:%d", spps_stop->status);

    dts->spp_service_registered = FALSE;

    deviceTestService_ContinueDisconnect();
}

static void deviceTestService_HandleSppsConnectInd(const SPP_CONNECT_IND_T *connect)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsConnectInd x%06x channel:%d",
                                connect->addr.lap, connect->server_channel);

    SppConnectResponse(DeviceTestServiceGetSppTask(),
                       &connect->addr, TRUE, 
                       connect->sink, connect->server_channel, 
                       0);

    deviceTestService_SetConnectedAddress(&connect->addr);
    dts->rfc_sink = connect->sink;
    dts->rfc_source = StreamSourceFromSink(dts->rfc_sink);

    MessageStreamTaskFromSink(dts->rfc_sink, DeviceTestServiceGetSppTask());
    MessageStreamTaskFromSource(dts->rfc_source, DeviceTestServiceGetSppTask());
}


static void deviceTestService_HandleSppServerConnectCfm(const SPP_SERVER_CONNECT_CFM_T *connect)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsConnectCfm SPP:%p sts:%d",
                connect->spp, connect->status);

    if (connect->status == spp_connect_success)
    {
        dts->spp = connect->spp;
    }
    else if (connect->status != spp_connect_pending)
    {
        DEBUG_LOG_WARN("deviceTestService_HandleSppServerConnectCfm Status unhandled :%d",
                       connect->status);

        SppStartService(DeviceTestServiceGetSppTask());
    }
}


static void deviceTestService_HandleSppsDisconnectCfm(const SPP_DISCONNECT_CFM_T *disconn)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsDisconnectCfm SPP:%p sts:%d",
                disconn->spp, disconn->status);

    if (disconn->status != spp_disconnect_success)
    {
        DEBUG_LOG_WARN("deviceTestService_HandleSppsDisconnectCfm Disconnect failed");
    }

    deviceTestService_TidyAfterConnectionLoss();
    deviceTestService_ContinueDisconnect();
}


static void deviceTestService_HandleSppsDisconnectInd(const SPP_DISCONNECT_IND_T *disconn)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleSppsDisconnectInd SPP:%p sts:%d",
                disconn->spp, disconn->status);

    deviceTestService_TidyAfterConnectionLoss();

    if (!deviceTestService_ContinueDisconnect())
    {
        SppStartService(DeviceTestServiceGetSppTask());
    }
}


static void deviceTestService_HandleDataFromSource(Source source)
{
    DEBUG_LOG_FN_ENTRY("deviceTestService_HandleDataFromSource. size:%d", SourceSize(source));

    while (!!SourceBoundary(source))
    {
#ifdef DEVELOPMENT_DEBUG
        {
            const uint8 *ptr = SourceMap(source);
            uint16 packet_size = SourceSize(source);

            uint16 to_print = MIN(packet_size,20);
            uint16 i;

#define P(_i) ((_i<to_print)?(isprint(ptr[_i])?ptr[_i]:'.'):'_')
            for (i=0; i < to_print; i+=5)
            {
                DEBUG_LOG_DEBUG("Rx: %c%c%c%c%c",P(i),P(i+1),P(i+2),P(i+3),P(i+4));
            }
        }
#endif
        if (!DeviceTestServiceParser_parseSource(source, DeviceTestServiceGetTask()))
        {
            break;
        }
    }
}

static void deviceTestService_SppsMessageHandler(Task task, MessageId id, Message message)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    UNUSED(task);

    DEBUG_LOG_FN_ENTRY("deviceTestService_SppMessageHandler Active:%d Id:x%x (%d) %d", dts->active, id, id, id&0xFF);

    if (!dts->active)
    {
        return;
    }

    switch (id)
    {
        case CL_RFCOMM_CONTROL_IND:
            /* This message is forwarded by the SPP Server library */
            break;

        case SPPS_START_SERVICE_CFM:
            deviceTestService_HandleSppsStartCfm((const SPP_START_SERVICE_CFM_T *)message);
            break;

        case SPPS_CONNECT_IND:
            deviceTestService_HandleSppsConnectInd((const SPP_CONNECT_IND_T *)message);
            break;

        case SPPS_SERVER_CONNECT_CFM:
            deviceTestService_HandleSppServerConnectCfm((const SPP_SERVER_CONNECT_CFM_T *)message);
            break;

        case SPPS_DISCONNECT_CFM:
            deviceTestService_HandleSppsDisconnectCfm((const SPP_DISCONNECT_CFM_T *)message);
            break;

        case SPPS_DISCONNECT_IND:
            deviceTestService_HandleSppsDisconnectInd((const SPP_DISCONNECT_IND_T *)message);
            break;

        case SPPS_STOP_SERVICE_CFM:
            deviceTestService_HandleSppsStopCfm((const SPP_STOP_SERVICE_CFM_T *)message);
            break;

        case SPPS_MESSAGE_MORE_DATA:
            if (((const SPP_MESSAGE_MORE_DATA_T *)message)->source == dts->rfc_source)
            {
                deviceTestService_HandleDataFromSource(dts->rfc_source);
            }
            else
            {
                DEBUG_LOG_WARN("deviceTestService_MessageHandler SPPS_MESSAGE_MORE_DATA mismatched source");
#ifdef DEVELOPMENT_DEBUG
                Panic();
#endif
            }
            break;

        default:
#ifdef DEVELOPMENT_DEBUG
            DEBUG_LOG_VERBOSE("deviceTestService_SppMessageHandler UNHANDLED Id:x%x (%x) %d", id, id, id&0xFF);
#endif
            break;
    }

}

bool DeviceTestService_Init(Task init_task)
{
    UNUSED(init_task);

    MessageSendLater(DeviceTestServiceGetTask(), 
                     DEVICE_TEST_SERVICE_INTERNAL_CLEAN_UP, NULL,
                     DeviceTestService_CleanupTimeOutMs());

    return TRUE;
}


bool DeviceTestService_TestMode(void)
{
    bool test_mode = FALSE;
    uint16 current_size_words;

    current_size_words = PsRetrieve(DeviceTestService_EnablingPskey(), NULL, 0);

    if (current_size_words)
    {
        uint16 *key_storage = PanicUnlessMalloc(current_size_words * sizeof(uint16));

        if (PsRetrieve(DeviceTestService_EnablingPskey(),
                       key_storage,
                       current_size_words) >= 1)
        {
            test_mode = key_storage[0] != 0;
        }
        free(key_storage);
    }

    DEBUG_LOG_INFO("DeviceTestService_TestMode : %d", test_mode);

    return test_mode;
}


void DeviceTestService_SaveTestMode(uint16 mode)
{
    uint16 already_exists;
    uint16 current_size_words;
    uint16 written_words;
    uint16 *key_storage;

    already_exists = PsRetrieve(DeviceTestService_EnablingPskey(), NULL, 0);
    current_size_words = MAX(1, already_exists);
    key_storage = PanicUnlessMalloc(current_size_words * sizeof(uint16));

    if (already_exists)
    {
        if (!PsRetrieve(DeviceTestService_EnablingPskey(),
                        key_storage,
                        current_size_words) >= 1)
        {
            Panic();
        }
    }
    key_storage[0] = mode;

    written_words = PsStore(DeviceTestService_EnablingPskey(),
                            key_storage, current_size_words);
    free(key_storage);

    if (written_words != current_size_words)
    {
        DEBUG_LOG_WARN("DeviceTestService_SaveTestMode Unable to save mode. %d words written", 
                       written_words);
    }
    else
    {
        DEBUG_LOG_INFO("DeviceTestService_SaveTestMode Saved mode:%d. %d words written", 
                       mode, written_words);
    }
}


void DeviceTestService_Start(Task app_task)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();
    uint16 service_record_length;

    if (dts->active)
    {
        DEBUG_LOG_ERROR("DeviceTestService_Start. Already running.");
        Panic();
    }

    if (dts->disconnecting)
    {
        DEBUG_LOG_WARN("DeviceTestService_Start. Disconnection in progress.");
        /*! May wish to do more here, but the app will receive an ENDED notification */
        return;
    }

    deviceTestService_SaveAndSetConnectableState();

    const uint8 *service_record = sdp_GetDeviceTestServiceServiceRecord(&service_record_length);
    spps_set_sdp_service_record(service_record, service_record_length);
    SppStartService(DeviceTestServiceGetSppTask());

    dts->app_task = app_task;
    dts->active = TRUE;

    appPhyStateUnregisterClient(DeviceTestServiceGetTask());
}


void DeviceTestService_Stop(Task app_task)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();
    bool active = dts->active || dts->app_task;

    PanicFalse(!dts->app_task || (app_task == dts->app_task));

    if (dts->disconnecting)
    {
        DEBUG_LOG_WARN("DeviceTestService_Stop. Already disconnecting.");
        return;
    }

    if (active)
    {
        DEBUG_LOG_FN_ENTRY("DeviceTestService_Stop.");

        MessageCancelAll(DeviceTestServiceGetTask(),
                    DEVICE_TEST_SERVICE_INTERNAL_END_TESTING);
        MessageSend(DeviceTestServiceGetTask(),
                    DEVICE_TEST_SERVICE_INTERNAL_END_TESTING, NULL);
    }
    else
    {
        DEBUG_LOG_FN_ENTRY("DeviceTestService_Stop. Not active");

        MessageSend(app_task, DEVICE_TEST_SERVICE_ENDED, NULL);
    }
}


bool DeviceTestService_IsActive(void)
{
    return DeviceTestServiceIsActive();
}

#endif /* INCLUDE_DEVICE_TEST_SERVICE */
