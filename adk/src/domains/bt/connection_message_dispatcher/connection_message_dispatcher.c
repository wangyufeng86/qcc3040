/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Implementation of the module distributing messages from the connection library.
*/

#include "connection_message_dispatcher.h"

#include <logging.h>
#include <panic.h>
#include <vmtypes.h>

#include <stdlib.h>
#include <connection.h>


/* ------------------------------ Definitions ------------------------------ */

/*! \brief Connection library message groups */
typedef enum
{
    inquiry_group = 0,
    acl_group,
    init_group,
    crypto_group,
    csb_group,
    le_group,
    tdl_group,
    l2cap_group,
    local_device_group,
    pairing_group,
    link_policy_group,
    test_group,
    remote_connection_group,
    rfcomm_group,
    sco_group,
    sdp_group,
    misc_group,
    unknown_group
} group_type_t;

#define MAX_NUMBER_OF_GROUPS unknown_group

/* ------------------------------ Global Variables ------------------------------ */

/** Encapsulation of connection_message_dispatcher state */
static struct connection_message_dispatcher
{
    /* The module's task */
    TaskData task;
    /* Array of registered clients */
    Task registered_clients[MAX_NUMBER_OF_GROUPS];

} connection_message_dispatcher;


/* ------------------------------ Static function prototypes ------------------------------ */

static void connectionMessageDispatcher_HandleMessage(Task task, MessageId id, Message message);
static Task connectionMessageDispatcher_RegisterClient(Task task, group_type_t group_id);
static void connectionMessageDispatcher_ResetRegisteredClients(void);
static group_type_t connectionMessageDispatcher_GetGroupType(MessageId id);

/* ------------------------------ API functions start here ------------------------------ */

void ConnectionMessageDispatcher_Init(void)
{
    DEBUG_LOG("ConnectionMessageDispatcher_Init called.");

    connectionMessageDispatcher_ResetRegisteredClients();

    connection_message_dispatcher.task.handler = connectionMessageDispatcher_HandleMessage;
}

Task ConnectionMessageDispatcher_GetHandler(void)
{
    return &connection_message_dispatcher.task;
}

Task ConnectionMessageDispatcher_RegisterInquiryClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, inquiry_group);
}

Task ConnectionMessageDispatcher_RegisterAclClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, acl_group);
}

Task ConnectionMessageDispatcher_RegisterInitClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, init_group);
}

Task ConnectionMessageDispatcher_RegisterCryptoClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, crypto_group);
}

Task ConnectionMessageDispatcher_RegisterCsbClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, csb_group);
}

Task ConnectionMessageDispatcher_RegisterLeClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, le_group);
}

Task ConnectionMessageDispatcher_RegisterTdlClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, tdl_group);
}

Task ConnectionMessageDispatcher_RegisterL2capClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, l2cap_group);
}

Task ConnectionMessageDispatcher_RegisterLocalDeviceClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, local_device_group);
}

Task ConnectionMessageDispatcher_RegisterPairingClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, pairing_group);
}

Task ConnectionMessageDispatcher_RegisterLinkPolicyClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, link_policy_group);
}

Task ConnectionMessageDispatcher_RegisterTestClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, test_group);
}

Task ConnectionMessageDispatcher_RegisterRemoteConnectionClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, remote_connection_group);
}

Task ConnectionMessageDispatcher_RegisterRfcommClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, rfcomm_group);
}

Task ConnectionMessageDispatcher_RegisterScoClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, sco_group);
}

Task ConnectionMessageDispatcher_RegisterSdpClient(Task client)
{
    return connectionMessageDispatcher_RegisterClient(client, sdp_group);
}

/* ------------------------------ Static functions start here ------------------------------ */

/*! \brief Distributes messages to all registered clients.
    \param  Pointer to task.
    \param  Message ID.
    \param  Message.
*/
static void connectionMessageDispatcher_HandleMessage(Task task, MessageId id, Message message)
{
    group_type_t group_id = connectionMessageDispatcher_GetGroupType(id);

    if (group_id != unknown_group)
    {
        Task handling_task = connection_message_dispatcher.registered_clients[group_id];
        if (handling_task)
        {
            PanicFalse(handling_task->handler != NULL);
            handling_task->handler(task, id, message);
        }
    }
}

/*! \brief Register a client task to the handler array (set to NULL to unregister).
    \param client Client task.
    \param group_id Group ID.
    \return Previously registered task (may be NULL if previously unregistered).
*/
static Task connectionMessageDispatcher_RegisterClient(Task new_client, group_type_t group_id)
{
    Task old_client;

    PanicFalse(group_id < MAX_NUMBER_OF_GROUPS);

    old_client = connection_message_dispatcher.registered_clients[group_id];
    connection_message_dispatcher.registered_clients[group_id] = new_client;

    DEBUG_LOG_VERBOSE("connectionMessageDispatcher_RegisterClient group=%d, old_client=%p, new_client=%p",
                    group_id, old_client, new_client);

    return old_client;
}

/*! \brief Resets the array containing the registered client tasks */
static void connectionMessageDispatcher_ResetRegisteredClients(void)
{
    unsigned index;

    for (index = 0; index < MAX_NUMBER_OF_GROUPS; index++)
    {
        connection_message_dispatcher.registered_clients[index] = NULL;
    }
}

/*! \brief Matches a message id from the connection library to its group.
    \param  Message ID.
    \return Group ID.
*/
static group_type_t connectionMessageDispatcher_GetGroupType(MessageId id)
{
    group_type_t group;

    switch (id)
    {
        case CL_DM_READ_INQUIRY_TX_CFM:
        case CL_DM_WRITE_INQUIRY_ACCESS_CODE_CFM:
        case CL_DM_READ_EIR_DATA_CFM:
        case CL_DM_WRITE_INQUIRY_MODE_CFM:
        case CL_DM_READ_INQUIRY_MODE_CFM:
        case CL_DM_INQUIRE_RESULT:
            group = inquiry_group;
            break;

        case CL_INIT_CFM:
            group = init_group;
            break;

        case CL_CRYPTO_GENERATE_PUBLIC_PRIVATE_KEY_CFM:
        case CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM:
        case CL_CRYPTO_ENCRYPT_CFM:
        case CL_CRYPTO_HASH_CFM:
        case CL_CRYPTO_DECRYPT_CFM:
            group = crypto_group;
            break;

        case CL_CSB_SET_RESERVED_LT_ADDR_CFM:
        case CL_CSB_DELETE_RESERVED_LT_ADDR_CFM:
        case CL_CSB_SET_CSB_CFM:
        case CL_CSB_WRITE_SYNC_TRAIN_PARAMETERS_CFM:
        case CL_CSB_START_SYNC_TRAIN_CFM:
        case CL_CSB_SYNC_TRAIN_RECEIVED_CFM:
        case CL_CSB_RECEIVE_CFM:
        case CL_CSB_TRANSMIT_TIMEOUT_IND:
        case CL_CSB_RECEIVE_TIMEOUT_IND:
        case CL_CSB_AFH_MAP_IND:
        case CL_CSB_CHANNEL_MAP_CHANGE_IND:
            group = csb_group;
            break;

        case CL_DM_BLE_READ_WHITE_LIST_SIZE_CFM:
        case CL_DM_BLE_CLEAR_WHITE_LIST_CFM:
        case CL_DM_BLE_ADD_DEVICE_TO_WHITE_LIST_CFM:
        case CL_DM_BLE_REMOVE_DEVICE_FROM_WHITE_LIST_CFM:
        case CL_DM_BLE_SET_ADVERTISING_DATA_CFM:
        case CL_DM_BLE_ADVERTISING_REPORT_IND:
        case CL_DM_BLE_SET_ADVERTISING_PARAMS_CFM:
        case CL_DM_BLE_ADVERTISING_PARAM_UPDATE_IND:
        case CL_DM_BLE_SET_SCAN_PARAMETERS_CFM:
        case CL_DM_BLE_SET_SCAN_RESPONSE_DATA_CFM:
        case CL_DM_BLE_SET_CONNECTION_PARAMETERS_CFM:
        case CL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_CFM:
        case CL_SM_BLE_LINK_SECURITY_IND:
        case CL_DM_BLE_CHANNEL_SELECTION_ALGORITHM_IND:
        case CL_DM_ULP_ENABLE_ZERO_SLAVE_LATENCY_CFM:
        case CL_DM_ULP_PHY_UPDATE_IND:
        case CL_DM_ULP_SET_PHY_CFM:
        case CL_DM_ULP_SET_DEFAULT_PHY_CFM:
        case CL_DM_BLE_CONNECTION_UPDATE_COMPLETE_IND:
        case CL_SM_BLE_READ_RANDOM_ADDRESS_CFM:
            group = le_group;
            break;

        case CL_SM_ADD_AUTH_DEVICE_CFM:
        case CL_SM_GET_AUTH_DEVICE_CFM:
        case CL_SM_GET_ATTRIBUTE_CFM:
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
        case CL_SM_SET_TRUST_LEVEL_CFM:
        case CL_SM_AUTH_DEVICE_DELETED_IND:
            group = tdl_group;
            break;

        case CL_L2CAP_REGISTER_CFM:
        case CL_L2CAP_UNREGISTER_CFM:
        case CL_L2CAP_CONNECT_CFM:
        case CL_L2CAP_TP_CONNECT_CFM:
        case CL_L2CAP_ADD_CREDIT_CFM:
        case CL_L2CAP_CONNECT_IND:
        case CL_L2CAP_TP_CONNECT_IND:
        case CL_L2CAP_DISCONNECT_CFM:
        case CL_L2CAP_DISCONNECT_IND:
        case CL_L2CAP_MAP_CONNECTIONLESS_CFM:
        case CL_L2CAP_MAP_CONNECTIONLESS_IND:
        case CL_L2CAP_UNMAP_CONNECTIONLESS_IND:
        case CL_L2CAP_TIMEOUT_IND:
            group = l2cap_group;
            break;

        case CL_DM_LOCAL_NAME_COMPLETE:
        case CL_DM_CLASS_OF_DEVICE_CFM:
        case CL_DM_READ_BT_VERSION_CFM:
        case CL_DM_LOCAL_BD_ADDR_CFM:
        case CL_DM_LOCAL_VERSION_CFM:
        case CL_DM_READ_TX_POWER_CFM:
            group = local_device_group;
            break;

        case CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND:
        case CL_SM_READ_LOCAL_OOB_DATA_CFM:
        case CL_SM_AUTHENTICATE_CFM:
        case CL_SM_IO_CAPABILITY_REQ_IND:
        case CL_SM_USER_CONFIRMATION_REQ_IND:
        case CL_SM_USER_PASSKEY_REQ_IND:
        case CL_SM_PIN_CODE_IND:
        case CL_SM_SEC_MODE_CONFIG_CFM:
        case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
        case CL_SM_KEYPRESS_NOTIFICATION_IND:
            group = pairing_group;
            break;

        case CL_DM_ROLE_IND:
        case CL_DM_ROLE_CFM:
            group = link_policy_group;
            break;

        case CL_DM_DUT_CFM:
        case CL_DM_MODE_CHANGE_EVENT:
            group = test_group;
            break;

        case CL_DM_RSSI_CFM:
        case CL_DM_RSSI_BDADDR_CFM:
        case CL_DM_REMOTE_FEATURES_CFM:
        case CL_SM_ENCRYPT_CFM:
        case CL_SM_ENCRYPTION_KEY_REFRESH_IND:
        case CL_DM_WRITE_APT_CFM:
        case CL_DM_READ_APT_CFM:
        case CL_DM_APT_IND:
        case CL_SM_ENCRYPTION_CHANGE_IND:
        case CL_DM_ACL_OPENED_IND:
        case CL_DM_ACL_CLOSED_IND:
        case CL_DM_ACL_CLOSE_CFM:
        case CL_DM_BLE_SECURITY_CFM:
        case CL_DM_LINK_SUPERVISION_TIMEOUT_IND:
        case CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM:
        case CL_DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND:
        case CL_DM_BLE_READ_ADVERTISING_CHANNEL_TX_POWER_CFM:
        case CL_SM_AUTHORISE_IND:
        case CL_SM_REMOTE_IO_CAPABILITY_IND:
        case CL_DM_SNIFF_SUB_RATING_IND:
        case CL_DM_REMOTE_NAME_COMPLETE:
        case CL_DM_LINK_QUALITY_CFM:
        case CL_DM_REMOTE_VERSION_CFM:
        case CL_DM_CLOCK_OFFSET_CFM:
        case CL_SM_REGISTER_OUTGOING_SERVICE_CFM:
        case CL_SM_SECURITY_LEVEL_CFM:
            group = remote_connection_group;
            break;

        case CL_RFCOMM_REGISTER_CFM:
        case CL_RFCOMM_UNREGISTER_CFM:
        case CL_RFCOMM_CLIENT_CONNECT_CFM:
        case CL_RFCOMM_CONNECT_IND:
        case CL_RFCOMM_DISCONNECT_CFM:
        case CL_RFCOMM_DISCONNECT_IND:
        case CL_RFCOMM_PORTNEG_CFM:
        case CL_RFCOMM_PORTNEG_IND:
        case CL_RFCOMM_CONTROL_CFM:
        case CL_RFCOMM_CONTROL_IND:
        case CL_RFCOMM_LINE_STATUS_CFM:
        case CL_RFCOMM_LINE_STATUS_IND:
        case CL_RFCOMM_SERVER_CONNECT_CFM:
            group = rfcomm_group;
            break;

        case CL_DM_SYNC_REGISTER_CFM:
        case CL_DM_SYNC_UNREGISTER_CFM:
        case CL_DM_SYNC_CONNECT_CFM:
        case CL_DM_SYNC_CONNECT_IND:
        case CL_DM_SYNC_DISCONNECT_IND:
        case CL_DM_SYNC_RENEGOTIATE_IND:
            group = sco_group;
            break;

        case CL_SDP_REGISTER_CFM:
        case CL_SDP_UNREGISTER_CFM:
        case CL_SDP_OPEN_SEARCH_CFM:
        case CL_SDP_CLOSE_SEARCH_CFM:
        case CL_SDP_SERVICE_SEARCH_CFM:
        case CL_SDP_SERVICE_SEARCH_REF_CFM:
        case CL_SDP_ATTRIBUTE_SEARCH_CFM:
        case CL_SDP_ATTRIBUTE_SEARCH_REF_CFM:
        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_REF_CFM:
            group = sdp_group;
            break;

        case CL_SM_INIT_CFM:
        case CL_DM_LINK_POLICY_IND:
        case CL_SM_AUTH_REPLACE_IRK_CFM:
        case CL_DM_REMOTE_EXTENDED_FEATURES_CFM:
            DEBUG_LOG("connectionMessageDispatcher_GetGroupType, misc_group message, id = 0x%x (%d)", (id), (id));
            group = misc_group;
            break;

        default:
            DEBUG_LOG("connectionMessageDispatcher_GetGroupType, unknown_group message, id = 0x%x (%d)", (id), (id));
            group = unknown_group;
            break;
    }

    return group;
}
