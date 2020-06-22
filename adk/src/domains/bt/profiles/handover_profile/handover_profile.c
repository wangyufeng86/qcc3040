/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       handover_profile.c
\brief      Implementation of the Handover Profile public APIs.
*/

#ifdef INCLUDE_MIRRORING

#include "handover_profile.h"
#include "handover_profile_private.h"
#include "init.h"
#include "sdp.h"
#include "bt_device.h"
#include "acl.h"
#include "mirror_profile_protected.h"
#include "timestamp_event.h"

#include <app/bluestack/mdm_prim.h>
#include <panic.h>
#include <message_broker.h>
#include <logging.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static void handoverProfile_HandleMessage(Task task, MessageId id, Message message);

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/
/*! Check if the state is Disconnecting and message is NOT one of the 
    connection library L2CAP disconnect messages (CL_L2CAP_DISCONNECT_CFM or
    CL_L2CAP_DISCONNECT_IND. */
#define handoverProfile_IsDisconnecting(id, ho_inst) \
    (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_DISCONNECTING \
     && (id) != CL_L2CAP_DISCONNECT_CFM \
     && (id) != CL_L2CAP_DISCONNECT_IND \
     && (id) != CL_L2CAP_CONNECT_CFM ? TRUE: FALSE)

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

/******************************************************************************
 * Global and Local Declarations
 ******************************************************************************/
/* Handover Profile task data. */
handover_profile_task_data_t ho_profile;

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! 
    \brief Handover Profile task message handler

    Handles all the messages sent to the handover profile task.

    \param[in] task      Task data
    \param[in] id        Message ID \ref MessageId.
    \param[in] message   Message.

*/
static void handoverProfile_HandleMessage(Task task,
    MessageId id,
    Message message)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    UNUSED(task);

    DEBUG_LOG("handoverProfile_HandleMessage Message Id=0x%x", id);

    /* If the state is Disconnecting, then reject all messages except
        CL_L2CAP_DISCONNECT_CFM.*/
    if(handoverProfile_IsDisconnecting(id, ho_inst))
    {
        DEBUG_LOG("handoverProfile_HandleMessage handoverProfile_IsDisconnecting dropping id=ox%s", id);
        return;
    }

    switch (id)
    {
        /* Connection library messages */
        case CL_L2CAP_REGISTER_CFM:
            HandoverProfile_HandleClL2capRegisterCfm((const CL_L2CAP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_REGISTER_CFM:
            HandoverProfile_HandleClSdpRegisterCfm((const CL_SDP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM:
            HandoverProfile_HandleClSdpServiceSearchAttributeCfm((const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
            return;

        case CL_L2CAP_CONNECT_IND:
            HandoverProfile_HandleL2capConnectInd((const CL_L2CAP_CONNECT_IND_T *)message);
            break;

        case CL_L2CAP_CONNECT_CFM:
            HandoverProfile_HandleL2capConnectCfm((const CL_L2CAP_CONNECT_CFM_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_IND:
            HandoverProfile_HandleL2capDisconnectInd((const CL_L2CAP_DISCONNECT_IND_T *)message);
            break;

        case CL_L2CAP_DISCONNECT_CFM:
            HandoverProfile_HandleL2capDisconnectCfm((const CL_L2CAP_DISCONNECT_CFM_T *)message);
            break;

        case MESSAGE_MORE_DATA:
            HandoverProfile_ProcessHandoverMessage((const MessageMoreData *)message);
            break;

        /* Internal Handover Profile Messages */
        case HANDOVER_PROFILE_INTERNAL_STARTUP_REQ:
            HandoverProfile_HandleInternalStartupRequest((HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T *)message);
            break;

        case HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ:
            HandoverProfile_HandleInternalShutdownReq();
            break;
        
        default:
            DEBUG_LOG("handoverProfile_HandleMessage Unhandled message 0x%04x",id);
            break;
    }
}

/******************************************************************************
 * Global Function Definitions
 ******************************************************************************/
/*! 
    \brief Check if the role for peer ACL link is changed to the paramter 
    'role'.

    \param[in] role         ACL role to check with.
    \param[in] timeout_ms   Time in milliseconds to enqire.

    \return TRUE If the role for the peer Acl link is same as parameter 'role'
            FALSE otherwise.

*/
bool HandoverProfile_GetAclRoleBlocking(const tp_bdaddr * acl_address, hci_role_t role, uint32 timeout_ms)
{
    uint32 timeout;

    timeout = VmGetClock() + timeout_ms;
    do
    {
        if (VmGetAclRole(acl_address) == role)
        {
            return TRUE;
        }
    } while(VmGetClock() < timeout);

    return FALSE;
}

/*! \brief Initialise the handover profile.

    Called at start up to initialise the handover profile task

    \param[in] init_task   Task to send confirmation message to.

    \return TRUE: Post the initialization of the task.
*/
bool HandoverProfile_Init(Task init_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    /* Set task's message handler */
    ho_inst->task.handler = handoverProfile_HandleMessage;

    /* Initialise the state machine */
    ho_inst->local_psm = 0;
    ho_inst->remote_psm = 0;
    ho_inst->state = HANDOVER_PROFILE_STATE_NONE;
    ho_inst->is_primary = FALSE;
    ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
    ho_inst->acl_handle = 0xFFFF;
    ho_inst->sink_written = 0;
    ho_inst->marshal_sink = 0;
    ho_inst->session_id = 0;
    ho_inst->sdp_search_attempts = 0;
    TaskList_Initialise(&ho_inst->handover_client_tasks);
    /* Move to 'initialising' state */
    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_INITIALISING);
    Init_SetInitTask(init_task);
    return TRUE;
}

/*! \brief Register to receive peer signalling notifications.
    \param[in]  client_task Task to send notification.
*/
void HandoverProfile_ClientRegister(Task client_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    TaskList_AddTask(&ho_inst->handover_client_tasks, client_task);
}

/*! \brief Unregister to stop receiving peer signalling notifications.
    \param[in]  client_task Task to send notification.
*/
void HandoverProfile_ClientUnregister(Task client_task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    TaskList_RemoveTask(&ho_inst->handover_client_tasks, client_task);
}

/*! 
    \brief Register handover profile clients. Refer handover_profile.h for 
           more details.

*/
bool HandoverProfile_RegisterHandoverClients(const handover_interface **clients, uint8 clients_size)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    if(ho_inst->state == HANDOVER_PROFILE_STATE_NONE)
    {
        DEBUG_LOG("HandoverProfile_RegisterHandoverClients failed, ho_inst->state=0x%x. Panic!!", ho_inst->state);
        Panic();
    }

    if(clients && clients_size)
    {
        ho_inst->ho_clients = clients;
        ho_inst->num_clients =  clients_size;
        return TRUE;
    }
    DEBUG_LOG("HandoverProfile_RegisterHandoverClients, failed");
    return FALSE;
}

/*! \brief Create L2CAP channel to the Peer earbud.

    SDP search for Handover PSM and create L2CAP channel with the Peer earbud.

    A HANDOVER_PROFILE_CONNECT_CFM message shall be sent with status 

    HANDOVER_PROFILE_STATUS_PEER_CONNECT_FAILED: If the connection fails.
    HANDOVER_PROFILE_STATUS_SUCCESS: This status is also sent with 
          HANDOVER_PROFILE_CONNECTION_IND message to all registered client post 
          succesful connection.

    \param[in]  task        Task to send confirmation message to.
    \param[in]  peer_addr   Address of peer earbud.

    \return None.
*/
void HandoverProfile_Connect(Task task, const bdaddr *peer_addr)
{
    if(peer_addr)
    {
        handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

        DEBUG_LOG("HandoverProfile_Connect - startup");

        ho_inst->is_primary = TRUE;

        /* Store peer device BD-Addr */
        memcpy(&ho_inst->peer_addr, peer_addr, sizeof(bdaddr));

        /* Send internal message to enter connecting state */
        HandoverProfile_Startup(task, peer_addr);
    }
    else
    {
        DEBUG_LOG("HandoverProfile_Connect - Peer address is NULL");
        Panic();
    }
}

/*! \brief Distroy L2CAP channel to the Peer earbud if exists.

    Post disconnection the HANDOVER_PROFILE_DISCONNECT_CFM message is sent with status 
    HANDOVER_PROFILE_STATUS_SUCCESS

    \param[in]  task        Task to send confirmation message to.
*/
void HandoverProfile_Disconnect(Task task)
{
    DEBUG_LOG("HandoverProfile_Disconnect");
    HandoverProfile_Shutdown(task);
}

/*! \brief Handle MDM_SET_BREDR_SLAVE_ADDRESS_IND

    \param[in] ind      MDM_SET_BREDR_SLAVE_ADDRESS_IND message 

*/
void HandoverProfile_HandleMdmSetBredrSlaveAddressInd(const MDM_SET_BREDR_SLAVE_ADDRESS_IND_T *ind)
{
    DEBUG_LOG("HandoverProfile_HandleMdmSetBredrSlaveAddressInd MDM_SET_BREDR_SLAVE_ADDRESS_IND old addr=%04x:%02x:%06x, new addr=%04x:%02x:%06x, flags=0x%x", 
                ind->old_bd_addr.nap, ind->old_bd_addr.uap, ind->old_bd_addr.lap, ind->new_bd_addr.nap, ind->new_bd_addr.uap, ind->new_bd_addr.lap, ind->flags);

    /* Do nothing */
}

/*! 
    \brief Starts the handover procedure as per the local device role.
           Refer handover_profile.h for more details.

    \param[in] remote_addr    Bluetooth address of the handset.

    \return handover_profile_status_t Status of the handover operation.
                                      Refer \ref handover_profile_status_t.
                                      See handover_profile.h for more details.

*/
handover_profile_status_t HandoverProfile_Handover(const tp_bdaddr *remote_addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    if(!remote_addr)
    {
        DEBUG_LOG("HandoverProfile_Handover NULL handset address passed.Panic");
        Panic();
    }

    /* Check if peer connection exists */
    if(ho_inst->state != HANDOVER_PROFILE_STATE_CONNECTED)
    {
        DEBUG_LOG("HandoverProfile_Handover Peer Disconnected!!");
        return HANDOVER_PROFILE_STATUS_PEER_DISCONNECTED;
    }

    memcpy(&ho_inst->tp_handset_addr, remote_addr, sizeof(tp_bdaddr));

    DEBUG_LOG("HandoverProfile_Handover ho_inst->tp_handset_addr %04x:%02x:%06x, type=0x%x, transport=0x%x",
                        ho_inst->tp_handset_addr.taddr.addr.nap, ho_inst->tp_handset_addr.taddr.addr.uap, ho_inst->tp_handset_addr.taddr.addr.lap,
                        ho_inst->tp_handset_addr.taddr.type,
                        ho_inst->tp_handset_addr.transport);

    /* If current role is Primary */
    if(ho_inst->is_primary)
    {
        handover_profile_status_t ret;
        tp_bdaddr peer_tp_addr;
        uint16 src_size=0;

        /* Empty the source */
        src_size = SourceSize(ho_inst->link_source);
        if(src_size)
        {
            DEBUG_LOG("HandoverProfile_Handover dropping stale %d bytes from source", src_size);
            SourceDrop(ho_inst->link_source, src_size);
        }

        TimestampEvent(TIMESTAMP_EVENT_PRI_HANDOVER_STARTED);

        /* Check if any of P1 clients Vetos */
        if(HandoverProfile_VetoP1Clients())
        {
            DEBUG_LOG("HandoverProfile_Handover P1 Client veto'ed before start request sent to peer");
            return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
        }

        ho_inst->session_id++;

        /* Send HANDOVER_START_REQ to peer to kick start the handover at peer */
        if((ret = HandoverProfile_SendHandoverStartReq(&remote_addr->taddr.addr, ho_inst->session_id)) != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            DEBUG_LOG("HandoverProfile_Handover failed to send HANDOVER_START_REQ to Peer");
            return ret;
        }

        /* Block on peer stream to receive HANDOVER_START_CFM */
        if((ret = HandoverProfile_ProcessProtocolStartCfm())  != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            DEBUG_LOG("HandoverProfile_Handover HandoverProfile_ProcessProtocolStartCfm wasn't success. ret:%d", ret);
            return ret;
        }
        ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_P1_MARSHALLING;

        /* Marshal P1 data */
        ret = HandoverProfile_MarshalP1Clients(&ho_inst->tp_handset_addr);
        if(ret != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            if(ret == HANDOVER_PROFILE_STATUS_HANDOVER_TIMEOUT && 
                (HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS))
            {
                DEBUG_LOG("HandoverProfile_Handover Failed to send Handover Cancel Ind");
            }
            HandoverProfile_AbortP1Clients();
            DEBUG_LOG("HandoverProfile_Handover HandoverProfile_Marshal_P1_Clients timeout/failed");
            ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
            return ret;
        }

        /* Block for P1 unmarshal confirmation from peer */
        if((ret = HandoverProfile_ProcessProtocolUnmarshalP1Cfm())  != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            if(HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS)
            {
                DEBUG_LOG("HandoverProfile_Handover Failed to send Handover Cancel Ind");
            }
            DEBUG_LOG("HandoverProfile_Handover Failed to receive Unmarshal P1 confimration message");
            ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
            HandoverProfile_AbortP1Clients();
            return ret;
        }

        /* Check if any of P1 clients Vetos to avoid flow off->flow on which may cause an audio glitch */
        if(HandoverProfile_VetoP1Clients())
        {
            if(HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS)
            {
                DEBUG_LOG("HandoverProfile_Handover Failed to send Handover Cancel Ind");
            }
            DEBUG_LOG("HandoverProfile_Handover P1 Client veto'ed before AclPrepare");
            ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
            HandoverProfile_AbortP1Clients();
            return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
        }

        /* Request for power performance */
        appPowerPerformanceProfileRequest();

        /* Prepare for Marshal */
        if((ret=HandoverProfile_PrepareForMarshal(&ho_inst->tp_handset_addr)) != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            /* 1. Send Cancel_Ind to peer.
               2. Re-enter sniff mode.
               3. Enable Rx.i.e., AclReceiveEnable(TRUE).
                  At this stage secondary needs to panic if BTSS data is unmarshalled.
            */
            HandoverProfile_HandlePrepareForMarshalFailure(&ho_inst->tp_handset_addr);
            DEBUG_LOG("HandoverProfile_Handover Prepare for Marshal failed. ret:%d", ret);
            return ret;
        }

        HandoverPioSet();
        /* Check if any of P1 clients Vetos. ACL-C traffic could arrive between last P1 veto check and AclHandoverPrepare */
        if(HandoverProfile_VetoP1Clients())
        {
            HandoverProfile_HandleMarshalFailure(&ho_inst->tp_handset_addr);
            DEBUG_LOG("HandoverProfile_Handover Vetoed by P1 after AclHandoverPrepare succeeded");
            HandoverPioClr();
            return HANDOVER_PROFILE_STATUS_HANDOVER_VETOED;
        }
        HandoverPioClr();

        ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_P0_MARSHALLING;

        /* Marshal P0 data */
        ret = HandoverProfile_MarshalP0Clients(&ho_inst->tp_handset_addr);
        if(ret != HANDOVER_PROFILE_STATUS_SUCCESS)
        {
            /* 1. Send Cancel Ind to peer.
               2. Request handover cancel to bluestack/BTSS. i.e.,AclHandoverCancel()
                  At this stage should secondary need to panic.
               3. Re-enter sniff mode.
               4. Enable Rx. i.e., AclReceiveEnable(TRUE)
            */
            HandoverProfile_HandleMarshalFailure(&ho_inst->tp_handset_addr);
            DEBUG_LOG("HandoverProfile_Handover HandoverProfile_Marshal_P0_Clients timeout/failed");
            return ret;
        }

        HandoverPioSet();
        BdaddrTpFromBredrBdaddr(&peer_tp_addr, &ho_inst->peer_addr);

        /*Make sure P0 data has been successfully transmitted to Secondary.
          Using AclTransmitDataPending API to check the status of transmitted data. AclTransmitDataPending
          uses NCP (number of completed packets) to confirm whether data has been successfully transmitted
          from the controller.
          
          Post P0 data transfer, both devices will commit to the new role by making necessary updates
          to the P0 and P1 data. Any failure after this transfer hence will be irrecoverable and will cause panic. 
        */
        if(HandoverProfile_IsAclTransmitPending(&peer_tp_addr, HANDOVER_PROFILE_P0_TRANSMIT_DATA_PENDING_TIMEOUT_MSEC))
        {
            DEBUG_LOG("HandoverProfile_IsAclTransmitPending timeout");
        }

        HandoverPioClr();

        /* Commit P1 Clients */
        HandoverProfile_CommitP1Clients(FALSE);

        HandoverPioSet();

        /* Commit P0 clients */
        if(!AclHandoverCommit(ho_inst->acl_handle))
        {
            DEBUG_LOG("HandoverProfile_Handover AclHandoverCommit() failed at Primary, acl handle=0x%x. Panic!!", ho_inst->acl_handle);
            /* Send HANDOVER_PROTOCOL_CANCEL_IND to secondary device and Panic */
            if(HandoverProfile_SendHandoverCancelInd() != HANDOVER_PROFILE_STATUS_SUCCESS)
            {
                DEBUG_LOG("HandoverProfile_Handover handoverProfile_SendHandoverCancelInd failed. Panic!!");
            }
            Panic();
        }

        HandoverPioClr();

        if (!MirrorProfile_WaitForPeerLinkMode(lp_sniff, HANDOVER_PROFILE_REENTER_SNIFF_TIMEOUT_MSEC))
        {
            DEBUG_LOG("HandoverProfile_Handover timeout waiting to re-enter sniff mode");
        }

        TimestampEvent(TIMESTAMP_EVENT_PRI_HANDOVER_COMPLETED);

        /* Call P1 complete() */
        HandoverProfile_CompleteP1Clients(!ho_inst->is_primary);

        ho_inst->marshal_state = HANDOVER_PROFILE_MARSHAL_STATE_IDLE;
        ho_inst->is_primary = FALSE;

        /* Relinquish power performance */
        appPowerPerformanceProfileRelinquish();

        /* Update the new peer address */
        {
            bdaddr bd_addr_peer;
            if(appDeviceGetPeerBdAddr(&bd_addr_peer))
            {
                memcpy(&ho_inst->peer_addr, &bd_addr_peer, sizeof(bdaddr));
            }
        }

        DEBUG_LOG("HandoverProfile_Handover Handover Complete. I am new Secondary.");

        return HANDOVER_PROFILE_STATUS_SUCCESS;
    }
    DEBUG_LOG("HandoverProfile_Handover Failed as instance was not primary");
    return HANDOVER_PROFILE_STATUS_HANDOVER_FAILURE;
}

#endif /* INCLUDE_MIRRORING */
