/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       handover_profile_peer_connection.c
\brief      Implementation of Handover Profile Connection with Peer device.
*/
#ifdef INCLUDE_MIRRORING
#include "handover_profile_private.h"
#include "handover_profile.h"
#include "sdp.h"
#include "init.h"

#include <bt_device.h>
#include <panic.h>
#include <logging.h>
#include <service.h>
#include <stream.h>
#include <task_list.h>
#include <message.h>
#include <bluestack/l2cap_prim.h>


/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/

/*! Macro to make a message. */
#define handoverProfile_IsConnected(ho_inst)  (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_CONNECTED)
#define handoverProfile_IsDisconnected(ho_inst)  (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_DISCONNECTED)
#define handoverProfile_IsConnecting(ho_inst) (HandoverProfile_GetState(ho_inst) >= HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH && \
                                                HandoverProfile_GetState(ho_inst) <= HANDOVER_PROFILE_STATE_CONNECTING_REMOTE)
#define handoverProfile_IsDisconnecting(ho_inst)  (HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_DISCONNECTING)

static const uint16 l2cap_conftab[] =
{
    L2CAP_AUTOPT_SEPARATOR,                                          /* START */
    L2CAP_AUTOPT_MTU_IN,            HANDOVER_PROFILE_L2CAP_MTU_SIZE, /* Maximum inbound MTU - 895 bytes */
    L2CAP_AUTOPT_MTU_OUT,           0x0030,                          /* Minimum acceptable outbound MTU - 48 bytes */
    L2CAP_AUTOPT_FLUSH_IN,          0x0000, 0x0000,                  /* Min acceptable remote flush timeout - zero*/
                                    0xFFFF, 0xFFFF,                  /* Max acceptable remote flush timeout - infinite*/
    L2CAP_AUTOPT_FLUSH_OUT,         0x0000, 0x0000,                  /* Min local flush timeout - zero */
                                    0xFFFF, 0xFFFF,                  /* Max local flush timeout - infinite */
    L2CAP_AUTOPT_TERMINATOR                                          /* END */
};

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! \brief Send confirmation of a connection to all registered clients.

    \param[in] status   Refer \ref handover_profile_status_t
*/
static void handoverProfile_SendConnectConfirmation(handover_profile_status_t status)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    /* Send HANDOVER_PROFILE_CONNECT_CFM to client which made a connect request. */
    if(ho_inst->connect_task != NULL)
    {
        MESSAGE_MAKE(message, HANDOVER_PROFILE_CONNECT_CFM_T);
        message->status = status;
        MessageSend(ho_inst->connect_task, HANDOVER_PROFILE_CONNECT_CFM, message);
    }
}

/*! \brief Send confirmation of a disconnection to all registered clients.

    \param[in] status   Refer \ref handover_profile_status_t
*/
static void handoverProfile_SendDisconnectConfirmation(handover_profile_status_t status)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    /* Send HANDOVER_PROFILE_DISCONNECT_CFM to clients who made a disconnect request. */
    if(ho_inst->disconnect_task != NULL)
    {
        MESSAGE_MAKE(message, HANDOVER_PROFILE_DISCONNECT_CFM_T);
        message->status = status;
        MessageSend(ho_inst->disconnect_task, HANDOVER_PROFILE_DISCONNECT_CFM, message);
    }
}

/*! \brief Send indication of connection state to all registered clients.

    \param[in] status   Refer \ref handover_profile_status_t.
*/
static void handoverProfile_SendConnectionIndication(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    /* Send message to all clients registered. */
    TaskList_MessageSendId(&ho_inst->handover_client_tasks,HANDOVER_PROFILE_CONNECTION_IND);
}

/*! \brief Send indication of disconnection state to all registered clients.

    \param[in] status   Refer \ref handover_profile_status_t
*/
static void handoverProfile_SendDisconnectionIndication(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    /* Send message to all clients registered. */
    TaskList_MessageSendId(&ho_inst->handover_client_tasks, HANDOVER_PROFILE_DISCONNECTION_IND);
}

/*! \brief Shutdown (or disconnect) the Handover profile connection.

    \param[in] task   Client task
*/
void HandoverProfile_Shutdown(Task task)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    ho_inst->disconnect_task=task;
    MessageSend(&ho_inst->task, HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ, NULL);
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_CONNECTING_LOCAL state. */
static void handoverProfile_EnterConnectingLocal(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("handoverProfile_EnterConnectingLocal");
    ho_inst->sdp_search_attempts = 0;
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_CONNECTING_LOCAL state. */
static void handoverProfile_ExitConnectingLocal(void)
{
    DEBUG_LOG("handoverProfile_ExitConnectingLocal");
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_CONNECTING_REMOTE state. */
static void handoverProfile_EnterConnectingRemote(void)
{
    DEBUG_LOG("handoverProfile_EnterConnectingRemote");
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_CONNECTING_REMOTE state. */
static void handoverProfile_ExitConnectingRemote(void)
{
    DEBUG_LOG("handoverProfile_ExitConnectingRemote");
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_CONNECTED state. */
static void handoverProfile_EnterConnected(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("handoverProfile_EnterConnected");

    /* Cancel any other startup requests */
    MessageCancelAll(&ho_inst->task, HANDOVER_PROFILE_INTERNAL_STARTUP_REQ);

    /* Send  */
    handoverProfile_SendConnectConfirmation(HANDOVER_PROFILE_STATUS_SUCCESS);

    /* If we have any clients inform them of handover signalling connection */
    handoverProfile_SendConnectionIndication();

}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_CONNECTED state. */
static void handoverProfile_ExitConnected(void)
{
    DEBUG_LOG("handoverProfile_ExitConnected");
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_DISCONNECTED state.

    \param[in] old_state    previous state
*/
static void handoverProfile_EnterDisconnected(handover_profile_state_t old_state)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("handoverProfile_EnterDisconnected");

    if(old_state > HANDOVER_PROFILE_STATE_DISCONNECTED &&
           old_state < HANDOVER_PROFILE_STATE_CONNECTED)
    {
        handoverProfile_SendConnectConfirmation(HANDOVER_PROFILE_STATUS_PEER_CONNECT_FAILED);
    }
    else if(old_state >= HANDOVER_PROFILE_STATE_CONNECTED)
    {
        handoverProfile_SendDisconnectConfirmation(HANDOVER_PROFILE_STATUS_SUCCESS);
        /* If we have any clients inform them of handover signalling disconnection */
        handoverProfile_SendDisconnectionIndication();
    }
    else
    {
        DEBUG_LOG("handoverProfile_EnterDisconnected, enterd from an Initialized state %d", old_state);
    }

    ho_inst->sdp_search_attempts = 0;
    ho_inst->is_primary = FALSE;
    /* Clear peer address, as we use that to detect if we've previously reject a peer connection */
    BdaddrSetZero(&ho_inst->peer_addr);
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_DISCONNECTED state. */
static void handoverProfile_ExitDisconnected(void)
{
    DEBUG_LOG("handoverProfile_ExitDisconnected");
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_INITIALISING state. */
static void handoverProfile_EnterInitialising(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("handoverProfile_EnterInitialising");

    /* Register a Protocol/Service Multiplexor (PSM) that will be
       used for this application. The same PSM is used at both
       ends. */
    ConnectionL2capRegisterRequest(&ho_inst->task, L2CA_PSM_INVALID, 0);
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_INITIALISING state. */
static void handoverProfile_ExitInitialising(void)
{
    DEBUG_LOG("handoverProfile_ExitInitialising");

    MessageSend(Init_GetInitTask(), HANDOVER_PROFILE_INIT_CFM, NULL);
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH state. */
static void handoverProfile_EnterSdpSearch(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("handoverProfile_EnterSdpSearch");

    /* Perform SDP search */
    ConnectionSdpServiceSearchAttributeRequest(&ho_inst->task, &ho_inst->peer_addr, 0x32,
                                               sdp_GetHandoverProfileServiceSearchRequestSize(), sdp_GetHandoverProfileServiceSearchRequest(),
                                               sdp_GetHandoverProfileAttributeSearchRequestSize(), sdp_GetHandoverProfileAttributeSearchRequest());
    ho_inst->sdp_search_attempts++;
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH state. */
static void handoverProfile_ExitSdpSearch(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("handoverProfile_ExitSdpSearch");

    ho_inst->sdp_search_attempts = 0;
}

/*! \brief Performs operation required while entering the HANDOVER_PROFILE_STATE_DISCONNECTING state. */
static void handoverProfile_EnterDisconnecting(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("handoverProfile_EnterDisconnecting");

    if (SinkIsValid(ho_inst->link_sink))
    {
        ConnectionL2capDisconnectRequest(&ho_inst->task, ho_inst->link_sink);
    }
}

/*! \brief Performs operation required for exiting the HANDOVER_PROFILE_STATE_DISCONNECTING state. */
static void handoverProfile_ExitDisconnecting(void)
{
    DEBUG_LOG("handoverProfile_ExitDisconnecting");
}

/*! \brief Start Handover Signalling channel

    Start handover profile signalling channel by establishing the L2CAP connection.

    \param[in] task         Client task requesting the Handover profile connection.
    \param[in] peer_addr    Address of the peer device
*/
void HandoverProfile_Startup(Task task, const bdaddr *peer_addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("handoverProfile_Startup peer_addr [ %04x,%02x,%06lx ], sdp_search_attempts=%d",
                  peer_addr->nap, peer_addr->uap, peer_addr->lap, ho_inst->sdp_search_attempts);

    MESSAGE_MAKE(message, HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T);
    message->peer_addr = *peer_addr;
    /* Wait until SDP search is complete if ongoing. In this case, 'sdp_search_attempts' will be set to zero when exiting SDP search state */
    MessageSendConditionally(&ho_inst->task, HANDOVER_PROFILE_INTERNAL_STARTUP_REQ, message, &ho_inst->sdp_search_attempts);

    ho_inst->connect_task = task;
}

/*! \brief Print Error Message

    \param[in] id         Message which is not handled.
*/
static void handoverProfile_handleUnhandledMsg(MessageId id)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    UNUSED(id);
    DEBUG_LOG("handoverProfile_handleUnhandledMsg, state %u, id %u", HandoverProfile_GetState(ho_inst), id);
    Panic();
}

/*! \brief Extract remote PSM value from a service record. 

    Extract the remote PSM value from a service record returned by a SDP service search.

    \param[in] begin        Start address of the SDP search result attributes
    \param[in] end          End address of the SDP search result attributes
    \param[in] psm          Address to store the remote L2CAP PSM value
    \param[in] id           search ID.

    \return TRUE: Able to find the L2CAP PSM for the Service UUID
            FALSE: Unable to find the L2CAP PSM for the Service UUID
*/
static bool handoverProfile_GetL2capPSM(const uint8 *begin, const uint8 *end, uint16 *psm, uint16 id)
{
    ServiceDataType type;
    Region record, protocols, protocol, value;
    record.begin = begin;
    record.end   = end;

    while (ServiceFindAttribute(&record, id, &type, &protocols))
    {
        if (type == sdtSequence)
        {
            while (ServiceGetValue(&protocols, &type, &protocol))
            {
                if (type == sdtSequence &&
                    ServiceGetValue(&protocol, &type, &value) &&
                    type == sdtUUID &&
                    RegionMatchesUUID32(&value, (uint32)UUID16_L2CAP) &&
                    ServiceGetValue(&protocol, &type, &value) &&
                    type == sdtUnsignedInteger)
                {
                    *psm = (uint16)RegionReadUnsigned(&value);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/*! \brief Initiate L2CAP connection request for Handover profile to the peer device 

    Extract the remote PSM value from a service record returned by a SDP service search.

    \param[in] bd_addr      BD Address of the peer device

*/
static void handoverProfile_ConnectL2cap(const bdaddr *bd_addr)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("handoverProfile_ConnectL2cap");

    ConnectionL2capConnectRequest(&ho_inst->task,
                                  bd_addr,
                                  ho_inst->local_psm, ho_inst->remote_psm,
                                  CONFTAB_LEN(l2cap_conftab),
                                  l2cap_conftab);

}

/******************************************************************************
 * Handlers for Handover Profile L2CAP channel  messages
 ******************************************************************************/

/*! \brief Set handover profile to new state.

    \param[in] state      Refer \ref handover_profile_state_t, new state.

*/
void HandoverProfile_SetState(handover_profile_state_t state)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    handover_profile_state_t old_state = HandoverProfile_GetState(ho_inst);
    DEBUG_LOG("handover_profile_state_t, state %x", state);

    /* Handle state exit functions */
    switch (old_state)
    {
        case HANDOVER_PROFILE_STATE_INITIALISING:
            handoverProfile_ExitInitialising();
            break;
        case HANDOVER_PROFILE_STATE_DISCONNECTED:
            handoverProfile_ExitDisconnected();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH:
            handoverProfile_ExitSdpSearch();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_LOCAL:
            handoverProfile_ExitConnectingLocal();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_REMOTE:
            handoverProfile_ExitConnectingRemote();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTED:
            handoverProfile_ExitConnected();
            break;
        case HANDOVER_PROFILE_STATE_DISCONNECTING:
            handoverProfile_ExitDisconnecting();
            break;
        default:
            break;
    }

    /* Set new state */
    ho_inst->state = state;

    /* Handle state entry functions */
    switch (state)
    {
        case HANDOVER_PROFILE_STATE_INITIALISING:
            handoverProfile_EnterInitialising();
            break;
        case HANDOVER_PROFILE_STATE_DISCONNECTED:
            handoverProfile_EnterDisconnected(old_state);
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH:
            handoverProfile_EnterSdpSearch();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_LOCAL:
            handoverProfile_EnterConnectingLocal();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTING_REMOTE:
            handoverProfile_EnterConnectingRemote();
            break;
        case HANDOVER_PROFILE_STATE_CONNECTED:
            handoverProfile_EnterConnected();
            break;
        case HANDOVER_PROFILE_STATE_DISCONNECTING:
            handoverProfile_EnterDisconnecting();
            break;
        default:
            break;
    }
}

/*! \brief Handles internal startup request of handover profile.

    Handles internal startup request by intiating connection to peer device based on 
    the current state machine.

    \param[in] req      Refer \ref HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T, pointer to 
                        internal startup request message.

*/
void HandoverProfile_HandleInternalStartupRequest(HANDOVER_PROFILE_INTERNAL_STARTUP_REQ_T *req)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleInternalStartupRequest, state %u, bdaddr %04x,%02x,%06lx",
               HandoverProfile_GetState(ho_inst),
               req->peer_addr.nap, req->peer_addr.uap, req->peer_addr.lap);

    switch (HandoverProfile_GetState(ho_inst))
    {
        case HANDOVER_PROFILE_STATE_DISCONNECTED:
        {
            /* Check if ACL is now up */
            if (ConManagerIsConnected(&req->peer_addr))
            {
                DEBUG_LOG("HandoverProfile_HandleInternalStartupRequest, ACL connected");

                /* Store address of peer */
                ho_inst->peer_addr = req->peer_addr;

                /* Begin the search for the handover profile SDP record */
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH);
            }
            else
            {
                /* Send connect failed status since there is no ACL link between the peer devices */
                handoverProfile_SendConnectConfirmation(HANDOVER_PROFILE_STATUS_PEER_CONNECT_FAILED);
                /* Since we are already in disconnected we continue to stay in the same state */
                return;
            }
        }
        break;

        case HANDOVER_PROFILE_STATE_CONNECTED:
        {
            /* If we have any clients inform them of handover signalling connection */
            handoverProfile_SendConnectConfirmation(HANDOVER_PROFILE_STATUS_SUCCESS);
        }
        break;

        default:
        {
            handoverProfile_handleUnhandledMsg(HANDOVER_PROFILE_INTERNAL_STARTUP_REQ);
        }
        break;
    }

    /* Cancel any other startup requests */
    MessageCancelAll(&ho_inst->task, HANDOVER_PROFILE_INTERNAL_STARTUP_REQ);
}

/*! \brief Handles internal shut-down request of handover-profile.

    Handles internal shutdown request by intiating disconnection to peer device based on 
    the current state machine state.
*/
void HandoverProfile_HandleInternalShutdownReq(void)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleInternalShutdownReq, state %u", HandoverProfile_GetState(ho_inst));

    switch (HandoverProfile_GetState(ho_inst))
    {
        case HANDOVER_PROFILE_STATE_CONNECTING_LOCAL:
            {
                 handoverProfile_SendConnectConfirmation(HANDOVER_PROFILE_STATUS_PEER_CONNECT_CANCELLED);
            }
        case HANDOVER_PROFILE_STATE_CONNECTED:
        case HANDOVER_PROFILE_STATE_CONNECTING_REMOTE:
            {
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTING);
            }
            break;

        case HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH:
            {
                /* Cancel SDP search */
                ConnectionSdpTerminatePrimitiveRequest(&ho_inst->task);
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
            }
        case HANDOVER_PROFILE_STATE_DISCONNECTED:
            {
                handoverProfile_SendDisconnectConfirmation(HANDOVER_PROFILE_STATUS_SUCCESS);
            }
            break;

        default:
            {
                handoverProfile_handleUnhandledMsg(HANDOVER_PROFILE_INTERNAL_SHUTDOWN_REQ);
            }
            break;
    }
}

/*! \brief Handle result of L2CAP PSM registration request.

    Handles registration of handover-profile with the L2CAP and register the handover 
    profile with the SDP.

    \param[in] cfm      Refer \ref CL_L2CAP_REGISTER_CFM_T, pointer to L2CAP register 
                        confirmation message.

*/
void HandoverProfile_HandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleClL2capRegisterCfm, status %u, psm %u", cfm->status, cfm->psm);
    PanicFalse(HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_INITIALISING);

    /* We have registered the PSM used for handover profile link with
       connection manager, now need to wait for requests to process
       an incoming connection or make an outgoing connection. */
    if (success == cfm->status)
    {
        /* Copy and update SDP record */
        uint8 *record = PanicUnlessMalloc(sdp_GetHandoverProfileServiceRecordSize());
        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        ho_inst->local_psm = cfm->psm;
        memcpy(record, sdp_GetHandoverProfileServiceRecord(), sdp_GetHandoverProfileServiceRecordSize());
        /* Write L2CAP PSM into service record */
        sdp_SetHandoverProfilePsm(record, cfm->psm);

        /* Register service record */
        ConnectionRegisterServiceRecord(&ho_inst->task, 
                                        sdp_GetHandoverProfileServiceRecordSize(), 
                                        record);
    }
    else
    {
        DEBUG_LOG("HandoverProfile_HandleClL2capRegisterCfm, failed to register L2CAP PSM");
        Panic();
    }
}

/*! \brief Handle result of the SDP service record registration request.

    Handles confirmation received for registration of handover-profile service record with SDP 
    and move to HANDOVER_PROFILE_STATE_DISCONNECTED state.

    \param[in] cfm      Refer \ref CL_SDP_REGISTER_CFM_T, pointer to SDP register 
                        confirmation message.

*/
void HandoverProfile_HandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleClSdpRegisterCfm, status %d", cfm->status);
    PanicFalse(HandoverProfile_GetState(ho_inst) == HANDOVER_PROFILE_STATE_INITIALISING);

    if (cfm->status == sds_status_success)
    {
        /* Move to 'disconnected' state */
        HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
    }
    else
    {
        DEBUG_LOG("HandoverProfile_HandleClSdpRegisterCfm, SDP registration failed");
        Panic();
    }
}

/*! \brief Handle the result of a SDP service attribute search.

    The returned attributes are checked to make sure they match the expected format of a 
    handover profile service record.

    \param[in] cfm      Refer \ref CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T, pointer to SDP searched 
                        attribute results.

*/
void HandoverProfile_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, status %d", cfm->status);

    switch (HandoverProfile_GetState(ho_inst))
    {
        case HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH:
        {
            /* Find the PSM in the returned attributes */
            if (cfm->status == sdp_response_success)
            {
                if (handoverProfile_GetL2capPSM(cfm->attributes, cfm->attributes + cfm->size_attributes,
                                         &ho_inst->remote_psm, saProtocolDescriptorList))
                {
                    DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, peer psm 0x%x", ho_inst->remote_psm);

                    /* Initiate outgoing peer L2CAP connection */
                    handoverProfile_ConnectL2cap(&ho_inst->peer_addr);
                    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_CONNECTING_LOCAL);
                }
                else
                {
                    /* No PSM found, malformed SDP record on peer? */
                    DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, malformed SDP record");
                    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
                }
            }
            else if (cfm->status == sdp_no_response_data)
            {
                /* Peer Earbud doesn't support Handover Profile service */
                DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, unsupported");
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
            }
            else
            {
                if (ConManagerIsConnected(&ho_inst->peer_addr) && ho_inst->sdp_search_attempts < HandoverProfile_GetSdpSearchTryLimit())
                {
                    /* SDP seach failed, retry? */
                    DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, retry");
                    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_CONNECTING_SDP_SEARCH);
                }
                else
                {
                    DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, moving to disconnected state. Retry attempts %d",
                                ho_inst->sdp_search_attempts);
                    HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
                }
            }
        }
        break;

        case HANDOVER_PROFILE_STATE_DISCONNECTED:
        {
            DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, state machine is disconnected state to client calling disconnect request ");
        }
        break;
        
        default:
        {
            DEBUG_LOG("HandoverProfile_HandleClSdpServiceSearchAttributeCfm, unexpected state 0x%x", HandoverProfile_GetState(ho_inst));
            Panic();
        }
        break;
    }
}

/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device.

    \param[in] ind      Refer \ref CL_L2CAP_CONNECT_IND_T, pointer to L2CAP connection 
                        indication message.

*/
void HandoverProfile_HandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    bool accept = FALSE;

    DEBUG_LOG("HandoverProfile_HandleL2capConnectInd, state %u, psm %u, local_psm %u", HandoverProfile_GetState(ho_inst), ind->psm, ho_inst->local_psm);

    /* If the PSM doesn't match, then send l2cap failure and put the device in disconnected state */
    PanicFalse(ind->psm == ho_inst->local_psm);

    switch (HandoverProfile_GetState(ho_inst))
    {
        case HANDOVER_PROFILE_STATE_DISCONNECTED:
        {
            /* only accept Handover Signalling connections from paired peer devices. */
            if (appDeviceIsPeer(&ind->bd_addr))
            {
                DEBUG_LOG("HandoverProfile_HandleL2capConnectInd, accepted");

                /* Move to 'connecting remote' state */
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_CONNECTING_REMOTE);

                /* Accept connection */
                accept = TRUE;
            }
            else
            {
                DEBUG_LOG("HandoverProfile_HandleL2capConnectInd, rejected, unknown peer");
            }
        }
        break;

        default:
        {
            DEBUG_LOG("HandoverProfile_HandleL2capConnectInd, rejected, state %u", HandoverProfile_GetState(ho_inst));
        }
        break;
    }

    /* Send a response accepting or rejcting the connection. */
    ConnectionL2capConnectResponse(&ho_inst->task,     /* The client task. */
                                   accept,                 /* Accept/reject the connection. */
                                   ind->psm,               /* The local PSM. */
                                   ind->connection_id,     /* The L2CAP connection ID.*/
                                   ind->identifier,        /* The L2CAP signal identifier. */
                                   CONFTAB_LEN(l2cap_conftab),
                                   l2cap_conftab);          /* The configuration table. */
}

/*! \brief Handle the result of a L2CAP connection request.

    This is called for both local and remote initiated L2CAP requests.

    \param[in] cfm      Refer \ref CL_L2CAP_CONNECT_CFM_T, pointer to L2CAP connect 
                        confirmation.

*/
void HandoverProfile_HandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();

    DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, status %u", cfm->status);

    /* Pending connection, return, will get another message in a bit */
    if (l2cap_connect_pending == cfm->status)
    {
        DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, connect pending, wait");
        return;
    }

    switch (HandoverProfile_GetState(ho_inst))
    {
        case HANDOVER_PROFILE_STATE_CONNECTING_LOCAL:
        case HANDOVER_PROFILE_STATE_CONNECTING_REMOTE:
        {
            /* If connection was succesful, get sink, attempt to enable wallclock and move
             * to connected state */
            if (cfm->status == l2cap_connect_success)
            {
                DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, connected, conn ID %u, flush remote %u, mtu_remote %u, qos  %u", cfm->connection_id, cfm->flush_timeout_remote, cfm->mtu_remote, cfm->qos_remote);

                PanicNull(cfm->sink);
                ho_inst->link_sink = cfm->sink;
                ho_inst->link_source = StreamSourceFromSink(cfm->sink);

                /* Configure the tx (sink) & rx (source) */
                appLinkPolicyUpdateRoleFromSink(ho_inst->link_sink);

                MessageStreamTaskFromSink(ho_inst->link_sink, &ho_inst->task);
                MessageStreamTaskFromSource(ho_inst->link_source, &ho_inst->task);

                PanicFalse(SinkConfigure(ho_inst->link_sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL));
                PanicFalse(SourceConfigure(ho_inst->link_source, VM_SOURCE_MESSAGES, VM_MESSAGES_ALL));

                /* Connection successful; go to connected state */
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_CONNECTED);
            }
            else if (cfm->status >= l2cap_connect_failed)
            {
                DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, failed, go to disconnected state");
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
            }
            else
            {
                DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, L2CAP connection is Pending");
            }
        }
        break;

        case HANDOVER_PROFILE_STATE_DISCONNECTING:
        {
            /* The L2CAP connect request was cancelled by a SHUTDOWN_REQ
               before we received the L2CAP_CONNECT_CFM. */
            DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, cancelled");

            if (l2cap_connect_success == cfm->status)
            {
                ho_inst->link_sink = cfm->sink;

                /* Re-enter the DISCONNECTING state - this time the L2CAP
                   disconnect request will be sent because link_sink is valid. */
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTING);
            }
            else
            {
                /* There is no valid L2CAP link to disconnect so go straight
                   to DISCONNECTED. */
                HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
            }
        }
        break;

        default:
        {
            DEBUG_LOG("HandoverProfile_HandleL2capConnectCfm, failed");
            PanicFalse(l2cap_connect_success != cfm->status);
        }
        break;
    }
}

/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void HandoverProfile_HandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleL2capDisconnectInd, status %u", ind->status);

    /* Always send reponse */
    ConnectionL2capDisconnectResponse(ind->identifier, ind->sink);

    if (!handoverProfile_IsDisconnected(ho_inst))
    {
        /* Setting is_primary flag to false as next handover shall happen from new primary earbud */
        ho_inst->is_primary=FALSE;
        
        /* Only change state if sink matches */
        if (ind->sink == ho_inst->link_sink)
        {
            /* Inform clients if link loss and we initiated the original connection */
            if (ind->status == l2cap_disconnect_link_loss && !BdaddrIsZero(&ho_inst->peer_addr))
            {
                DEBUG_LOG("HandoverProfile_HandleL2capDisconnectInd, link-loss");

                /* Set link-loss flag */
                ho_inst->link_loss_occured = TRUE;
            }
            else
            {
                /* Clear link-loss flag */
                ho_inst->link_loss_occured = FALSE;
            }

            HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
        }
        else
        {
            DEBUG_LOG("HandoverProfile_HandleL2capDisconnectInd, sink doesn't match");
            Panic();
        }
    }
}

/*! \brief Handle a L2CAP disconnect confirmation.

    This is called for both local and remote initiated disconnects.

    \param[in] cfm      Refer \ref CL_L2CAP_DISCONNECT_CFM_T, pointer to L2CAP disconnect 
                        confirmation.

*/
void HandoverProfile_HandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm)
{
    handover_profile_task_data_t *ho_inst = Handover_GetTaskData();
    DEBUG_LOG("HandoverProfile_HandleL2capDisconnectCfm, status %u", cfm->status);

    /* Move to disconnected state if we're in the disconnecting state */
    if (handoverProfile_IsDisconnecting(ho_inst))
    {
        HandoverProfile_SetState(HANDOVER_PROFILE_STATE_DISCONNECTED);
    }
    /* Don't worry if we are disconnected already - may have been disconnected remotely */
    else if (!handoverProfile_IsDisconnected(ho_inst))
    {
        DEBUG_LOG("HandoverProfile_HandleL2capDisconnectCfm, Received disconnect in a wrong state ");
        Panic();
    }
}
#endif /* INCLUDE_MIRRORING */
