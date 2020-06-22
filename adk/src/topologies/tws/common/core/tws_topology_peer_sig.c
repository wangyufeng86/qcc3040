/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Implementation of TWS Topology use of peer signalling marshalled message channel.
*/

#include "tws_topology.h"
#include "tws_topology_goals.h"
#include "tws_topology_peer_sig.h"
#include "tws_topology_private.h"
#include "tws_topology_typedef.h"
#include "tws_topology_marshal_typedef.h"
#include "tws_topology_rule_events.h"

#include "tws_topology_procedure_command_role_switch.h"

#include <peer_signalling.h>
#include <power_manager.h>
#include <task_list.h>
#include <logging.h>
#include <timestamp_event.h>

#include <message.h>
#include <panic.h>
#include <marshal.h>
#include "voice_ui.h"

void TwsTopology_RegisterPeerSigClient(Task task)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    task_list_t* list = NULL;

    list = TaskList_GetFlexibleBaseTaskList((task_list_flexible_t*)&td->peer_sig_msg_client_list);
    TaskList_AddTask(list, task);
}

void TwsTopology_UnregisterPeerSigClient(Task task)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    task_list_t* list = NULL;

    list = TaskList_GetFlexibleBaseTaskList((task_list_flexible_t*)&td->peer_sig_msg_client_list);
    PanicFalse(TaskList_RemoveTask(list, task));
}

void TwsTopology_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    switch (ind->type)
    {
        case MARSHAL_TYPE(tws_topology_remote_rule_event_t):
            {
                tws_topology_remote_rule_event_t* event_req = (tws_topology_remote_rule_event_t*)ind->msg;

                DEBUG_LOG("TwsTopology_HandleMarshalledMsgChannelRxInd tws_topology_remote_rule_event_t event 0x%x", event_req->event);
                twsTopology_RulesSetEvent(event_req->event);
            }
            break;

        case MARSHAL_TYPE(tws_topology_remote_static_handover_cmd_t):
            {
                tws_topology_remote_static_handover_cmd_t* handover_cmd = (tws_topology_remote_static_handover_cmd_t*)ind->msg;

                DEBUG_LOG("TwsTopology_HandleMarshalledMsgChannelRxInd tws_topology_remote_static_handover_cmd_t media 0x%x", handover_cmd->media_was_playing);
                TimestampEvent(TIMESTAMP_EVENT_ROLE_SWAP_COMMAND_RECEIVED);
                appAvPlayOnHandsetConnection(handover_cmd->media_was_playing);
                twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_STATIC_HANDOVER);

                MessageCancelAll(TwsTopologyGetTask(), TWSTOP_INTERNAL_CLEAR_HANDOVER_PLAY);
                MessageSendLater(TwsTopologyGetTask(), TWSTOP_INTERNAL_CLEAR_HANDOVER_PLAY, NULL, D_SEC(20));
            }
            break;

#ifdef TOPOLOGY_PEER_SIG_RX_HANDLING_ENABLE
        case MARSHAL_TYPE(tws_topology_NEW_PEER_SIG_MSG:
            TWSTOP_PEER_SIG_MSG_RX_T* message = PanicUnlessMalloc(sizeof(TWSTOP_PEER_SIG_MSG_RX_T));

            DEBUG_LOG("TwsTopology_HandleMarshalledMsgChannelRxInd tws_topology_remote_goal_cfm_t");

            memcpy(&message->msg.NEW_PEER_SIG_MSG_TYPE, ind->msg, sizeof(tws_topology_NEW_PEER_SIG_MSG_TYPE));
            message->type = MARSHAL_TYPE(tws_topology_NEW_PEER_SIG_MSG_TYPE);
            TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList((task_list_flexible_t*)&td->peer_sig_msg_client_list), TWSTOP_PEER_SIG_MSG_RX, message);
            break;
#endif

        default:
            break;
    }

    /* free unmarshalled msg */
    free(ind->msg);
}

void TwsTopology_HandleMarshalledMsgChannelTxCfm(PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    twsTopologyTaskData* td = TwsTopologyGetTaskData();
    task_list_t *peersig_subscribers = TaskList_GetFlexibleBaseTaskList((task_list_flexible_t*)&td->peer_sig_msg_client_list);

    DEBUG_LOG("TwsTopology_HandleMarshalledMsgChannelTxCfm sts:%u type:%d role:%d", cfm->status, cfm->type, TwsTopology_GetRole());

    if (cfm->type == MARSHAL_TYPE(tws_topology_remote_static_handover_cmd_t))
    {
        if (TwsTopology_GetRole() == tws_topology_role_shutdown)
        {
            DEBUG_LOG("tws_topology_role_shutdown indicating prepare completed");

            /* Assume that this is a response to the requested TWSTOP_RULE_EVENT_FORCED_ROLE_SWITCH.
             We need to complete the Shutdown, so continue regardless of cfm->status field. If the
             message transmission failed, it will be safeguarded by the "Peer link dropped, then find
             role" rule. */
            appPowerShutdownPrepareResponse(TwsTopologyGetTask());
        }
    }

    if (TaskList_Size(peersig_subscribers))
    {
        PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* copy = PanicUnlessMalloc(sizeof(PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T));
        *copy = *cfm;
        TaskList_MessageSend(peersig_subscribers , PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM, copy);
    }

}

void TwsTopology_SecondaryStaticHandoverCommand(void)
{
    tws_topology_remote_static_handover_cmd_t* req = PanicUnlessMalloc(sizeof(tws_topology_remote_static_handover_cmd_t));
    bool playing = ((appAvPlayStatus() == avrcp_play_status_playing) && (VoiceUi_IsVoiceAssistantA2dpStreamActive() == FALSE));
    req->media_was_playing = playing;

    DEBUG_LOG("TwsTopology_SecondaryStaticHandoverCommand type:%d (playing:%d)", MARSHAL_TYPE(tws_topology_remote_static_handover_cmd_t), playing);

    appPeerSigMarshalledMsgChannelTx(TwsTopologyGetTask(), PEER_SIG_MSG_CHANNEL_TOPOLOGY, req, MARSHAL_TYPE(tws_topology_remote_static_handover_cmd_t));
}
