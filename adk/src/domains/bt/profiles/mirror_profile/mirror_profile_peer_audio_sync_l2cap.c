/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       mirror_profile_peer_audio_sync_l2cap.c
\brief      Mirror profile L2cap Channel creation for Audio synchronisation.
*/

#ifdef INCLUDE_MIRRORING

#include <service.h>
#include <bt_device.h>
#include "mirror_profile_peer_audio_sync_l2cap.h"
#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "sdp.h"
/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/

/******************************************************************************
 * Macro Definitions
 ******************************************************************************/

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! \brief Performs operation required while entering the MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH state. */
static void mirrorProfile_EnterSdpSearch(void)
{
    DEBUG_LOG("mirrorProfile_EnterSdpSearch");

    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    /* Perform SDP search */
    ConnectionSdpServiceSearchAttributeRequest(&mirror_inst->task_data, &mirror_inst->audio_sync.peer_addr, 0x32,
                                               appSdpGetMirrorProfileServiceSearchRequestSize(), appSdpGetMirrorProfileServiceSearchRequest(),
                                               appSdpGetMirrorProfileAttributeSearchRequestSize(), appSdpGetMirrorProfileAttributeSearchRequest());
    mirror_inst->audio_sync.sdp_search_attempts++;
}

/*! \brief Send confirmation of a connection to all registered clients.

    \param[in] status   Refer \ref mirror_profile_status_t
*/
static void mirrorProfile_SendConnectConfirmation(mirror_profile_status_t status)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    /* Send MIRROR_PROFILE_CONNECT_CFM to client which made a connect request. */
    if(mirror_inst->audio_sync.connect_task != NULL)
    {
        MESSAGE_MAKE(message, MIRROR_PROFILE_CONNECT_CFM_T);
        message->status = status;
        MessageSend(mirror_inst->audio_sync.connect_task, MIRROR_PROFILE_CONNECT_CFM, message);
        mirror_inst->audio_sync.connect_task = NULL;
    }
}

/*! \brief Send confirmation of a disconnection to all registered clients.

    \param[in] status   Refer \ref mirror_profile_status_t
*/
static void mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_t status)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    /* Send MIRROR_PROFILE_DISCONNECT_CFM to client which made a disconnect request. */
    if(mirror_inst->audio_sync.disconnect_task != NULL)
    {
        MESSAGE_MAKE(message, MIRROR_PROFILE_DISCONNECT_CFM_T);
        message->status = status;
        MessageSend(mirror_inst->audio_sync.disconnect_task, MIRROR_PROFILE_DISCONNECT_CFM, message);
        mirror_inst->audio_sync.disconnect_task = NULL;
    }
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
static bool mirrorProfile_GetL2capPSM(const uint8 *begin, const uint8 *end, uint16 *psm, uint16 id)
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

/*! \brief Initiate L2CAP connection request for Mirror profile to the peer device 

    Extract the remote PSM value from a service record returned by a SDP service search.

    \param[in] bd_addr      BD Address of the peer device

*/
static void mirrorProfile_ConnectL2cap(const bdaddr *bd_addr)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Flow & Error Control Mode. */
        L2CAP_AUTOPT_FLOW_MODE,
        /* Set to Basic mode with no fallback mode */
        BKV_16_FLOW_MODE( FLOW_MODE_BASIC, 0 ),
        /* Local MTU exact value (incoming). */
        L2CAP_AUTOPT_MTU_IN,
        /*  Exact MTU for this L2CAP connection - 672. */
        MIRROR_PROFILE_L2CAP_MTU_SIZE,
        /* Remote MTU Minumum value (outgoing). */
        L2CAP_AUTOPT_MTU_OUT,
        /*  Minimum MTU accepted from the Remote device. */
        48,
        L2CAP_AUTOPT_FLUSH_IN,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Local Flush Timeout  */
        L2CAP_AUTOPT_FLUSH_OUT,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("mirrorProfile_ConnectL2cap");

    ConnectionL2capConnectRequest(&mirror_inst->task_data,
                                  bd_addr,
                                  mirror_inst->audio_sync.local_psm,
                                  mirror_inst->audio_sync.remote_psm,
                                  CONFTAB_LEN(l2cap_conftab),
                                  l2cap_conftab);

}

/*! \brief Function to set the L2CAP internal state information.

    \param[in] state      Internal state value to be updated.
*/
static void mirrorProfile_SetAudioSyncL2capState(mirror_profile_audio_sync_l2cap_state_t state)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("mirrorProfile_SetAudioSyncL2capState: exit %d enter %d",
                mirror_inst->audio_sync.l2cap_state, state);

    /*! update the internal state */
    mirror_inst->audio_sync.l2cap_state = state;

    switch (state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            MirrorProfile_SetTargetStateFromProfileState();

            /*! Send the connect confirmation back to topology layer */
            mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connect_failed);
            mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_peer_disconnected);

            /*! Reset the sdp search attempts */
            mirror_inst->audio_sync.sdp_search_attempts = 0;

            MessageCancelFirst(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT);

            MirrorProfile_ClearQhsReady();
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            /*! Start the SDP Search */
            mirrorProfile_EnterSdpSearch();
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        {
            mirror_inst->audio_sync.sdp_search_attempts = 0;
            /*! Establish the L2cap connection for Audio Synchronisation */
            mirrorProfile_ConnectL2cap(&mirror_inst->audio_sync.peer_addr);
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
        {
            if(SinkIsValid(mirror_inst->audio_sync.link_sink))
            {
                /* Initiate the L2cap disconnect request */
                ConnectionL2capDisconnectRequest(&mirror_inst->task_data, mirror_inst->audio_sync.link_sink);
            }
            else 
            {
                
                DEBUG_LOG("mirrorProfile_SetAudioSyncL2capState: Unable to disconnect as no valid sink handle"); 
            }
        }
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING:
        break;

        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED:
        {
            if (!MirrorProfile_IsQhsReady())
            {
                MessageSendLater(MirrorProfile_GetTask(), MIRROR_INTERNAL_QHS_START_TIMEOUT, NULL,
                                 mirrorProfileConfig_QhsStartTimeout());
            }
            else
            {
                /*! Set the Mirror profile state */
                MirrorProfile_SetTargetStateFromProfileState();
            }

            /* Send connected status */
            mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connected);
        }
        break;

        default:
        break;
    }
}

/*! \brief Handles shut-down request for Mirror-profile.

    Handles L2cap shutdown request by intiating disconnection to peer device based on
    the current machine state.
*/
static void mirrorProfile_HandleShutdown(void)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            /*! Cancel the ongoing SDP search */
            ConnectionSdpTerminatePrimitiveRequest(&mirror_inst->task_data);

            /*! Update the internal L2cap state to none( No L2cap channel exists) */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
        }
        break;
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED:
        {
            /*! Move to L2cap disconnecting state */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING);
        }
        break;
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            /*! No L2cap channel exists! Just send disconnected status back */
            mirrorProfile_SendDisconnectConfirmation(mirror_profile_status_peer_disconnected);
        }
        break;

        default:
        break;
    }
}

/*! \brief Close the L2cap channel created for audio synchronisation.

    Close the L2CAP connection for Audio synchronisation for Mirror Profile.
*/
void MirrorProfile_CloseAudioSyncL2capChannel(Task task)
{
    /*! Store the disconnect task */
    MirrorProfile_GetAudioSyncL2capState()->disconnect_task = task;
    mirrorProfile_HandleShutdown();
}

/*! \brief Function to handle the service search attribute request.

    \param[in] cfm Service search attribute confirmation message.
*/
void MirrorProfile_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, status %d", cfm->status);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH:
        {
            /* Find the PSM in the returned attributes */
            if (cfm->status == sdp_response_success)
            {
                if (mirrorProfile_GetL2capPSM(cfm->attributes, cfm->attributes + cfm->size_attributes,
                                         &mirror_inst->audio_sync.remote_psm, saProtocolDescriptorList))
                {
                    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, peer psm 0x%x", mirror_inst->audio_sync.remote_psm);

                    /* Create the L2cap connection */
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING);
                }
                else
                {
                    /* No PSM found */
                    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, malformed SDP record");

                    /*! Update the internal L2cap state to none( No L2cap channel exists) */
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
                }
            }
            else if (cfm->status == sdp_no_response_data)
            {
                /* Peer Earbud doesn't support Mirror Profile service */
                DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, unsupported");

                /*! Update the internal L2cap state to none( No L2cap channel exists) */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
            else
            {
                if (ConManagerIsConnected(&mirror_inst->audio_sync.peer_addr) && mirror_inst->audio_sync.sdp_search_attempts < MirrorProfile_GetSdpSearchTryLimit())
                {
                    /* SDP search failed, retry! */
                    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, retry");
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH);
                }
                else
                {
                    DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, moving to disconnected state. Retry attempts %d",
                                mirror_inst->audio_sync.sdp_search_attempts);

                    /*! Update the internal L2cap state to none( No L2cap channel exists) */
                    mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
                }
            }
        }
        break;
        default:
        {
            DEBUG_LOG("MirrorProfile_HandleClSdpServiceSearchAttributeCfm, unexpected state 0x%x", MirrorProfile_GetState());
            Panic();
        }
        break;
    }
}

/*! \brief Handle the result of a L2CAP connection request.

    This is called for both local and remote initiated L2CAP requests.

    \param[in] cfm      Refer \ref CL_L2CAP_CONNECT_CFM_T, pointer to L2CAP connect 
                        confirmation.

*/
void MirrorProfile_HandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();

    DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, status %u", cfm->status);

    /* Pending connection, return, will get another message in a bit */
    if (l2cap_connect_pending == cfm->status)
    {
        DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, connect pending, wait");
        return;
    }

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING:
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING:
        {
            /* If connection was succesful, get sink, attempt to enable wallclock and move
             * to connected state */
            if (cfm->status == l2cap_connect_success)
            {
                DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, connected, conn ID %u, flush remote %u", cfm->connection_id, cfm->flush_timeout_remote);

                PanicNull(cfm->sink);
                mirror_inst->audio_sync.link_sink = cfm->sink;
                mirror_inst->audio_sync.link_source = StreamSourceFromSink(cfm->sink);

                MessageStreamTaskFromSink(mirror_inst->audio_sync.link_sink, &mirror_inst->task_data);
                MessageStreamTaskFromSource(mirror_inst->audio_sync.link_source, &mirror_inst->task_data);

                PanicFalse(SinkConfigure(mirror_inst->audio_sync.link_sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL));
                PanicFalse(SourceConfigure(mirror_inst->audio_sync.link_source, VM_SOURCE_MESSAGES, VM_MESSAGES_ALL));

                StreamConnectDispose(mirror_inst->audio_sync.link_source);

                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED);
            }
            else if (cfm->status >= l2cap_connect_failed)
            {
                DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, failed, go to disconnected state");

                /*! Move the L2cap state to none */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
            else
            {
                DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, L2CAP connection is Pending");
            }
        }
        break;
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
        {
            /* The L2cap channel is getting closed when we received the l2cap create confirmation message */
            DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, cancelled");

            if (l2cap_connect_success == cfm->status)
            {
                mirror_inst->audio_sync.link_sink = cfm->sink;

                /* Re-enter the DISCONNECTING state - this time the L2CAP
                   disconnect request will be sent because link_sink is valid. */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING);
            }
            else
            {
                /*! Move the L2cap state to none */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
            }
        }
        break;

        default:
        {
            DEBUG_LOG("MirrorProfile_HandleL2capConnectCfm, failed");
            PanicFalse(l2cap_connect_success != cfm->status);
        }
        break;
    }
}

/*! \brief Handle a L2CAP connection request that was initiated by the remote peer device.

    \param[in] ind      Refer \ref CL_L2CAP_CONNECT_IND_T, pointer to L2CAP connection 
                        indication message.

*/
void MirrorProfile_HandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    bool accept = FALSE;
    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        L2CAP_AUTOPT_FLUSH_IN,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        /* Local Flush Timeout  */
        L2CAP_AUTOPT_FLUSH_OUT,
        BKV_UINT32R(MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT,MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT),
        L2CAP_AUTOPT_TERMINATOR
    };

    DEBUG_LOG("MirrorProfile_HandleL2capConnectInd, state %u, psm %u, local_psm %u", MirrorProfile_GetState(), ind->psm, mirror_inst->audio_sync.local_psm);

    /* If the PSM doesn't match, panic! */
    PanicFalse(ind->psm == mirror_inst->audio_sync.local_psm);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE:
        {
            /* only accept L2cap connections from paired peer device. */
            if (appDeviceIsPeer(&ind->bd_addr))
            {
                DEBUG_LOG("MirrorProfile_HandleL2capConnectInd, accepted");

                /* Move to 'Remote connecting' state */
                mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING);

                MirrorProfile_GetAudioSyncL2capState()->peer_addr = ind->bd_addr;

                /* Accept connection */
                accept = TRUE;
            }
            else
            {
                DEBUG_LOG("MirrorProfile_HandleL2capConnectInd, rejected, unknown peer");
            }
        }
        break;

        default:
        {
            DEBUG_LOG("MirrorProfile_HandleL2capConnectInd, rejected, state %u", MirrorProfile_GetState());
        }
        break;
    }

    /* Send a response accepting or rejecting the connection. */
    ConnectionL2capConnectResponse(&mirror_inst->task_data,/* The client task. */
                                   accept,                 /* Accept/reject the connection. */
                                   ind->psm,               /* The local PSM. */
                                   ind->connection_id,     /* The L2CAP connection ID.*/
                                   ind->identifier,        /* The L2CAP signal identifier. */
                                   CONFTAB_LEN(l2cap_conftab),
                                   l2cap_conftab);          /* The configuration table. */
}

/*! \brief Create the L2cap channel for the Audio Synchronistion.
 */
void MirrorProfile_CreateAudioSyncL2capChannel(Task task, const bdaddr *peer_addr)
{
    mirror_profile_audio_sync_context_t *audio_sync = MirrorProfile_GetAudioSyncL2capState();

    audio_sync->connect_task = task;
    audio_sync->peer_addr = *peer_addr;

    DEBUG_LOG("MirrorProfile_CreateAudioSyncL2capChannel, state %u, bdaddr %04x,%02x,%06lx",
               MirrorProfile_GetState(),
               peer_addr->nap,
               peer_addr->uap,
               peer_addr->lap);

    /* Check if ACL is now up */
    if (ConManagerIsConnected(&audio_sync->peer_addr))
    {
        DEBUG_LOG("MirrorProfile_CreateAudioSyncL2capChannel, ACL connected");

        /* Trigger the SDP Search */
        mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH);
    }
    else
    {
        /* Send connect failed status since there is no ACL link between the peer devices */
        mirrorProfile_SendConnectConfirmation(mirror_profile_status_peer_connect_failed);
    }
}


/*! \brief Handle a L2CAP disconnect initiated by the remote peer.

    \param[in] ind      Refer \ref CL_L2CAP_DISCONNECT_IND_T, pointer to L2CAP disconnect 
                        indication.

*/
void MirrorProfile_HandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    DEBUG_LOG("MirrorProfile_HandleL2capDisconnectInd, status %u", ind->status);

    /* Always send reponse */
    ConnectionL2capDisconnectResponse(ind->identifier, ind->sink);

    /* Only change state if sink matches */
    if (ind->sink == mirror_inst->audio_sync.link_sink)
    {
        /*! Move to L2cap none state */
        mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
    }
}
/*! \brief Handle a L2CAP disconnect confirmation.

    This is called for both local and remote initiated disconnects.

    \param[in] cfm      Refer \ref CL_L2CAP_DISCONNECT_CFM_T, pointer to L2CAP disconnect 
                        confirmation.

*/
void MirrorProfile_HandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm)
{
    mirror_profile_task_data_t *mirror_inst = MirrorProfile_Get();
    DEBUG_LOG("MirrorProfile_HandleL2capDisconnectCfm, status %u", cfm->status);

    switch (mirror_inst->audio_sync.l2cap_state)
    {
        case MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING:
        {
            /*! L2cap channel got closed. Move to L2cap none state */
            mirrorProfile_SetAudioSyncL2capState(MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE);
        }
        default:
        break;
    }
}

void MirrorProfile_HandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm)
{
    mirror_profile_task_data_t *mirror_task_data = MirrorProfile_Get();
    DEBUG_LOG("MirrorProfile_HandleClL2capRegisterCfm, status %u, psm %u", cfm->status, cfm->psm);

    /* We have registered the PSM used for mirror profile link with
       connection manager, now need to wait for requests to process
       an incoming connection or make an outgoing connection. */
    if (success == cfm->status)
    {
        /* Copy and update SDP record */
        uint8 *record = PanicUnlessMalloc(appSdpGetMirrorProfileServiceRecordSize());
        
        /* Keep a copy of the registered L2CAP PSM, maybe useful later */
        mirror_task_data->audio_sync.local_psm = cfm->psm;
        
        memcpy(record, appSdpGetMirrorProfileServiceRecord(), appSdpGetMirrorProfileServiceRecordSize());
        
        /* Write L2CAP PSM into service record */
        appSdpSetMirrorProfilePsm(record, cfm->psm);

        /* Register service record */
        ConnectionRegisterServiceRecord(MirrorProfile_GetTask(),
                                        appSdpGetMirrorProfileServiceRecordSize(), 
                                        record);
    }
    else
    {
        DEBUG_LOG("MirrorProfile_HandleClL2capRegisterCfm, failed to register L2CAP PSM");
        Panic();
    }
}

void MirrorProfile_HandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    DEBUG_LOG("MirrorProfile_HandleClSdpRegisterCfm, status %d", cfm->status);

    if (cfm->status == sds_status_success)
    {
        /* Register with the firmware to receive MESSAGE_BLUESTACK_MDM_PRIM messages */
        MessageMdmTask(MirrorProfile_GetTask());
        
        /* Register with the MDM service */
        MirrorProfile_MirrorRegisterReq();
    }
    else
    {
        DEBUG_LOG("MirrorProfile_HandleClSdpRegisterCfm, SDP registration failed");
        Panic();
    }
}

#endif /* INCLUDE_MIRRORING */
