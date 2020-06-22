/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Component handling synchronisation of keys between peers.
*/

#include "key_sync.h"
#include "key_sync_private.h"
#include "key_sync_marshal_defs.h"

#include <peer_signalling.h>
#include <bt_device.h>

#include <logging.h>
#include <connection.h>

#include "device_properties.h"
#include <device_db_serialiser.h>

#include <device_list.h>

#include <panic.h>
#include <stdlib.h>
#include <stdio.h>

#define MAKE_KEY_SYNC_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);

/*! \brief Key Sync task data. */
key_sync_task_data_t key_sync;

/*! \brief Handle confirmation of transmission of a marshalled message. */
static void keySync_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);
}

static void keySync_HandleKeySyncConfirmation(const bdaddr* bd_addr, bool synced)
{
    if(!synced)
    {
        DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation sync failure reported by peer");
    }
    else
    {
        if(!appDeviceSetHandsetAddressForwardReq(bd_addr, FALSE))
        {
            DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation FAILED TO CLEAR ADDRESS FWD REQD!");
        }
        else
        {
            DEBUG_LOG_ALWAYS("keySync_HandleKeySyncConfirmation cleared ADDRESS FWD REQD");
        }
    }
}

/*! \brief Handle incoming marshalled messages from peer key sync component. */
static void keySync_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE_key_sync_req_t:
        {
            key_sync_req_t* req = (key_sync_req_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd rx key to add");
            /* update local PDL, note the API uses 16-bit words */
            ConnectionSmAddAuthDevice(keySync_GetTask(), &req->bd_addr, req->bits.trusted, TRUE,
                                      req->link_key_type, req->size_link_key / sizeof(uint16), (uint16*)req->link_key);
        }
        break;
        
        case MARSHAL_TYPE_key_sync_paired_device_req_t:
        {
            key_sync_paired_device_req_t* req = (key_sync_paired_device_req_t*)ind->msg;
            typed_bdaddr taddr = {TYPED_BDADDR_PUBLIC, req->bd_addr};

            device_t device = BtDevice_GetDeviceForBdAddr(&taddr.addr);
            if (device)
            {
                PanicFalse(BtDevice_SetFlags(device, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS));
            }

            ConnectionSmDeleteAuthDeviceReq(taddr.type, &taddr.addr);
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd rx paired device to add");
            ConnectionSmAddAuthDeviceRawRequest(keySync_GetTask(), &taddr, req->size_data / sizeof(uint16), (uint16*)req->data);
        }
        break;

        case MARSHAL_TYPE_key_sync_cfm_t:
        {
            key_sync_cfm_t* cfm = (key_sync_cfm_t*)ind->msg;
            DEBUG_LOG_ALWAYS("keySync_HandleMarshalledMsgChannelRxInd synced %u", cfm->synced);
            keySync_HandleKeySyncConfirmation(&cfm->bd_addr, cfm->synced);
        }
        break;

        default:
        break;
    }
    
    if(ind->msg)
    {
        free(ind->msg);
    }
}

static void keySync_HandleClSmGetAuthDeviceRawConfirm(CL_SM_GET_AUTH_DEVICE_RAW_CFM_T* cfm)
{
    bdaddr peer_addr;

    DEBUG_LOG_ALWAYS("keySync_HandleClSmGetAuthDeviceRawConfirm %u size %u", cfm->status, cfm->size_data);

    if ((cfm->status == success) && appDeviceGetPeerBdAddr(&peer_addr))
    {
        /* data size is specified in 16-bit words, adjust to 8-bit */
        size_t size_data = cfm->size_data * sizeof(uint16);
        
        key_sync_paired_device_req_t* req = PanicUnlessMalloc(sizeof(key_sync_paired_device_req_t) + (size_data - 1));

        req->bd_addr = cfm->peer_taddr.addr;
        req->size_data = size_data;
        memcpy(req->data, cfm->data, size_data);

        /* send to counterpart on other earbud */
        appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                         req, MARSHAL_TYPE_key_sync_paired_device_req_t);
    }
    else
    {
        DEBUG_LOG_ALWAYS("keySync_HandleClSmGetAuthDeviceRawConfirm no peer to send to");
    }
}

static void keySync_AddDeviceAttributes(bdaddr* bd_addr)
{
    device_t handset_device = PanicNull(BtDevice_GetDeviceCreateIfNew(bd_addr, DEVICE_TYPE_HANDSET));
    PanicFalse(BtDevice_SetDefaultProperties(handset_device));
    PanicFalse(BtDevice_SetFlags(handset_device, DEVICE_FLAGS_PRE_PAIRED_HANDSET, DEVICE_FLAGS_PRE_PAIRED_HANDSET));
    PanicFalse(BtDevice_SetFlags(handset_device, DEVICE_FLAGS_KEY_SYNC_PDL_UPDATE_IN_PROGRESS, DEVICE_FLAGS_NO_FLAGS));
}

static void keySync_SendKeySyncCfm(bdaddr* bd_addr, bool synced)
{
    DEBUG_LOG_ALWAYS("keySync_SendKeySyncCfm");
    key_sync_cfm_t* key_cfm = PanicUnlessMalloc(sizeof(key_sync_cfm_t));
    key_cfm->bd_addr = *bd_addr;
    key_cfm->synced = synced;
    appPeerSigMarshalledMsgChannelTx(keySync_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                     key_cfm, MARSHAL_TYPE_key_sync_cfm_t);
}

static void keySync_HandleClSmAddAuthDeviceConfirm(CL_SM_ADD_AUTH_DEVICE_CFM_T* cfm)
{
    bdaddr peer_addr;

    DEBUG_LOG_ALWAYS("keySync_HandleClSmAddAuthDeviceConfirm %u", cfm->status);

    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        keySync_SendKeySyncCfm(&cfm->bd_addr, cfm->status == success);
        keySync_AddDeviceAttributes(&cfm->bd_addr);
        BtDevice_PrintAllDevices();
    }
    else
    {
        DEBUG_LOG_ALWAYS("keySync_HandleClSmAddAuthDeviceConfirm no peer to send to");
    }
}

/*! Key Sync Message Handler. */
static void keySync_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    
    switch (id)
    {
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            keySync_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            keySync_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

        case CL_SM_GET_AUTH_DEVICE_RAW_CFM:
            keySync_HandleClSmGetAuthDeviceRawConfirm((CL_SM_GET_AUTH_DEVICE_RAW_CFM_T*)message);
            break;
        case CL_SM_ADD_AUTH_DEVICE_CFM:
            keySync_HandleClSmAddAuthDeviceConfirm((CL_SM_ADD_AUTH_DEVICE_CFM_T*)message);
            break;

        default:
            break;
    }
}

bool KeySync_Init(Task init_task)
{
    key_sync_task_data_t *ks = keySync_GetTaskData();

    UNUSED(init_task);

    DEBUG_LOG_ALWAYS("KeySync_Init");

    /* Initialise component task data */
    memset(ks, 0, sizeof(*ks));
    ks->task.handler = keySync_HandleMessage;

    /* Register with peer signalling to use the key sync msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(keySync_GetTask(), 
                                               PEER_SIG_MSG_CHANNEL_KEY_SYNC,
                                               key_sync_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
    return TRUE;
}

static void keySync_FindHandsetsAndCheckForSync(device_t device, void *data)
{
    uint16 flags = 0;
    typed_bdaddr handset_taddr = {0};

    UNUSED(data);
    
    DEBUG_LOG("KeySync_Sync Device %p", device);

    if (BtDevice_GetDeviceType(device) != DEVICE_TYPE_HANDSET)
    {
        DEBUG_LOG("KeySync_Sync Device not DEVICE_TYPE_HANDSET");
        return;
    }

    if(!Device_GetPropertyU16(device, device_property_flags, &flags))
    {
        DEBUG_LOG("KeySync_Sync No flags property");
        return;
    }
    
    if((flags & DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD) != DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD)
    {
        DEBUG_LOG("KeySync_Sync DEVICE_FLAGS_HANDSET_ADDRESS_FORWARD_REQD not set");
        return;
    }
    
    handset_taddr.addr = DeviceProperties_GetBdAddr(device);
    handset_taddr.type = TYPED_BDADDR_PUBLIC;

    DEBUG_LOG_ALWAYS("KeySync_Sync found key to sync, handset bd_addr [0x%04x,0x%02x,0x%06lx]",
                      handset_taddr.addr.nap, handset_taddr.addr.uap, handset_taddr.addr.lap);
    ConnectionSmGetAuthDeviceRawRequest(keySync_GetTask(), &handset_taddr);
}

void KeySync_Sync(void)
{
    DEBUG_LOG("KeySync_Sync");
    DeviceList_Iterate(keySync_FindHandsetsAndCheckForSync, NULL);
}
