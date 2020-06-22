/*!
\copyright  Copyright (c) 2015 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       bt_device.c
\brief      Device Management.
*/

#include "bt_device_marshal_typedef.h"
#include "bt_device_marshal_table.h"
#include "bt_device_typedef.h"
#include "device_properties.h"

#include <panic.h>
#include <connection.h>
#include <device.h>
#include <device_list.h>
#include <marshal.h>
#include <ps.h>
#include <string.h>
#include <stdlib.h>
#include <region.h>
#include <service.h>

#include "av.h"
#include "device_db_serialiser.h"
#include "adk_log.h"
#include "a2dp_profile.h"

#include <connection_manager.h>
#include <connection_manager_config.h>
#include <hfp_profile.h>
#include <scofwd_profile.h>
#include "scofwd_profile_config.h"
#include "mirror_profile.h"
#include "ui.h"

/*! \brief Macro for simplifying creating messages */
#define MAKE_DEVICE_MESSAGE(TYPE) \
    TYPE##_T *message = PanicUnlessNew(TYPE##_T);
/*! \brief Macro for simplying copying message content */
#define COPY_DEVICE_MESSAGE(src, dst) *(dst) = *(src);

#define SIZE_OF_PAYLOAD_TYPE    0x1
#define PAYLOAD_DATA_OFFSET     (SIZE_OF_PAYLOAD_TYPE)

/*! \brief BT device internal messages */
enum
{
    BT_INTERNAL_MSG_STORE_PS_DATA,            /*!< Store device data in PS */
};

/*! \brief Delay before storing the device data in ps */
#define BT_DEVICE_STORE_PS_DATA_DELAY D_SEC(1)

/*!< App device management task */
deviceTaskData  app_device;

static unsigned Device_GetCurrentContxt(void);

static bdaddr btDevice_SanitiseBdAddr(const bdaddr *bd_addr)
{
    bdaddr sanitised_bdaddr = {0};
    sanitised_bdaddr.uap = bd_addr->uap;
    sanitised_bdaddr.lap = bd_addr->lap;
    sanitised_bdaddr.nap = bd_addr->nap;
    return sanitised_bdaddr;
}

static void btDevice_PrintDeviceInfo(device_t device, void *data)
{
    size_t size = 0;
    bdaddr *addr = NULL;
    deviceType *type = NULL;
    uint16 flags = 0;

    UNUSED(data);

    DEBUG_LOG("btDevice_PrintDeviceInfo");

    Device_GetProperty(device, device_property_bdaddr, (void *)&addr, &size);
    DEBUG_LOG("bd addr %04x:%02x:%06x", addr->nap, addr->uap, addr->lap);

    Device_GetProperty(device, device_property_type, (void *)&type, &size);

    switch(*type)
    {
        case DEVICE_TYPE_UNKNOWN:
            DEBUG_LOG("type is unknown");
            break;

        case DEVICE_TYPE_EARBUD:
            DEBUG_LOG("type is earbud");
            break;

        case DEVICE_TYPE_HANDSET:
            DEBUG_LOG("type is handset");
            break;

        case DEVICE_TYPE_SELF:
            DEBUG_LOG("type is self");
            break;

        default:
            DEBUG_LOG("type is INVALID!!!");
    }

    Device_GetPropertyU16(device, device_property_flags, (void *)&flags);

    if(flags & DEVICE_FLAGS_PRIMARY_ADDR)
    {
        DEBUG_LOG("has flag DEVICE_FLAGS_PRIMARY_ADDR");
    }

    if(flags & DEVICE_FLAGS_SECONDARY_ADDR)
    {
        DEBUG_LOG("has flag DEVICE_FLAGS_SECONDARY_ADDR");
    }

    if(flags & DEVICE_FLAGS_MIRRORING_C_ROLE)
    {
        DEBUG_LOG("has flag DEVICE_FLAGS_MIRRORING_C_ROLE");
    }
}

static device_t btDevice_CreateDevice(const bdaddr *bd_addr, deviceType type)
{
    deviceLinkMode link_mode = DEVICE_LINK_MODE_UNKNOWN;
    device_t device = Device_Create();

    bdaddr sanitised_bdaddr = btDevice_SanitiseBdAddr(bd_addr);
    Device_SetProperty(device, device_property_bdaddr, &sanitised_bdaddr, sizeof(bdaddr));
    Device_SetProperty(device, device_property_type, &type, sizeof(deviceType));
    Device_SetProperty(device, device_property_link_mode, &link_mode, sizeof(deviceLinkMode));
    Device_SetPropertyU8(device, device_property_supported_profiles, 0x0);
    Device_SetPropertyU8(device, device_property_last_connected_profiles, 0x0);
    Device_SetPropertyU16(device, device_property_flags, 0x0);

    return device;
}

device_t BtDevice_GetDeviceCreateIfNew(const bdaddr *bd_addr, deviceType type)
{
    device_t device = NULL;

    DEBUG_LOG("BtDevice_GetDeviceCreateIfNew: %04x %02x %06x type %u",
        bd_addr->nap, bd_addr->uap, bd_addr->lap, type);

    bdaddr sanitised_bdaddr = btDevice_SanitiseBdAddr(bd_addr);
    device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, &sanitised_bdaddr, sizeof(bdaddr));
    if (!device)
    {
        DEBUG_LOG("- new");
        device = btDevice_CreateDevice(bd_addr, type);
        if (!DeviceList_AddDevice(device))
        {
            Device_Destroy(&device);

            /* As can't add the device to the device list so no point going forward */
            DEBUG_LOG("BtDevice_GetDeviceCreateIfNew can't add device to the device list");
            Panic();
        }
    }
    else
    {
        deviceType *existing_type = NULL;
        size_t size = 0;
        
        PanicFalse(Device_GetProperty(device, device_property_type, (void *)&existing_type, &size));
        DEBUG_LOG_ERROR("- existing type %u", *existing_type);
        PanicFalse(*existing_type == type);
    }

    return device;
}

static bool btDevice_DeviceIsValid_flag;

static void btDevice_Matches(device_t device, void *sought_device)
{
    if ((void*)device == sought_device)
    {
        btDevice_DeviceIsValid_flag = TRUE;
    }
}

bool BtDevice_DeviceIsValid(device_t device)
{
    btDevice_DeviceIsValid_flag = FALSE;

    DeviceList_Iterate(btDevice_Matches, (void*)device);

    DEBUG_LOG_VERBOSE("BtDevice_DeviceIsValid %p=%d", device, btDevice_DeviceIsValid_flag);

    return btDevice_DeviceIsValid_flag;
}

bool BtDevice_isKnownBdAddr(const bdaddr *bd_addr)
{
    bdaddr sanitised_bdaddr = btDevice_SanitiseBdAddr(bd_addr);
    if (DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, &sanitised_bdaddr, sizeof(bdaddr)) != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

device_t BtDevice_GetDeviceForBdAddr(const bdaddr *bd_addr)
{
    bdaddr sanitised_bdaddr = btDevice_SanitiseBdAddr(bd_addr);
    return DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, &sanitised_bdaddr, sizeof(bdaddr));
}

static bool btDevice_GetDeviceBdAddr(deviceType type, bdaddr *bd_addr)
{
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type, sizeof(deviceType));
    if (device)
    {
        *bd_addr = DeviceProperties_GetBdAddr(device);
        return TRUE;
    }
    else
    {
        BdaddrSetZero(bd_addr);
        return FALSE;
    }
}

static void btDevice_StoreDeviceDataInPs(void)
{
    bdaddr handset_address = {0,0,0};
    appDeviceGetHandsetBdAddr(&handset_address);

    /* Update mru device in ps */
    appDeviceUpdateMruDevice(&handset_address);

    /* Store device data in ps */
    DeviceDbSerialiser_Serialise();

}

bool appDeviceGetPeerBdAddr(bdaddr *bd_addr)
{
//    return btDevice_GetDeviceBdAddr(DEVICE_TYPE_EARBUD, bd_addr);     
    bool rc = FALSE;     
    rc = btDevice_GetDeviceBdAddr(DEVICE_TYPE_EARBUD, bd_addr);    
    DEBUG_LOG("appDeviceGetPeerBdAddr %04x,%02x,%06lx", bd_addr->nap, bd_addr->uap, bd_addr->lap);    
    return rc;
}

bool appDeviceGetHandsetBdAddr(bdaddr *bd_addr)
{
    uint8 is_mru_handset = TRUE;
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &is_mru_handset, sizeof(uint8));
    if (device)
    {
        // Get MRU handset device
        *bd_addr = DeviceProperties_GetBdAddr(device);
        return TRUE;
    }
    else
    {
        // Get first Handset in Device Database
        return btDevice_GetDeviceBdAddr(DEVICE_TYPE_HANDSET, bd_addr);
    }
}

bool BtDevice_IsPairedWithHandset(void)
{
    bdaddr bd_addr;
    BdaddrSetZero(&bd_addr);
    return btDevice_GetDeviceBdAddr(DEVICE_TYPE_HANDSET, &bd_addr);
}

bool BtDevice_IsPairedWithPeer(void)
{
    bdaddr bd_addr;
    BdaddrSetZero(&bd_addr);
    return btDevice_GetDeviceBdAddr(DEVICE_TYPE_EARBUD, &bd_addr);
}

bool appDeviceGetFlags(bdaddr *bd_addr, uint16 *flags)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);

    if (device)
    {
        return Device_GetPropertyU16(device, device_property_flags, flags);
    }
    else
    {
        *flags = 0;
        return FALSE;
    }
}

bool appDeviceGetMyBdAddr(bdaddr *bd_addr)
{
    bool succeeded = FALSE;
    if (bd_addr && btDevice_GetDeviceBdAddr(DEVICE_TYPE_SELF, bd_addr))
    {
        succeeded = TRUE;
    }
    return succeeded;
}

bool appDeviceDelete(const bdaddr *bd_addr)
{
    DEBUG_LOG("appDeviceDelete addr = %04x,%02x,%06lx",bd_addr->nap, bd_addr->uap, bd_addr->lap);

    if (!ConManagerIsConnected(bd_addr))
    {
        ConnectionAuthSetPriorityDevice(bd_addr, FALSE);
        ConnectionSmDeleteAuthDevice(bd_addr);

        device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
        if (device)
        {
            DeviceList_RemoveDevice(device);
            Device_Destroy(&device);
            DeviceDbSerialiser_Serialise();

            BtDevice_PrintAllDevices();
        }

        return TRUE;
    }
    else
    {
        DEBUG_LOG("appDeviceDelete, Failed to delete device as connected");
        return FALSE;
    }
}

void BtDevice_DeleteAllHandsetDevices(void)
{
    device_t* devices = NULL;
    unsigned num_devices = 0;
    deviceType type = DEVICE_TYPE_HANDSET;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);
    if (devices && num_devices)
    {
        for (unsigned i=0; i< num_devices; i++)
        {
            bdaddr bd_addr = DeviceProperties_GetBdAddr(devices[i]);
            if (!appDeviceDelete(&bd_addr))
            {
                break;
            }
        }
    }
    free(devices);
    devices = NULL;
}

/*! @brief Update connectivity state for a device. */
static void appDeviceHandleConManagerConnectionInd(CON_MANAGER_CONNECTION_IND_T* ind)
{
    if (!ind->ble)
    {
        if (appDeviceIsHandset(&ind->bd_addr))
        {
            DEBUG_LOG("appDeviceHandleConManagerConnectionInd HANDSET CONN:%d Status:%d",
                                                        ind->connected, ind->reason);
        }
        else if (appDeviceIsPeer(&ind->bd_addr))
        {
            DEBUG_LOG("appDeviceHandleConManagerConnectionInd PEER CONN:%d Status:%d",
                                                        ind->connected, ind->reason);
        }
        else
        {
            DEBUG_LOG("appDeviceHandleConManagerConnectionInd UNKNOWN BREDR %lx CONN:%d Status:%d",
                                        ind->bd_addr.lap, ind->connected, ind->reason);
        }
    }
    else
    {
        DEBUG_LOG("appDeviceHandleConManagerConnectionInd UNKNOWN BLE %lx CONN:%d Status:%d",
                                    ind->bd_addr.lap, ind->connected, ind->reason);
        return;
    }

    /* If we've disconnected, clear just paired flag */
    device_t device = BtDevice_GetDeviceForBdAddr(&ind->bd_addr);
    if (!ind->connected && device)
    {
        uint16 flags = 0;
        Device_GetPropertyU16(device, device_property_flags, &flags);

        if (flags & DEVICE_FLAGS_JUST_PAIRED)
        {
            flags &= ~DEVICE_FLAGS_JUST_PAIRED;
            Device_SetPropertyU16(device, device_property_flags, flags);
            DEBUG_LOG("appDeviceHandleConManagerConnectionInd, clearing just paired flag");
        }
    }
}

bool BtDevice_IsFull(void)
{
    return appConfigMaxTrustedDevices() == DeviceList_GetNumOfDevices();
}

bool appDeviceHandleClDmLocalBdAddrCfm(Message message)
{
    CL_DM_LOCAL_BD_ADDR_CFM_T *cfm = (CL_DM_LOCAL_BD_ADDR_CFM_T *)message;

    DEBUG_LOG("appDeviceHandleClDmLocalBdAddrCfm");

    if (cfm->status != hci_success)
    {
        DEBUG_LOG("Failed to read local BDADDR");
        Panic();
    }
    deviceType type = DEVICE_TYPE_SELF;
    device_t my_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type, sizeof(deviceType));
    if (my_device)
    {
        bdaddr sanitised_bdaddr = btDevice_SanitiseBdAddr(&cfm->bd_addr);

        BtDevice_SetMyAddress(&sanitised_bdaddr);
        DEBUG_LOG("local device bd addr set to lap: 0x%x", sanitised_bdaddr.lap);
    }

    return TRUE;
}

/*! @brief BT device task message handler.
 */
static void appDeviceHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        /* Peer signalling message */
        case CON_MANAGER_CONNECTION_IND:
            appDeviceHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
            break;

        /* Bt device handover message */
        case BT_INTERNAL_MSG_STORE_PS_DATA:
            btDevice_StoreDeviceDataInPs();
            break;

        default:
            break;
    }
}

bool BtDevice_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    bool handled = FALSE;
    UNUSED(already_handled);

    switch(id)
    {
        case CL_SM_AUTH_DEVICE_DELETED_IND:
            {
                CL_SM_AUTH_DEVICE_DELETED_IND_T *ind = (CL_SM_AUTH_DEVICE_DELETED_IND_T *)message;
                bdaddr *bd_addr = &ind->taddr.addr;
                device_t device;

                DEBUG_LOG_INFO("BtDevice_HandleConnectionLibraryMessages CL_SM_AUTH_DEVICE_DELETED_IND type: 0x%x lap 0x%x", ind->taddr.type, bd_addr->lap);

                device = BtDevice_GetDeviceForBdAddr(bd_addr);
                if (device)
                {
                    uint16 flags = 0;
                    Device_GetPropertyU16(device, device_property_flags, &flags);
                    if ((flags & DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS) == 0)
                    {
                        DeviceList_RemoveDevice(device);
                        Device_Destroy(&device);
                        DeviceDbSerialiser_Serialise();

                        DEBUG_LOG_VERBOSE("BtDevice_HandleConnectionLibraryMessages device removed");
                    }
                }

                handled = TRUE;
            }
            break;

        default:
            break;
    }

    return handled;
}

static uint8 btDevice_calculateLengthPdd(void)
{
    marshaller_t marshaller;
    size_t space_required = 0;
    bt_device_pdd_t pdd = {0};

    marshaller = PanicNull(MarshalInit(bt_device_marshal_type_descriptors, NUMBER_OF_MARSHAL_OBJECT_TYPES));
    MarshalSetBuffer(marshaller, NULL, 0);
    PanicFalse(!Marshal(marshaller, &pdd, MARSHAL_TYPE_bt_device_pdd_t));
    space_required = MarshalRemaining(marshaller);
    MarshalClearStore(marshaller);
    MarshalDestroy(marshaller, TRUE);
    return space_required;
}

static uint8 btDevice_GetDeviceDataLen(device_t device)
{
    UNUSED(device);
    deviceTaskData *theDevice = DeviceGetTaskData();
    return theDevice->pdd_len;
}

static void btDevice_SerialisePersistentDeviceData(device_t device, void *buf, uint8 offset)
{
    marshaller_t marshaller;
    uint8* bufptr = (uint8 *)buf;
    bt_device_pdd_t data_to_marshal_from_device_database = {0};

    UNUSED(offset);

    BtDevice_GetDeviceData(device, &data_to_marshal_from_device_database);

    marshaller = PanicNull(MarshalInit(bt_device_marshal_type_descriptors, NUMBER_OF_MARSHAL_OBJECT_TYPES));
    MarshalSetBuffer(marshaller, bufptr, btDevice_GetDeviceDataLen(device));
    PanicFalse(Marshal(marshaller, &data_to_marshal_from_device_database, MARSHAL_TYPE_bt_device_pdd_t));
    MarshalClearStore(marshaller);
    MarshalDestroy(marshaller, TRUE);
}

static void btDevice_DeserialisePddAndIncrementIndexU8(uint8 *data, uint8 *buf, uint16 *index)
{
    *data = buf[*index];
    *index += sizeof(uint8);
}

static void btDevice_DeserialisePddAndIncrementIndexU16(uint16 *data, uint8 *buf, uint16 *index)
{
    *data = buf[*index + 1] << 8 | buf[*index];
    *index += sizeof(uint16);
}

static void btDevice_DeserialisePersistentDeviceData(device_t device, void *buffer, uint8 data_length, uint8 offset)
{
    bt_device_pdd_t device_data = {0};
    uint8 *buf = buffer;
    uint16 index = PAYLOAD_DATA_OFFSET;

    UNUSED(offset);

    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.a2dp_volume, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.hfp_profile, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.type, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.link_mode, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.supported_profiles, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.last_connected_profiles, buf, &index);

    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.padding, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.flags, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.sco_fwd_features, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.battery_server_config_l, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.battery_server_config_r, buf, &index);
    btDevice_DeserialisePddAndIncrementIndexU16(&device_data.gatt_server_config, buf, &index);

    btDevice_DeserialisePddAndIncrementIndexU8(&device_data.gatt_server_services_changed, buf, &index);
    
    if(index < data_length)
    {
        btDevice_DeserialisePddAndIncrementIndexU8(&device_data.voice_assistant, buf, &index);
    }
    
    if(index < data_length)
    {
        btDevice_DeserialisePddAndIncrementIndexU8(&device_data.dts, buf, &index);
    }

    BtDevice_SetDeviceData(device, &device_data);
}

void BtDevice_RegisterPddu(void)
{
    deviceTaskData *theDevice = DeviceGetTaskData();
    theDevice->pdd_len = btDevice_calculateLengthPdd();

    DeviceDbSerialiser_RegisterPersistentDeviceDataUser(
        0,
        btDevice_GetDeviceDataLen,
        btDevice_SerialisePersistentDeviceData,
        btDevice_DeserialisePersistentDeviceData);
}

bool appDeviceInit(Task init_task)
{
    deviceTaskData *theDevice = DeviceGetTaskData();

    DEBUG_LOG("appDeviceInit");

    theDevice->task.handler = appDeviceHandleMessage;
    TaskList_InitialiseWithCapacity(DeviceGetVersionClientTasks(), DEVICE_VERSION_CLIENT_TASKS_LIST_INIT_CAPACITY);

    /* register to receive notifications of connections */
    ConManagerRegisterConnectionsClient(&theDevice->task);

    Ui_RegisterUiProvider(ui_provider_device, Device_GetCurrentContxt);

    ConnectionReadLocalAddr(init_task);

    BtDevice_PrintAllDevices();

    return TRUE;
}

deviceType BtDevice_GetDeviceType(device_t device)
{
    deviceType type = DEVICE_TYPE_UNKNOWN;
    void *value = NULL;
    size_t size = sizeof(deviceType);
    if (Device_GetProperty(device, device_property_type, &value, &size))
    {
        type = *(deviceType *)value;
    }
    return type;
}

bool appDeviceIsPeer(const bdaddr *bd_addr)
{
    bool isPeer = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        if ((BtDevice_GetDeviceType(device) == DEVICE_TYPE_EARBUD) ||
            (BtDevice_GetDeviceType(device) == DEVICE_TYPE_SELF))
        {
            isPeer = TRUE;
        }
    }
    return isPeer;
}

bool BtDevice_LeDeviceIsPeer(const tp_bdaddr *tpaddr)
{
    bool device_is_peer;

    if (tpaddr->taddr.type == TYPED_BDADDR_RANDOM)
    {
        tp_bdaddr remote;

        if (VmGetPublicAddress(tpaddr, &remote))
        {
            device_is_peer = appDeviceIsPeer(&remote.taddr.addr);
        }
        else
        {
        /*  Assume no IRK => not bonded => not our peer  */
            device_is_peer = FALSE;
        }
    }
    else
    {
        device_is_peer = appDeviceIsPeer(&tpaddr->taddr.addr);
    }

    return device_is_peer;
}

bool appDeviceIsHandset(const bdaddr *bd_addr)
{
    return appDeviceTypeIsHandset(bd_addr);
}

bool appDeviceTypeIsHandset(const bdaddr *bd_addr)
{
    bool is_handset = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        if (BtDevice_GetDeviceType(device) == DEVICE_TYPE_HANDSET)
        {
            is_handset = TRUE;
        }
    }
    return is_handset;
}

bool BtDevice_IsProfileSupported(const bdaddr *bd_addr, uint8 profile_to_check)
{
    bool is_supported = FALSE;
    uint8 supported_profiles = 0;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device && Device_GetPropertyU8(device, device_property_supported_profiles, &supported_profiles))
    {
        is_supported = !!(supported_profiles & profile_to_check);
    }
    return is_supported;
}

device_t BtDevice_SetSupportedProfile(const bdaddr *bd_addr, uint8 profile_to_set)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        uint8 supported_profiles = 0;
        Device_GetPropertyU8(device, device_property_supported_profiles, &supported_profiles);
        supported_profiles |= profile_to_set;
        Device_SetPropertyU8(device, device_property_supported_profiles, supported_profiles);
    }
    return device;
}

void appDeviceSetLinkMode(const bdaddr *bd_addr, deviceLinkMode link_mode)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        Device_SetProperty(device, device_property_link_mode, (void *)&link_mode, sizeof(deviceLinkMode));
    }
}

bool appDeviceIsHandsetConnected(void)
{
    bool is_handset_connected = FALSE;
    device_t* devices = NULL;
    unsigned num_devices = 0;
    deviceType type = DEVICE_TYPE_HANDSET;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);
    if (devices && num_devices)
    {
        for (unsigned i=0; i< num_devices; i++)
        {
            bdaddr handset_addr = DeviceProperties_GetBdAddr(devices[i]);
            is_handset_connected = ConManagerIsConnected(&handset_addr);
            if (is_handset_connected)
                break;
        }
    }
    free(devices);
    devices = NULL;

    return is_handset_connected;
}

unsigned appDeviceNumOfHandsetsConnected(void)
{
    bool is_handset_connected = FALSE;
    device_t* devices = NULL;
    unsigned num_devices = 0;
    unsigned num_handsets_connected = 0;
    deviceType type = DEVICE_TYPE_HANDSET;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);

    if (devices && num_devices)
    {
        for (unsigned i=0; i< num_devices; i++)
        {
            bdaddr handset_addr = DeviceProperties_GetBdAddr(devices[i]);
            is_handset_connected = ConManagerIsConnected(&handset_addr);
            if (is_handset_connected)
            {
                DEBUG_LOG("appDeviceNumOfHandsetsConnected handset[%d] %04x,%02x,%06lx is connected", i, handset_addr.nap, handset_addr.uap, handset_addr.lap);
                num_handsets_connected += 1;
            }
        }
    }
    free(devices);
    devices = NULL;
    DEBUG_LOG("appDeviceNumOfHandsetsConnected num_handsets_connected =%d", num_handsets_connected);
    return num_handsets_connected;
}

static avInstanceTaskData* btDevice_GetAvInstanceForHandset(void)
{
    avInstanceTaskData *inst = NULL;
    device_t* devices = NULL;
    unsigned num_devices = 0;
    deviceType type = DEVICE_TYPE_HANDSET;

    DeviceList_GetAllDevicesWithPropertyValue(device_property_type, &type, sizeof(deviceType), &devices, &num_devices);
    if (devices && num_devices)
    {
        for (unsigned i=0; i< num_devices; i++)
        {
            bdaddr addr = DeviceProperties_GetBdAddr(devices[i]);
            inst = appAvInstanceFindFromBdAddr(&addr);
            if (inst)
                break;
        }
    }
    free(devices);
    devices = NULL;

    return inst;
}

bool appDeviceIsHandsetA2dpDisconnected(void)
{
    bool is_disconnected = TRUE;
    avInstanceTaskData *inst = btDevice_GetAvInstanceForHandset();
    if (inst)
    {
        if (!appA2dpIsDisconnected(inst))
            is_disconnected = FALSE;
    }
    return is_disconnected;
}

bool appDeviceIsHandsetA2dpConnected(void)
{
    bool is_connected = FALSE;
    avInstanceTaskData *inst = btDevice_GetAvInstanceForHandset();
    if (inst)
    {
        if (appA2dpIsConnected(inst))
            is_connected = TRUE;
    }
    return is_connected;
}

bool appDeviceIsHandsetA2dpStreaming(void)
{
    bool is_streaming = FALSE;
    avInstanceTaskData *inst = btDevice_GetAvInstanceForHandset();
    if (inst)
    {
        if (appA2dpIsStreaming(inst))
            is_streaming = TRUE;
    }
    return is_streaming;
}

bool appDeviceIsHandsetAvrcpDisconnected(void)
{
    bool is_disconnected = TRUE;
    avInstanceTaskData *inst = btDevice_GetAvInstanceForHandset();
    if (inst)
    {
        if (!appAvrcpIsDisconnected(inst))
            is_disconnected = FALSE;
    }
    return is_disconnected;
}

bool appDeviceIsHandsetAvrcpConnected(void)
{
    bool is_connected = FALSE;
    avInstanceTaskData *inst = btDevice_GetAvInstanceForHandset();
    if (inst)
    {
        if (appAvrcpIsConnected(inst))
            is_connected = TRUE;
    }
    return is_connected;
}

bool appDeviceIsHandsetHfpDisconnected(void)
{
    return appHfpIsDisconnected();
}

bool appDeviceIsHandsetHfpConnected(void)
{
    return appHfpIsConnected();
}

bool appDeviceIsHandsetScoActive(void)
{
    return appHfpIsScoActive();
}

bool appDeviceIsPeerConnected(void)
{
    bool is_peer_connected = FALSE;
    bdaddr peer_addr;
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        is_peer_connected = ConManagerIsConnected(&peer_addr);
    }
    return is_peer_connected;
}

bool appDeviceIsPeerA2dpConnected(void)
{
    bdaddr peer_addr;
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        avInstanceTaskData* inst = appAvInstanceFindFromBdAddr(&peer_addr);
        if (inst)
        {
            if (!appA2dpIsDisconnected(inst))
                return TRUE;
        }
    }
    return FALSE;
}

bool appDeviceIsPeerAvrcpConnected(void)
{
    bdaddr peer_addr;
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        avInstanceTaskData* inst = appAvInstanceFindFromBdAddr(&peer_addr);
        if (inst)
        {
            if (!appAvrcpIsDisconnected(inst))
                return TRUE;
        }
    }
    return FALSE;
}

bool appDeviceIsPeerAvrcpConnectedForAv(void)
{
    bdaddr peer_addr;
    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        avInstanceTaskData* inst = appAvInstanceFindFromBdAddr(&peer_addr);
        if (inst)
        {
            return appAvIsAvrcpConnected(inst);
        }
    }
    return FALSE;
}

/*! \brief Determine if SCOFWD is connected to peer earbud. */
bool appDeviceIsPeerScoFwdConnected(void)
{
    return ScoFwdIsConnected();
}

bool appDeviceIsPeerMirrorConnected(void)
{
    return MirrorProfile_IsConnected();
}

/*! \brief Set flag for handset device indicating if address needs to be sent to peer earbud.

    \param handset_bd_addr BT address of handset device.
    \param reqd  TRUE Flag is set, link key is required to be sent to peer earbud.
                 FALSE Flag is clear, link key does not need to be sent to peer earbud.
    \return bool TRUE Success, FALSE failure device not known.
*/
bool appDeviceSetHandsetAddressForwardReq(const bdaddr *handset_bd_addr, bool reqd)
{
    device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, handset_bd_addr, sizeof(bdaddr));
    if (device)
    {
        uint16 flags = 0;
        Device_GetPropertyU16(device, device_property_flags, &flags);

        if (reqd)
            flags |= DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD;
        else
            flags &= ~DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD;

        Device_SetPropertyU16(device, device_property_flags, flags);

        return TRUE;
    }

    return FALSE;
}

bool appDeviceIsTwsPlusHandset(const bdaddr *handset_bd_addr)
{
    UNUSED(handset_bd_addr);
    return FALSE;
}

bool appDeviceIsHandsetAnyProfileConnected(void)
{
    return appDeviceIsHandsetHfpConnected() ||
           appDeviceIsHandsetA2dpConnected() ||
           appDeviceIsHandsetAvrcpConnected();
}

void appDeviceRegisterDeviceVersionClient(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(DeviceGetVersionClientTasks()), client_task);
}

static void btDevice_ClearPreviousMruDevice(void)
{
    uint8 mru = TRUE;
    device_t old_mru_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_mru, &mru, sizeof(uint8));
    if (old_mru_device)
    {
        Device_SetPropertyU8(old_mru_device, device_property_mru, FALSE);
    }
}

void appDeviceUpdateMruDevice(const bdaddr *bd_addr)
{
    static bdaddr bd_addr_mru_cached = {0, 0, 0};

    if (!BdaddrIsSame(bd_addr, &bd_addr_mru_cached))
    {
        device_t new_mru_device = BtDevice_GetDeviceForBdAddr(bd_addr);
        if (new_mru_device)
        {
            if (BtDevice_GetDeviceType(new_mru_device)==DEVICE_TYPE_HANDSET)
            {
                btDevice_ClearPreviousMruDevice();

                Device_SetPropertyU8(new_mru_device, device_property_mru, TRUE);
            }
            ConnectionSmUpdateMruDevice(bd_addr);
            bd_addr_mru_cached = *bd_addr;
        }
        else
        {
            // Unexpectedly unable to find device address, reset mru cache
            memset(&bd_addr_mru_cached, 0, sizeof(bd_addr_mru_cached));
        }
    }
}

static unsigned Device_GetCurrentContxt(void)
{
    dm_provider_context_t current_ctxt;

    if(appDeviceIsHandsetHfpConnected() || appDeviceIsHandsetA2dpConnected())
    {
        current_ctxt = context_handset_connected;
    }
    else
    {
        current_ctxt = context_handset_not_connected;
    }

    return (unsigned)current_ctxt;
}

static bool appDeviceGetBdAddrByFlag(bdaddr* bd_addr, uint16 desired_mask)
{
    uint16 flags;

    /*! \todo Would we do better with a database scan and check on flags.
        Or make the property of PRI/SEC a field  */
    if (appDeviceGetMyBdAddr(bd_addr))
    {
        if (appDeviceGetFlags(bd_addr, &flags))
        {
            if ((flags & desired_mask) == desired_mask) 
            {
                return TRUE;
            }
        }
    }

    if (appDeviceGetPeerBdAddr(bd_addr))
    {
        if (appDeviceGetFlags(bd_addr, &flags))
        {
            if ((flags & desired_mask) == desired_mask) 
            {
                return TRUE;
            }
        }
    }

    BdaddrSetZero(bd_addr);
    return FALSE;
}

bool appDeviceGetPrimaryBdAddr(bdaddr* bd_addr)
{
    return appDeviceGetBdAddrByFlag(bd_addr, DEVICE_FLAGS_PRIMARY_ADDR);
}

bool appDeviceGetSecondaryBdAddr(bdaddr* bd_addr)
{
    return appDeviceGetBdAddrByFlag(bd_addr, DEVICE_FLAGS_SECONDARY_ADDR);
}

bool appDeviceIsPrimary(const bdaddr* bd_addr)
{
    bdaddr primary_addr;
    return (appDeviceGetBdAddrByFlag(&primary_addr, DEVICE_FLAGS_PRIMARY_ADDR)
            && BdaddrIsSame(bd_addr, &primary_addr));
}

bool appDeviceIsSecondary(const bdaddr* bd_addr)
{
    bdaddr secondary_addr;
    return (appDeviceGetBdAddrByFlag(&secondary_addr, DEVICE_FLAGS_SECONDARY_ADDR)
            && BdaddrIsSame(bd_addr, &secondary_addr));
}

bool BtDevice_IsMyAddressPrimary(void)
{
    bdaddr self, primary;
    bool is_primary = FALSE;
    if(appDeviceGetPrimaryBdAddr(&primary) && appDeviceGetMyBdAddr(&self))
    {
        is_primary = BdaddrIsSame(&primary, &self);
    }
    DEBUG_LOG("BtDevice_AmIPrimary =%d, primary %04x,%02x,%06lx, self %04x,%02x,%06lx", is_primary, primary.lap, primary.uap, primary.nap, self.lap, self.uap, self.nap );
    return is_primary;
}

static void btDevice_ClearJustPairedFlag(device_t device)
{
    uint16 flags = 0;
    Device_GetPropertyU16(device, device_property_flags, &flags);

    /* got a profile connection, so the just paired flag is no longer
     * valid, clear it */
    if (flags & DEVICE_FLAGS_JUST_PAIRED)
        flags &= ~DEVICE_FLAGS_JUST_PAIRED;

    Device_SetPropertyU16(device, device_property_flags, flags);
}

/*! \brief Set flag to indicate whether the provided profile was connected or not

    \param device the device to modify
    \param profile_mask A bit mask of the profiles to set as connected/diconnected
    \param connected TRUE if profile was connected, FALSE if it wasn't connected.
*/
void BtDevice_SetLastConnectedProfilesForDevice(device_t device, uint8 profile_mask, bool connected)
{
    uint8 connected_profiles = 0;
    Device_GetPropertyU8(device, device_property_last_connected_profiles, &connected_profiles);

    DEBUG_LOG("BtDevice_SetLastConnectedProfilesForDevice, device 0x%x connected_profiles %02x profiles_to_set %02x connected %d", 
                device, connected_profiles, profile_mask, connected);
    connected_profiles &= ~profile_mask;
    if (connected)
    {
        connected_profiles |= profile_mask;
        btDevice_ClearJustPairedFlag(device);

        /* Update the PDL with this profile connection state in the persistent device data. This is in order to ensure
           we don't lose state information in the case of unexpected power loss. N.b. Normally serialisation occurs
           during a controlled shutdown of the App. */
        DeviceDbSerialiser_Serialise();
    }
    Device_SetPropertyU8(device, device_property_last_connected_profiles, connected_profiles);
    DEBUG_LOG("BtDevice_SetLastConnectedProfilesForDevice, device 0x%x connected_profiles %02x", device, connected_profiles);
}

/*! \brief Set flag to indicate whether the provided profile was connected or not

    \param bd_addr Pointer to read-only device BT address.
    \param profile_mask A bit mask of the profiles to set as connected/diconnected
    \param connected TRUE if profile was connected, FALSE if it wasn't connected.
*/
void BtDevice_SetLastConnectedProfiles(const bdaddr *bd_addr, uint8 profile_mask, bool connected)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        BtDevice_SetLastConnectedProfilesForDevice(device, profile_mask, connected);
    }
}

/*! \brief Determine which profiles were connected to a device.
*/
uint8 BtDevice_GetLastConnectedProfilesForDevice(device_t device)
{
    uint8 conn_profiles = 0;
    if (device)
    {
        Device_GetPropertyU8(device, device_property_last_connected_profiles, &conn_profiles);
        DEBUG_LOG("BtDevice_GetLastConnectedProfiles, device 0x%x connected_profiles %02x", device, conn_profiles);
    }
    return conn_profiles;
}

/*! \brief Determine which profiles were connected to a device.
*/
uint8 BtDevice_GetLastConnectedProfiles(const bdaddr *bd_addr)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return BtDevice_GetLastConnectedProfilesForDevice(device);
}

/*! \brief Determine if a device had profiles connected. */
bool BtDevice_GetWasConnected(const bdaddr *bd_addr)
{
    return (BtDevice_GetLastConnectedProfiles(bd_addr) != 0);
}

void BtDevice_SetConnectedProfiles(device_t device, uint8 connected_profiles_mask )
{
    PanicNull(device);

    DEBUG_LOG("BtDevice_SetConnectedProfiles, connected_profiles %02x", connected_profiles_mask);
    Device_SetPropertyU8(device, device_property_connected_profiles, connected_profiles_mask);
}

uint8 BtDevice_GetConnectedProfiles(device_t device)
{
    uint8 connected_profiles_mask = 0;
    PanicNull(device);
    Device_GetPropertyU8(device, device_property_connected_profiles, &connected_profiles_mask);
    return connected_profiles_mask;
}

/*! \brief Determine if a device has just paired.

    \param bd_addr Pointer to read-only BT device address.
    \return bool TRUE address is just paired device, FALSE not just paired.
*/
bool BtDevice_IsJustPaired(const bdaddr *bd_addr)
{
    bool just_paired = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        uint16 flags = 0;
        Device_GetPropertyU16(device, device_property_flags, &flags);
        just_paired = !!(flags & DEVICE_FLAGS_JUST_PAIRED);
    }
    return just_paired;
}

/*! \brief Writes the device attribute profile connected/supported flags based on
    peer's profile connection state. Is much faster than calling individual
    WasConnected/IsSupported functions for each profile. As it only read attributes
    once and only writes attributes when modified.
    \param bd_addr The peer's handset address.
    \param peer_a2dp_connected TRUE if the peer's A2DP profile is connected to the handset.
    \param peer_avrcp_connected TRUE if the peer's AVRCP profile is connected to the handset.
    \param peer_hfp_connected TRUE if the peer's HFP profile is connected to the handset.
    \param peer_hfp_profile_connected The hfp version of the peer's handset.
    \return uint8 Bitmask of profiles that were known to be supported before this function was called.
*/
uint8 BtDevice_SetProfileConnectedAndSupportedFlagsFromPeer(const bdaddr *bd_addr,
    bool peer_a2dp_connected, bool peer_avrcp_connected, bool peer_hfp_connected,
    hfp_profile peer_hfp_profile_connected)
{
    uint8 was_supported = 0;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {
        uint8 connected_profiles = 0;
        uint8 supported_profiles = 0;
        uint8 our_hfp_profile = 0;

        Device_GetPropertyU8(device, device_property_last_connected_profiles, &connected_profiles);
        Device_GetPropertyU8(device, device_property_supported_profiles, &supported_profiles);
        Device_GetPropertyU8(device, device_property_hfp_profile, &our_hfp_profile);

        DEBUG_LOG("SetProfileConnectedAndSupportedFlagsFromPeer, connected=0x%02x, supported=0x%02x",
            connected_profiles, supported_profiles);

        /* For a TWS Standard headset, if the peer is connected then this Earbud should
           also consider itself 'was connected' for those profiles */
        if (!appDeviceIsTwsPlusHandset(bd_addr))
        {
            if (peer_a2dp_connected)
            {
                connected_profiles |= DEVICE_PROFILE_A2DP;
            }
            if (peer_hfp_connected)
            {
                connected_profiles |= DEVICE_PROFILE_HFP;
            }
            if (peer_a2dp_connected || peer_hfp_connected)
            {
                btDevice_ClearJustPairedFlag(device);
            }
        }

        was_supported = supported_profiles;
        if (peer_a2dp_connected)
        {
            supported_profiles |= DEVICE_PROFILE_A2DP;
        }
        if (peer_avrcp_connected)
        {
            supported_profiles |= DEVICE_PROFILE_AVRCP;
        }
        if (peer_hfp_connected)
        {
            supported_profiles |= DEVICE_PROFILE_HFP;
            our_hfp_profile = peer_hfp_profile_connected;
        }

        {
            DEBUG_LOG("SetProfileConnectedAndSupportedFlagsFromPeer, connected=0x%02x, supported=0x%02x",
                connected_profiles, supported_profiles);
            Device_SetPropertyU8(device, device_property_last_connected_profiles, connected_profiles);
            Device_SetPropertyU8(device, device_property_supported_profiles, supported_profiles);
            Device_SetPropertyU8(device, device_property_hfp_profile, our_hfp_profile);
        }

        /* Update the PDL with this state provided by the peer in our persistent device data. This is in order to ensure
           we don't lose state information in the case of unexpected power loss. N.b. Normally serialisation occurs
           during a controlled shutdown of the App. */
        DeviceDbSerialiser_Serialise();
    }

    return was_supported;
}

/*! \brief Set flag for handset device indicating if link key needs to be sent to
           peer earbud.

    \param handset_bd_addr BT address of handset device.
    \param reqd  TRUE link key TX is required, FALSE link key TX not required.
    \return bool TRUE Success, FALSE failure.
 */
bool BtDevice_SetHandsetLinkKeyTxReqd(bdaddr *handset_bd_addr, bool reqd)
{
    if (appDeviceGetHandsetBdAddr(handset_bd_addr))
    {
        uint16 flags = 0;
        device_t device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_bdaddr, handset_bd_addr, sizeof(bdaddr));
        PanicFalse(device);
        Device_GetPropertyU16(device, device_property_flags, &flags);
        if (reqd)
            flags |= DEVICE_FLAGS_HANDSET_LINK_KEY_TX_REQD;
        else
            flags &= ~DEVICE_FLAGS_HANDSET_LINK_KEY_TX_REQD;
        Device_SetPropertyU16(device, device_property_flags, flags);
        return TRUE;
    }
    return FALSE;
}

bool appDeviceSetBatterServerConfigLeft(const bdaddr *bd_addr, uint16 config)
{
    bool config_set = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {  
        uint16 client_config = config;
        config_set = Device_GetPropertyU16(device, device_property_battery_server_config_l, &client_config);
        if (!config_set || (config != client_config))
        {
            Device_SetPropertyU16(device, device_property_battery_server_config_l, config);
        }
    }
    return config_set;
}

bool appDeviceGetBatterServerConfigLeft(const bdaddr *bd_addr, uint16* config)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return (device) ? Device_GetPropertyU16(device, device_property_battery_server_config_l, config) : FALSE;
}

bool appDeviceSetBatterServerConfigRight(const bdaddr *bd_addr, uint16 config)
{
    bool config_set = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {  
        uint16 client_config = config;
        config_set = Device_GetPropertyU16(device, device_property_battery_server_config_r, &client_config);
        if (!config_set || (config != client_config))
        {
            Device_SetPropertyU16(device, device_property_battery_server_config_r, config);
        }
    }
    return config_set;
}

bool appDeviceGetBatterServerConfigRight(const bdaddr *bd_addr, uint16* config)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return (device) ? Device_GetPropertyU16(device, device_property_battery_server_config_r, config) : FALSE;
}

bool appDeviceSetGattServerConfig(const bdaddr *bd_addr, uint16 config)
{
    bool config_set = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {  
        uint16 client_config = config;
        config_set = Device_GetPropertyU16(device, device_property_gatt_server_config, &client_config);
        if (!config_set || (config != client_config))
        {
            Device_SetPropertyU16(device, device_property_gatt_server_config, config);
        }
    }
    return config_set;
}

bool appDeviceGetGattServerConfig(const bdaddr *bd_addr, uint16* config)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return (device) ? Device_GetPropertyU16(device, device_property_gatt_server_config, config) : FALSE;
}

bool appDeviceSetGattServerServicesChanged(const bdaddr *bd_addr, uint8 flag)
{
    bool config_set = FALSE;
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    if (device)
    {  
        uint8 client_flag = flag;
        config_set = Device_GetPropertyU8(device, device_property_gatt_server_services_changed, &client_flag);
        if (!config_set || (flag != client_flag))
        {
            Device_SetPropertyU8(device, device_property_gatt_server_services_changed, flag);
        }
    }
    return config_set;
}

bool appDeviceGetGattServerServicesChanged(const bdaddr *bd_addr, uint8* flag)
{
    device_t device = BtDevice_GetDeviceForBdAddr(bd_addr);
    return (device) ? Device_GetPropertyU8(device, device_property_gatt_server_services_changed, flag) : FALSE;
}

static bool btDevice_ValidateAddressesForAddressSwap(const bdaddr *bd_addr_1, const bdaddr *bd_addr_2)
{
    if(!BtDevice_GetDeviceForBdAddr(bd_addr_1))
    {
        DEBUG_LOG("There is no device corresponding to address lap 0x%x", bd_addr_1->lap);
        return FALSE;
    }

    if(!BtDevice_GetDeviceForBdAddr(bd_addr_2))
    {
        DEBUG_LOG("There is no device corresponding to address lap 0x%x", bd_addr_2->lap);
        return FALSE;
    }

    if(BdaddrIsSame(bd_addr_1, bd_addr_2))
    {
        DEBUG_LOG("Addresses are the same, no point in swapping them");
        return FALSE;
    }

    if(!appDeviceIsPeer(bd_addr_1))
    {
        DEBUG_LOG("Address lap 0x%x doesn't belong to a peer device", bd_addr_1->lap);
        return FALSE;
    }

    if(!appDeviceIsPeer(bd_addr_2))
    {
        DEBUG_LOG("Address lap 0x%x doesn't belong to a peer device", bd_addr_2->lap);
        return FALSE;
    }

    return TRUE;
}

static void btDevice_SwapFlags(uint16 *flags_1, uint16 *flags_2, uint16 flags_to_swap)
{
    uint16 temp_1;
    uint16 temp_2;

    temp_1 = *flags_1 & flags_to_swap;
    *flags_1 &= ~flags_to_swap;
    temp_2 = *flags_2 & flags_to_swap;
    *flags_2 &= ~flags_to_swap;
    *flags_1 |= temp_2;
    *flags_2 |= temp_1;
}

bool BtDevice_SwapAddresses(const bdaddr *bd_addr_1, const bdaddr *bd_addr_2)
{
    device_t device_1;
    device_t device_2;

    uint16 flags_1;
    uint16 flags_2;

    PanicNull((bdaddr *)bd_addr_1);
    PanicNull((bdaddr *)bd_addr_2);

    DEBUG_LOG("BtDevice_SwapAddresses addr 1 lap 0x%x, addr 2 lap 0x%x", bd_addr_1->lap, bd_addr_2->lap);

    if(!btDevice_ValidateAddressesForAddressSwap(bd_addr_1, bd_addr_2))
    {
        return FALSE;
    }

    device_1 = BtDevice_GetDeviceForBdAddr(bd_addr_1);
    device_2 = BtDevice_GetDeviceForBdAddr(bd_addr_2);

    /* Swap BT addresses */

    Device_SetProperty(device_1, device_property_bdaddr, (void*)bd_addr_2, sizeof(bdaddr));
    Device_SetProperty(device_2, device_property_bdaddr, (void*)bd_addr_1, sizeof(bdaddr));

    /* Swap flags associated with the BT address */

    Device_GetPropertyU16(device_1, device_property_flags, &flags_1);
    Device_GetPropertyU16(device_2, device_property_flags, &flags_2);

    btDevice_SwapFlags(&flags_1, &flags_2,
            DEVICE_FLAGS_PRIMARY_ADDR | DEVICE_FLAGS_SECONDARY_ADDR);

    Device_SetPropertyU16(device_1, device_property_flags, flags_1);
    Device_SetPropertyU16(device_2, device_property_flags, flags_2);

    return TRUE;
}

bool BtDevice_SetMyAddress(const bdaddr *new_bd_addr)
{
    bdaddr my_bd_addr;

    DEBUG_LOG("BtDevice_SetMyAddressBySwapping new_bd_addr lap 0x%x", new_bd_addr->lap);

    if(!appDeviceGetMyBdAddr(&my_bd_addr))
    {
        return FALSE;
    }

    if(BdaddrIsSame(&my_bd_addr, new_bd_addr))
    {
        DEBUG_LOG("BtDevice_SetMyAddressBySwapping address is already new_bdaddr, no need to swap");
        BtDevice_PrintAllDevices();
        return TRUE;
    }
    else
    {
        bool ret = BtDevice_SwapAddresses(&my_bd_addr, new_bd_addr);
        BtDevice_PrintAllDevices();
        return ret;
    }


}

void BtDevice_PrintAllDevices(void)
{
    DEBUG_LOG("BtDevice_PrintAllDevices number of devices: %d", DeviceList_GetNumOfDevices());

    DeviceList_Iterate(btDevice_PrintDeviceInfo, NULL);
}

void BtDevice_GetDeviceData(device_t device, bt_device_pdd_t *device_data)
{
    DEBUG_LOG("BtDevice_GetDeviceData");

    void *property_value = NULL;
    size_t size;

    Device_GetPropertyU8(device, device_property_a2dp_volume, &device_data->a2dp_volume);
    Device_GetPropertyU8(device, device_property_hfp_profile, &device_data->hfp_profile);
    Device_GetPropertyU8(device, device_property_supported_profiles, &device_data->supported_profiles);
    Device_GetPropertyU8(device, device_property_last_connected_profiles, &device_data->last_connected_profiles);

    Device_GetPropertyU16(device, device_property_flags, &device_data->flags);

    if (Device_GetProperty(device, device_property_type, &property_value, &size))
    {
        PanicFalse(size == sizeof(deviceType));
        device_data->type = *((deviceType *)property_value);
    }

    if (Device_GetProperty(device, device_property_link_mode, &property_value, &size))
    {
        PanicFalse(size == sizeof(deviceLinkMode));
        device_data->link_mode = *((deviceLinkMode *)property_value);
    }

    Device_GetPropertyU16(device, device_property_sco_fwd_features, &device_data->sco_fwd_features);
    Device_GetPropertyU16(device, device_property_battery_server_config_l, &device_data->battery_server_config_l);
    Device_GetPropertyU16(device, device_property_battery_server_config_r, &device_data->battery_server_config_r);
    Device_GetPropertyU16(device, device_property_gatt_server_config, &device_data->gatt_server_config);
    Device_GetPropertyU8(device, device_property_gatt_server_services_changed, &device_data->gatt_server_services_changed);
    Device_GetPropertyU8(device, device_property_voice_assistant, &device_data->voice_assistant);
    Device_GetPropertyU8(device, device_property_device_test_service, &device_data->dts);
}

void BtDevice_SetDeviceData(device_t device, const bt_device_pdd_t *device_data)
{
    DEBUG_LOG("BtDevice_SetDeviceData device-type %d", device_data->type);

    Device_SetProperty(device, device_property_type, &device_data->type, sizeof(deviceType));
    Device_SetPropertyU16(device, device_property_flags, device_data->flags);
    Device_SetProperty(device, device_property_link_mode, &device_data->link_mode, sizeof(deviceLinkMode));
    Device_SetPropertyU8(device, device_property_voice_assistant, device_data->voice_assistant);
    if (device_data->dts)
    {
        Device_SetPropertyU8(device, device_property_device_test_service, device_data->dts);
    }

    switch(device_data->type)
    {
        case DEVICE_TYPE_EARBUD:
            Device_SetPropertyU16(device, device_property_sco_fwd_features, device_data->sco_fwd_features);
            Device_SetPropertyU8(device, device_property_supported_profiles, device_data->supported_profiles);
            Device_SetPropertyU8(device, device_property_last_connected_profiles, device_data->last_connected_profiles);
            break;
        case DEVICE_TYPE_HANDSET:
            Device_SetPropertyU8(device, device_property_a2dp_volume, device_data->a2dp_volume);
            Device_SetPropertyU8(device, device_property_hfp_profile, device_data->hfp_profile);
            Device_SetPropertyU8(device, device_property_supported_profiles, device_data->supported_profiles);
            Device_SetPropertyU8(device, device_property_last_connected_profiles, device_data->last_connected_profiles);
            Device_SetPropertyU16(device, device_property_battery_server_config_l, device_data->battery_server_config_l);
            Device_SetPropertyU16(device, device_property_battery_server_config_r, device_data->battery_server_config_r);
            Device_SetPropertyU16(device, device_property_gatt_server_config, device_data->gatt_server_config);
            Device_SetPropertyU16(device, device_property_gatt_server_services_changed, device_data->gatt_server_services_changed);
            break;

        case DEVICE_TYPE_SELF:
        case DEVICE_TYPE_UNKNOWN:
        default:
            break;
    }

}

void BtDevice_StorePsDeviceDataWithDelay(void)
{
    MessageSendLater(&DeviceGetTaskData()->task, BT_INTERNAL_MSG_STORE_PS_DATA,
                     NULL, BT_DEVICE_STORE_PS_DATA_DELAY);
}

static tp_bdaddr btDevice_GetTpAddrFromTypedAddr(const typed_bdaddr *taddr)
{
    tp_bdaddr tpaddr;
    tp_bdaddr public_tpaddr;
    
    tpaddr.transport = TRANSPORT_BLE_ACL;
    tpaddr.taddr = *taddr;
    
    if(taddr->type == TYPED_BDADDR_RANDOM)
    {
        if(VmGetPublicAddress(&tpaddr, &public_tpaddr))
        {
            tpaddr = public_tpaddr;
        }
    }
    
    return tpaddr;
}

bool BtDevice_GetPublicAddress(const typed_bdaddr *source_taddr, typed_bdaddr *public_taddr)
{
    bool status;
    
    if (source_taddr->type == TYPED_BDADDR_PUBLIC)
    {
        *public_taddr = *source_taddr;
        status = TRUE;
    }
    else
    {
        tp_bdaddr tpaddr;
        
        tpaddr.transport = TRANSPORT_BLE_ACL;
        tpaddr.taddr = *source_taddr;
        status = VmGetPublicAddress(&tpaddr, &tpaddr);
        
        if (status)
        {
            *public_taddr = tpaddr.taddr;
        }
    }
    
    DEBUG_LOG("BtDevice_GetPublicAddress: %02x %04x %02x %06x -> %02x %04x %02x %06x (%u)",
        source_taddr->type,
        source_taddr->addr.nap, source_taddr->addr.uap, source_taddr->addr.lap,
        public_taddr->type,
        public_taddr->addr.nap, public_taddr->addr.uap, public_taddr->addr.lap,
        status);

    return status;
}

bool BtDevice_ResolvedBdaddrIsSame(const bdaddr *public_addr, const typed_bdaddr *taddr)
{
    typed_bdaddr resolved_taddr;
    bool is_same = FALSE;
    
    if (BtDevice_GetPublicAddress(taddr, &resolved_taddr))
    {
        if (BdaddrIsSame(public_addr, &resolved_taddr.addr))
        {
            is_same = TRUE;
        }
    }
    
    return is_same;
}

bool BtDevice_BdaddrTypedIsSame(const typed_bdaddr *taddr1, const typed_bdaddr *taddr2)
{
    tp_bdaddr tpaddr1 = btDevice_GetTpAddrFromTypedAddr(taddr1);
    tp_bdaddr tpaddr2 = btDevice_GetTpAddrFromTypedAddr(taddr2);
    
    return BdaddrTpIsSame(&tpaddr1, &tpaddr2);
}

bool BtDevice_SetDefaultProperties(device_t device)
{
    if(!BtDevice_SetFlags(device, DEVICE_FLAGS_JUST_PAIRED, DEVICE_FLAGS_JUST_PAIRED))
    {
        return FALSE;
    }
    if(!Device_SetPropertyU8(device, device_property_a2dp_volume, A2dpProfile_GetDefaultVolume()))
    {
        return FALSE;
    }
    if(!Device_SetPropertyU8(device, device_property_hfp_profile, hfp_handsfree_107_profile))
    {
        return FALSE;
    }
    
    return TRUE;
}

bool BtDevice_SetFlags(device_t device, uint16 flags_to_modify, uint16 flags)
{
    uint16 old_flags;
    uint16 new_flags;
    
    DEBUG_LOG("BtDevice_SetFlags %04x %04x", flags_to_modify, flags);
    
    if(!Device_GetPropertyU16(device, device_property_flags, &old_flags))
    {
        /* No flags property has been set, default to 0 */
        old_flags = 0;
    }
    
    DEBUG_LOG("BtDevice_SetFlags old %04x", old_flags);
    
    new_flags = old_flags;
    
    new_flags &= ~(flags_to_modify & ~flags);
    new_flags |= (flags_to_modify & flags);
    
    DEBUG_LOG("BtDevice_SetFlags new %04x", new_flags);
    
    if(new_flags != old_flags)
    {
        if(!Device_SetPropertyU16(device, device_property_flags, new_flags))
        {
            return FALSE;
        }
    }
    return TRUE;
}
