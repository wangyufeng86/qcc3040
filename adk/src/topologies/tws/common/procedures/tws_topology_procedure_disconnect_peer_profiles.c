/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to disconnect peer BREDR profiles.
*/

#include "tws_topology_procedure_disconnect_peer_profiles.h"
#include "tws_topology_procedures.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_config.h"

#include <bt_device.h>
#include <peer_signalling.h>
#include <handover_profile.h>
#include <scofwd_profile.h>
#include <mirror_profile.h>
#include <av.h>
#include <connection_manager.h>

#include <logging.h>

#include <message.h>

const DISCONNECT_PEER_PROFILES_T proc_disconnect_peer_profiles_all = {
    TwsTopologyConfig_PeerProfiles()
};

static void twsTopology_ProcDisconnectPeerProfilesHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedureDisconnectPeerProfilesStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDisconnectPeerProfilesCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_disconnect_peer_profiles_fns = {
    TwsTopology_ProcedureDisconnectPeerProfilesStart,
    TwsTopology_ProcedureDisconnectPeerProfilesCancel,
};

typedef struct
{
    TaskData task;
    Task result_task;
    procedure_complete_func_t complete_fn;
    uint8 profiles_status;
    bool wait_for_link_disconnect;
    bool active;
} twsTopProcDisconnectPeerProfilesTaskData;

twsTopProcDisconnectPeerProfilesTaskData twstop_proc_disconnect_peer_profiles = {twsTopology_ProcDisconnectPeerProfilesHandleMessage};

#define TwsTopProcDisconnectPeerProfilesGetTaskData()     (&twstop_proc_disconnect_peer_profiles)
#define TwsTopProcDisconnectPeerProfilesGetTask()         (&twstop_proc_disconnect_peer_profiles.task)

static void twsTopology_ProcedureDisconnectPeerProfilesResetProc(void)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();
    td->profiles_status = 0;
    td->active = FALSE;
}

static void twsTopology_ProcedureDisconnectPeerProfilesStopProfile(uint8 profiles)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();
    bdaddr peer_addr;

    /* update profiles being tracked for disconnect complete */
    td->profiles_status |= profiles;

    appDeviceGetPeerBdAddr(&peer_addr);

    /* If the profiles did not come up originally, the ACL may be held open */
    ConManagerReleaseAcl(&peer_addr);

    /* start the procedure */
    if (profiles & DEVICE_PROFILE_A2DP)
    {
        DEBUG_LOG("twsTopology_ProcedureDisconnectPeerProfilesStopProfile A2DP");
        appAvDisconnectPeer(&peer_addr);
        /*! \todo temporary fix as we do not yet have DISCONNECT_CFM from AV */
        td->profiles_status &= ~DEVICE_PROFILE_A2DP;
    }
    if (profiles & DEVICE_PROFILE_SCOFWD)
    {
        DEBUG_LOG("twsTopology_ProcedureDisconnectPeerProfilesStopProfile SCOFWD");
        ScoFwdDisconnectPeer(TwsTopProcDisconnectPeerProfilesGetTask());
    }
    if (profiles & DEVICE_PROFILE_PEERSIG)
    {
        DEBUG_LOG("twsTopology_ProcedureDisconnectPeerProfilesStopProfile PEERSIG");
        appPeerSigDisconnect(TwsTopProcDisconnectPeerProfilesGetTask());
    }
#ifdef INCLUDE_MIRRORING
    if (profiles & DEVICE_PROFILE_HANDOVER)
    {
        DEBUG_LOG("twsTopology_ProcedureDisconnectPeerProfilesStopProfile HANDPVER");
        HandoverProfile_Disconnect(TwsTopProcDisconnectPeerProfilesGetTask());
    }
    if (profiles & DEVICE_PROFILE_MIRROR)
    {
        DEBUG_LOG("twsTopology_ProcedureDisconnectPeerProfilesStopProfile MIRROR");
        MirrorProfile_Disconnect(TwsTopProcDisconnectPeerProfilesGetTask());
    }
#endif /* INCLUDE_MIRRORING */
}

void TwsTopology_ProcedureDisconnectPeerProfilesStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();
    DISCONNECT_PEER_PROFILES_T* dpp = (DISCONNECT_PEER_PROFILES_T*)goal_data;
    bdaddr peer_addr;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureDisconnectPeerProfilesStart profile %x", dpp->profiles);

    ConManagerRegisterConnectionsClient(TwsTopProcDisconnectPeerProfilesGetTask());

    /* fail if we don't have a paired peer */
    if (!appDeviceGetPeerBdAddr(&peer_addr))
    {
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, 
                                               tws_topology_procedure_disconnect_peer_profiles,
                                               procedure_result_failed);
    }

    /* complete already if not connected */
    if (!ConManagerIsConnected(&peer_addr))
    {
        proc_start_cfm_fn(tws_topology_procedure_disconnect_peer_profiles, procedure_result_success);
        Procedures_DelayedCompleteCfmCallback(proc_complete_fn, 
                                               tws_topology_procedure_disconnect_peer_profiles,
                                               procedure_result_success);
    }
    else
    {
        /* start the procedure */

        td->complete_fn = proc_complete_fn;
        td->active = TRUE;
        td->wait_for_link_disconnect = FALSE;

        /* if disconnecting all profiles and currently connected, then need to wait
         * for link disconnect to complete the procedure */
        if (dpp->profiles == TwsTopologyConfig_PeerProfiles())
        {
            td->wait_for_link_disconnect = TRUE;
        }
        /* disconnect request profiles */
        twsTopology_ProcedureDisconnectPeerProfilesStopProfile(dpp->profiles);

        /* procedure started synchronously so return TRUE */
        proc_start_cfm_fn(tws_topology_procedure_disconnect_peer_profiles, procedure_result_success);
    }
}

void TwsTopology_ProcedureDisconnectPeerProfilesCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDisconnectPeerProfilesCancel");
    twsTopology_ProcedureDisconnectPeerProfilesResetProc();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_disconnect_peer_profiles, procedure_result_success);
}

static void twsTopology_ProcDisconnectPeerProfilesStatus(uint8 profile)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();
    
    /* clear the disconnected profile and report complete if all done */
    td->profiles_status &= ~profile;
    if (!td->profiles_status && !td->wait_for_link_disconnect)
    {
        twsTopology_ProcedureDisconnectPeerProfilesResetProc();
        td->complete_fn(tws_topology_procedure_disconnect_peer_profiles, procedure_result_success);
    }
}

static void twsTopology_ProcDisconnectPeerProfilesHandleConManagerConnectionInd(const CON_MANAGER_CONNECTION_IND_T* ind)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();

    DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleConManagerConnectionInd Conn:%u BLE:%u %04x,%02x,%06lx", ind->connected,
                                                                                          ind->ble,
                                                                                          ind->bd_addr.nap,
                                                                                          ind->bd_addr.uap,
                                                                                          ind->bd_addr.lap);

    if (   (!ind->connected)
        && (appDeviceGetPeerBdAddr(&ind->bd_addr)
        && (!ind->ble)))
    {
        DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleConManagerConnectionInd peer disconnected");

        /* clear flag waiting on the link disconnect, if received link disconnection
         * notification before all profiles disconnected, then when final profile
         * disconnect is received the procedure will complete. */
        td->wait_for_link_disconnect = FALSE;

        /* if all profiles already complete, then issue completion now. */
        if (!td->profiles_status)
        {
            twsTopology_ProcedureDisconnectPeerProfilesResetProc();
            td->complete_fn(tws_topology_procedure_disconnect_peer_profiles, procedure_result_success);
        }
    }
}

static void twsTopology_ProcDisconnectPeerProfilesHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcDisconnectPeerProfilesTaskData* td = TwsTopProcDisconnectPeerProfilesGetTaskData();

    UNUSED(task);

    /* if no longer active (cleanup() was called) then ignore any CFM messages,
     * they'll be disconnect_cfm(cancelled) */
    if (!td->active)
    {
        return;
    }

    switch (id)
    {
        /*! \todo handle AV disconnect CFM */

        case PEER_SIG_DISCONNECT_CFM:
        {
            DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleMessage PEER_SIG_DISCONNECT_CFM");
            twsTopology_ProcDisconnectPeerProfilesStatus(DEVICE_PROFILE_PEERSIG);
        }
        break;

        case SFWD_DISCONNECT_CFM:
        {
            DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleMessage SFWD_DISCONNECT_CFM");
            twsTopology_ProcDisconnectPeerProfilesStatus(DEVICE_PROFILE_SCOFWD);
        }
        break;

        case CON_MANAGER_CONNECTION_IND:
        {
            twsTopology_ProcDisconnectPeerProfilesHandleConManagerConnectionInd((CON_MANAGER_CONNECTION_IND_T*)message);
        }
        break;

#ifdef INCLUDE_MIRRORING
        case MIRROR_PROFILE_DISCONNECT_CFM:
        {
            DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleMessage MIRROR_PROFILE_DISCONNECT_CFM");
            twsTopology_ProcDisconnectPeerProfilesStatus(DEVICE_PROFILE_MIRROR);
        }
        break;
        
        case HANDOVER_PROFILE_DISCONNECT_CFM:
        {
            DEBUG_LOG("twsTopology_ProcDisconnectPeerProfilesHandleMessage HANDOVER_PROFILE_DISCONNECT_CFM");
            twsTopology_ProcDisconnectPeerProfilesStatus(DEVICE_PROFILE_HANDOVER);
        }
        break;
#endif /* INCLUDE_MIRRORING */

        default:
        break;
    }
}
