/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      File containing a procedure that controls whether a handset is 
            allowed to connect over BREDR.

            The procedure takes a parameter that decides whether connection is 
            to be enabled or disabled.

            \note Other procedures can block connection at a lower level, for 
            instance disabling Bluetooth completely. These functions have the 
            effect of suspending the ability of a handset to connect.
*/

#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_procedures.h"
#include "le_advertising_manager.h"

#include <handset_service.h>
#include <bt_device.h>
#include <logging.h>

#include <message.h>
#include <panic.h>

/*! Parameter definition for connectable enable */
const ENABLE_CONNECTABLE_HANDSET_PARAMS_T proc_enable_connectable_handset_enable = { .enable = TRUE };
/*! Parameter definition for connectable disable */
const ENABLE_CONNECTABLE_HANDSET_PARAMS_T proc_connectable_handset_disable = { .enable = FALSE };

static void twsTopology_ProcEnableConnectableHandsetHandleMessage(Task task, MessageId id, Message message);
static void TwsTopology_ProcedureEnableConnectableHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void TwsTopology_ProcedureEnableConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_connectable_handset_fns = {
    TwsTopology_ProcedureEnableConnectableHandsetStart,
    TwsTopology_ProcedureEnableConnectableHandsetCancel,
};

typedef struct
{
    TaskData task;
    ENABLE_CONNECTABLE_HANDSET_PARAMS_T params;
} twsTopProcEnableConnectableHandsetTaskData;

twsTopProcEnableConnectableHandsetTaskData twstop_proc_enable_connectable_handset = {twsTopology_ProcEnableConnectableHandsetHandleMessage};

#define TwsTopProcEnableConnectableHandsetGetTaskData()     (&twstop_proc_enable_connectable_handset)
#define TwsTopProcEnableConnectableHandsetGetTask()         (&twstop_proc_enable_connectable_handset.task)

static void TwsTopology_ProcedureEnableConnectableHandsetStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcEnableConnectableHandsetTaskData* td = TwsTopProcEnableConnectableHandsetGetTaskData();
    ENABLE_CONNECTABLE_HANDSET_PARAMS_T* params = (ENABLE_CONNECTABLE_HANDSET_PARAMS_T*)goal_data;

    UNUSED(result_task);

    td->params = *params;

    /* start the procedure */
    if (params->enable)
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectableHandsetStart ENABLE");

        LeAdvertisingManager_ParametersSelect(0);
        HandsetService_ConnectableRequest(TwsTopProcEnableConnectableHandsetGetTask());
    }
    else
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectableHandsetStart DISABLE");

        if(appDeviceIsPeerConnected())
            LeAdvertisingManager_ParametersSelect(1);
        HandsetService_CancelConnectableRequest(TwsTopProcEnableConnectableHandsetGetTask());
    }

    proc_start_cfm_fn(tws_topology_procedure_enable_connectable_handset, procedure_result_success);

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn,
                                           tws_topology_procedure_enable_connectable_handset,
                                           procedure_result_success);
}

static void TwsTopology_ProcedureEnableConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcEnableConnectableHandsetTaskData* td = TwsTopProcEnableConnectableHandsetGetTaskData();

    if (td->params.enable)
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectableHandsetCancel cancel enable");

        HandsetService_CancelConnectableRequest(TwsTopProcEnableConnectableHandsetGetTask());
    }
    else
    {
        DEBUG_LOG("TwsTopology_ProcedureEnableConnectableHandsetCancel cancel disable");

        HandsetService_ConnectableRequest(TwsTopProcEnableConnectableHandsetGetTask());
    }

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                         tws_topology_procedure_enable_connectable_handset,
                                         procedure_result_success);
}

static void twsTopology_ProcEnableConnectableHandsetHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    UNUSED(id);
}
