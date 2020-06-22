/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_account_key_sync.c
\defgroup   fast_pair
\brief      Component handling synchronization of fast pair account keys between peers 

*/

#include "fast_pair_account_key_sync.h"
#include "fast_pair_session_data.h"
#include "fast_pair.h"
#include "device_properties.h"

#include <marshal_common.h>
#include <marshal.h>
#include <peer_signalling.h>
#include <bt_device.h>
#include <device_list.h>
#include <task_list.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <stdio.h>
/*! Global Instance of Account Key Sync Task Data */
fp_account_key_sync_task_data_t account_key_sync;
/*!Definition of marshalled messages used by Account Key Sync. */
const marshal_member_descriptor_t fp_account_key_sync_req_member_descriptors[] =
{
    MAKE_MARSHAL_MEMBER_ARRAY(fast_pair_account_key_sync_req_t, uint16, account_key_index, MAX_FAST_PAIR_ACCOUNT_KEYS),
    MAKE_MARSHAL_MEMBER_ARRAY(fast_pair_account_key_sync_req_t, uint16, account_keys, ACCOUNT_KEY_DATA_LENTH),
};

const marshal_type_descriptor_t marshal_type_descriptor_fast_pair_account_key_sync_req_t =
    MAKE_MARSHAL_TYPE_DEFINITION(fast_pair_account_key_sync_req_t, fp_account_key_sync_req_member_descriptors);

const marshal_type_descriptor_t marshal_type_descriptor_fast_pair_account_key_sync_cfm_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(fast_pair_account_key_sync_cfm_t));

/*! X-Macro generate account key sync marshal type descriptor set that can be passed to a (un)marshaller to initialise it.
 */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const fp_account_key_sync_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION

/*! \brief Send the marshalled data to the peer
 */
static void fastPair_AccountKeySync_SendMarshalledData(fast_pair_account_key_sync_req_t *sync_data)
{
    bdaddr peer_addr;

    if(appDeviceGetPeerBdAddr(&peer_addr))
    {
        DEBUG_LOG_V_VERBOSE("fastPair_AccountKeySync_SendMarshalledData. Account Key Index Info");
        for(uint8 index = 0; index < MAX_FAST_PAIR_ACCOUNT_KEYS; index++)
        {
            uint16 temp = sync_data->account_key_index[index];
            DEBUG_LOG_V_VERBOSE("%02x", temp);
        }
        DEBUG_LOG_V_VERBOSE("fastPair_AccountKeySync_SendMarshalledData : Account Keys Info");
        for(uint8 account_key =0; account_key < ACCOUNT_KEY_DATA_LENTH; account_key++)
        {
            uint16 temp = sync_data->account_keys[account_key];
            DEBUG_LOG_V_VERBOSE("%02x", temp);
        }
        
        DEBUG_LOG_DEBUG("fastPair_AccountKeySync_SendMarshalledData. Send Marshalled Data to the peer.");
        /*! send the account key index and account keys to counterpart on other earbud */
        appPeerSigMarshalledMsgChannelTx(fpAccountKeySync_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_FP_ACCOUNT_KEY_SYNC,
                                         sync_data, MARSHAL_TYPE_fast_pair_account_key_sync_req_t);
    }
    else
    {
        DEBUG_LOG_DEBUG("fastPair_AccountKeySync_SendMarshalledData. No Peer to send to.");
    }
}

/*! \brief Send the confirmation of synchronization to primary device
 */
static void fastPair_AccountKeySync_SendConfirmation(bool synced)
{
    bdaddr peer_addr;

    if (appDeviceGetPeerBdAddr(&peer_addr))
    {
        fast_pair_account_key_sync_cfm_t* cfm = PanicUnlessMalloc(sizeof(fast_pair_account_key_sync_cfm_t));
        cfm->synced = synced;
        DEBUG_LOG("fastPair_AccountKeySync_SendConfirmation. Send confirmation to the peer.");
        /*! send confirmation of account key received */
        appPeerSigMarshalledMsgChannelTx(fpAccountKeySync_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_FP_ACCOUNT_KEY_SYNC,
                                         cfm, MARSHAL_TYPE_fast_pair_account_key_sync_cfm_t);
    }
    else
    {
        DEBUG_LOG("fastPair_AccountKeySync_SendConfirmation. No Peer to send to.");
    }
}

/*! \brief Handle confirmation of transmission of a marshalled message.
 */
static void fastPair_AccountKeySync_HandleMarshalledMsgChannelTxCfm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    DEBUG_LOG("fastPair_AccountKeySync_HandleMarshalledMsgChannelTxCfm channel %u status %u", cfm->channel, cfm->status);
}

/*! \brief Handle incoming marshalled messages from peer account key sync component.
 */
static void fastPair_AccountKeySync_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE_fast_pair_account_key_sync_req_t:
        {
            bool synced;
            fast_pair_account_key_sync_req_t* req = (fast_pair_account_key_sync_req_t*)ind->msg;
            DEBUG_LOG("fastPair_AccountKeySync_HandleMarshalledMsgChannelRxInd RX Account Key ");
            /*! Store the Account keys and send the confirmation to the peer */
            synced = fastPair_StoreAllAccountKeys(req);
            fastPair_AccountKeySync_SendConfirmation(synced);
            free(req);
        }
        break;

        case MARSHAL_TYPE_fast_pair_account_key_sync_cfm_t:
        {
            fast_pair_account_key_sync_cfm_t *cfm = (fast_pair_account_key_sync_cfm_t*)ind->msg;
            if(!cfm->synced)
            {
                DEBUG_LOG("fastPair_AccountKeySync_HandleMarshalledMsgChannelRxInd. Failed to Synchronize.");
            }
            else
            {
                DEBUG_LOG("fastPair_AccountKeySync_HandleMarshalledMsgChannelRxInd. Synchronized successfully.");
            }
            free(cfm);
        }
        break;

        default:
            break;
    }
}

/*!\brief Fast Pair Account Key Sync Message Handler
 */
static void fastPair_AccountKeySync_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
            /* marshalled messaging */
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
            DEBUG_LOG("fastPair_AccountKeySync_HandleMessage. PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND");
            fastPair_AccountKeySync_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
            break;
        case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
            DEBUG_LOG("fastPair_AccountKeySync_HandleMessage. PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM");
            fastPair_AccountKeySync_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
            break;

        default:
            break;
    }
}

/*! \brief Fast Pair Account Key Sync Initialization
 */
void fastPair_AccountKeySync_Init(void)
{
    DEBUG_LOG("fastPair_AccountKeySync_Init");
    fp_account_key_sync_task_data_t *key_sync = fpAccountKeySync_GetTaskData();

    /* Initialize component task data */
    memset(key_sync, 0, sizeof(*key_sync));
    key_sync->task.handler = fastPair_AccountKeySync_HandleMessage;

    /* Register with peer signalling to use the account key sync msg channel */
    appPeerSigMarshalledMsgChannelTaskRegister(fpAccountKeySync_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_FP_ACCOUNT_KEY_SYNC,
                                               fp_account_key_sync_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);
    DEBUG_LOG("fastPair_AccountKeySync_Init. Initialized successfully. ");
}

/*! \brief Fast Pair Account Key Synchronization API
 */
void fastPair_AccountKeySync_Sync(void)
{
    DEBUG_LOG("fastPair_AccountKeySync_Sync. Synchronization starts.");
    deviceType type = DEVICE_TYPE_SELF;
    device_t my_device = DeviceList_GetFirstDeviceWithPropertyValue(device_property_type, &type, sizeof(deviceType));
    void *account_key_index_value = NULL;
    void *account_keys_value = NULL;
    size_t account_key_index_size = 0;
    size_t account_keys_size = 0;

    if(my_device)
    {
        if(Device_GetProperty(my_device, device_property_fast_pair_account_key_index, &account_key_index_value, &account_key_index_size) &&
            Device_GetProperty(my_device, device_property_fast_pair_account_keys, &account_keys_value, &account_keys_size))
        {
            fast_pair_account_key_sync_req_t *sync_data = PanicUnlessMalloc(sizeof(fast_pair_account_key_sync_req_t));
            uint16 * temp = (uint16 *)account_key_index_value;
            for(uint8 i = 0; i < MAX_FAST_PAIR_ACCOUNT_KEYS; i++)
            {
                sync_data->account_key_index[i] = temp[i];
            }
            memcpy(sync_data->account_keys, account_keys_value, sizeof(sync_data->account_keys));
            fastPair_AccountKeySync_SendMarshalledData(sync_data);
        }
        else
        {
            DEBUG_LOG("fastPair_AccountKeySync_Sync. Should not reach here.Unexpected Data.");
        }
    }
    else
    {
        DEBUG_LOG("fastPair_AccountKeySync_Sync. SELF device does not exist.");
    }
}
