/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure for Primary to connect BR/EDR ACL to Handset.
*/

#include "tws_topology_procedure_connect_handset.h"
#include "tws_topology_procedures.h"
#include "tws_topology_config.h"
#include "tws_topology_primary_ruleset.h"
#include "tws_topology_common_primary_rule_functions.h"

#include <handset_service.h>
#include <connection_manager.h>
#include <peer_find_role.h>

#include <logging.h>

#include <message.h>
#include <panic.h>



void TwsTopology_ProcedureConnectHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureConnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_connect_handset_fns = {
    TwsTopology_ProcedureConnectHandsetStart,
    TwsTopology_ProcedureConnectHandsetCancel,
};

typedef struct
{
    TaskData task;
    procedure_complete_func_t complete_fn;
    procedure_cancel_cfm_func_t cancel_fn;
    uint8 profiles_status;
    bool prepare_requested;
    bdaddr handset_addr;
} twsTopProcConnectHandsetTaskData;

twsTopProcConnectHandsetTaskData twstop_proc_connect_handset;

#define TwsTopProcConnectHandsetGetTaskData()     (&twstop_proc_connect_handset)
#define TwsTopProcConnectHandsetGetTask()         (&twstop_proc_connect_handset.task)

/*! Internal messages use by this ConnectHandset procedure. */
typedef enum
{
    PROC_CONNECT_HANDSET_INTERNAL_ACL_CONNECT,
    PROC_CONNECT_HANDSET_INTERNAL_ACL_CONNECT_TIMEOUT,
} procConnetHandsetInternalMessages;

static void twsTopology_ProcConnectHandsetHandleMessage(Task task, MessageId id, Message message);

twsTopProcConnectHandsetTaskData twstop_proc_connect_handset = {twsTopology_ProcConnectHandsetHandleMessage};

/*! \brief Send a response to a PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION.

    This will only send the response if we have received a
    PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION, otherwise it will do
    nothing.

    Note: There should only ever be one response per
          PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION received, hence why
          this is guarded on the prepare_requested flag.
*/
static void twsTopology_ProcConnectHandsetPeerFindRolePrepareRespond(void)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    if (td->prepare_requested)
    {
        PeerFindRole_PrepareResponse();
        td->prepare_requested = FALSE;
    }
}

static void twsTopology_ProcConnectHandsetResetProc(void)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    twsTopology_ProcConnectHandsetPeerFindRolePrepareRespond();

    td->complete_fn = NULL;
    td->cancel_fn = NULL;
    td->profiles_status = 0;
    td->prepare_requested = FALSE;
    BdaddrSetZero(&td->handset_addr);

    PeerFindRole_DisableScanning(FALSE);
    PeerFindRole_UnregisterPrepareClient(TwsTopProcConnectHandsetGetTask());
    ConManagerUnregisterConnectionsClient(TwsTopProcConnectHandsetGetTask());
}

void TwsTopology_ProcedureConnectHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();
    TWSTOP_PRIMARY_GOAL_CONNECT_HANDSET_T* chp = (TWSTOP_PRIMARY_GOAL_CONNECT_HANDSET_T*)goal_data;

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureConnectHandsetStart profiles 0x%x", chp->profiles);

    /* Block scanning temporarily while we are connecting */
    PeerFindRole_DisableScanning(TRUE);

    /* save state to perform the procedure */
    td->complete_fn = proc_complete_fn;
    td->profiles_status = chp->profiles;
    BdaddrSetZero(&td->handset_addr);

    /* start the procedure */
    if (appDeviceGetHandsetBdAddr(&td->handset_addr))
    {
        PeerFindRole_RegisterPrepareClient(TwsTopProcConnectHandsetGetTask());
        HandsetService_ConnectAddressRequest(TwsTopProcConnectHandsetGetTask(), &td->handset_addr, chp->profiles);
        ConManagerRegisterConnectionsClient(TwsTopProcConnectHandsetGetTask());
        proc_start_cfm_fn(tws_topology_procedure_connect_handset, procedure_result_success);
    }
    else
    {
        DEBUG_LOG("TwsTopology_ProcedureConnectHandsetStart shouldn't be called with no paired handset");
        Panic();
    }
}

void TwsTopology_ProcedureConnectHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureConnectHandsetCancel");

    td->complete_fn = NULL;
    td->cancel_fn = proc_cancel_cfm_fn;
    HandsetService_StopConnect(TwsTopProcConnectHandsetGetTask(), &td->handset_addr);
}

static void twsTopology_ProcConnectHandsetProfilesStatus(uint8 profile)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    /* clear the connected profile */
    td->profiles_status &= ~profile;

    /* report start complete if all done */
    if (!td->profiles_status)
    {
        if (td->complete_fn)
        {
            td->complete_fn(tws_topology_procedure_connect_handset, procedure_result_success);
        }
        twsTopology_ProcConnectHandsetResetProc();
    }
}

static void twsTopology_ProcConnectHandsetHandleHandsetConnectCfm(const HANDSET_SERVICE_CONNECT_CFM_T *cfm)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    DEBUG_LOG("twsTopology_ProcConnectHandsetHandleHandsetConnectCfm status %d", cfm->status);

    if (cfm->status == handset_service_status_success)
    {
        device_t dev = BtDevice_GetDeviceForBdAddr(&cfm->addr);
        uint8 profiles_connected = BtDevice_GetConnectedProfiles(dev);
        twsTopology_ProcConnectHandsetProfilesStatus(profiles_connected);
    }
    else if (cfm->status != handset_service_status_cancelled)
    {
        if (td->complete_fn)
        {
            td->complete_fn(tws_topology_procedure_connect_handset, procedure_result_failed);
        }
        else if (td->cancel_fn)
        {
            td->cancel_fn(tws_topology_procedure_connect_handset, procedure_result_success);
        }
        twsTopology_ProcConnectHandsetResetProc();
    }
    else
    {
        /* A status of handset_service_status_cancelled means the connect
           request ewas cancelled by a separate disconnect request. In the
           tws topology we should never overlap connect & disconnect requests
           like this so it is an error. */
        Panic();
    }
    
}

static void twsTopology_ProcConnectHandsetHandleHandsetConnectStopCfm(const HANDSET_SERVICE_CONNECT_STOP_CFM_T* cfm)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    DEBUG_LOG("twsTopology_ProcConnectHandsetHandleHandsetConnectStopCfm status 0x%x", cfm->status);

    if (BdaddrIsSame(&td->handset_addr, &cfm->addr))
    {
        twsTopology_ProcConnectHandsetPeerFindRolePrepareRespond();

        /* If the procedure was cancelled, let the topology know and tidy up
           this procedure. If not cancelled, wait for the
           HANDSET_SERVICE_CONNECT_CFM instead. */
        if (td->cancel_fn)
        {
            td->cancel_fn(tws_topology_procedure_connect_handset, procedure_result_success);
            twsTopology_ProcConnectHandsetResetProc();
        }
    }
}

static void twsTopology_ProcConnectHandsetHandlePeerFindRolePrepareForRoleSelection(void)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    DEBUG_LOG("twsTopology_ProcConnectHandsetHandlePeerFindRolePrepareForRoleSelection");

    HandsetService_StopConnect(TwsTopProcConnectHandsetGetTask(), &td->handset_addr);
    td->prepare_requested = TRUE;
}


/*! Use connection manager indication to re-enable scanning once we connect to handset

    We will do this anyway once we are fully connected to the handset (all selected
    profiles), but that can take some time.

    \param conn_ind The Connection manager indication
 */
static void twsTopology_ProcConnectHandsetHandleConMgrConnInd(const CON_MANAGER_CONNECTION_IND_T *conn_ind)
{
    DEBUG_LOG("twsTopology_ProcConnectHandsetHandleConMgrConnInd LAP:x%06x ble:%d conn:%d", 
                    conn_ind->bd_addr.lap, conn_ind->ble, conn_ind->connected);

    if (!conn_ind->ble
        && conn_ind->connected
        && appDeviceIsHandset(&conn_ind->bd_addr))
    {
        /* Additional call here as we only care about the handset connection,
            not the profiles */
        PeerFindRole_DisableScanning(FALSE);
        ConManagerUnregisterConnectionsClient(TwsTopProcConnectHandsetGetTask());
    }
}

static void twsTopology_ProcConnectHandsetHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcConnectHandsetTaskData* td = TwsTopProcConnectHandsetGetTaskData();

    UNUSED(task);
    UNUSED(message);

    if (!td->complete_fn && !td->cancel_fn)
    {
        /* If neither callback is set this procedure is not active so ignore any messages */
        return;
    }

    switch (id)
    {
    case HANDSET_SERVICE_CONNECT_CFM:
        twsTopology_ProcConnectHandsetHandleHandsetConnectCfm((const HANDSET_SERVICE_CONNECT_CFM_T *)message);
        break;

    case HANDSET_SERVICE_CONNECT_STOP_CFM:
        twsTopology_ProcConnectHandsetHandleHandsetConnectStopCfm((const HANDSET_SERVICE_CONNECT_STOP_CFM_T *)message);
        break;

    case CON_MANAGER_CONNECTION_IND:
        twsTopology_ProcConnectHandsetHandleConMgrConnInd((const CON_MANAGER_CONNECTION_IND_T *)message);
        break;

    case PEER_FIND_ROLE_PREPARE_FOR_ROLE_SELECTION:
        twsTopology_ProcConnectHandsetHandlePeerFindRolePrepareForRoleSelection();
        break;

    default:
        DEBUG_LOG("twsTopology_ProcConnectHandsetHandleMessage unhandled id 0x%x(%d)", id, id);
        break;
    }
}
