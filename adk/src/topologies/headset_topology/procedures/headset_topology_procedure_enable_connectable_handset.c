/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
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

#include "headset_topology_procedure_enable_connectable_handset.h"
#include "headset_topology_procedures.h"

#include <handset_service.h>

#include <logging.h>

#include <message.h>


/*! Parameter definition for connectable enable */
const ENABLE_CONNECTABLE_HANDSET_PARAMS_T hs_topology_procedure_connectable_handset_enable = { .enable = TRUE };
/*! Parameter definition for connectable disable */
const ENABLE_CONNECTABLE_HANDSET_PARAMS_T hs_topology_procedure_connectable_handset_disable = { .enable = FALSE };

static void headsetTopology_ProcEnableConnectableHandsetHandleMessage(Task task, MessageId id, Message message);
static void HeadsetTopology_ProcedureEnableConnectableHandsetStart(Task result_task,
                                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                                   procedure_complete_func_t proc_complete_fn,
                                                                   Message goal_data);

static void HeadsetTopology_ProcedureEnableConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);


typedef struct
{
    TaskData task;
    ENABLE_CONNECTABLE_HANDSET_PARAMS_T params;
} headsetTopProcEnableConnectableHandsetTaskData;

const procedure_fns_t hs_proc_enable_connectable_handset_fns =
{
    HeadsetTopology_ProcedureEnableConnectableHandsetStart,
    HeadsetTopology_ProcedureEnableConnectableHandsetCancel,
};

headsetTopProcEnableConnectableHandsetTaskData headsettop_proc_enable_connectable_handset = {headsetTopology_ProcEnableConnectableHandsetHandleMessage};


#define HeadsetTopProcEnableConnectableHandsetGetTaskData()     (&headsettop_proc_enable_connectable_handset)
#define HeadsetTopProcEnableConnectableHandsetGetTask()         (&headsettop_proc_enable_connectable_handset.task)


static void HeadsetTopology_ProcedureEnableConnectableHandsetStart(Task result_task,
                                                                   procedure_start_cfm_func_t proc_start_cfm_fn,
                                                                   procedure_complete_func_t proc_complete_fn,
                                                                   Message goal_data)
{
    headsetTopProcEnableConnectableHandsetTaskData* td = HeadsetTopProcEnableConnectableHandsetGetTaskData();
    ENABLE_CONNECTABLE_HANDSET_PARAMS_T* params = (ENABLE_CONNECTABLE_HANDSET_PARAMS_T*)goal_data;

    UNUSED(result_task);

    td->params = *params;

    if (params->enable)
    {
        DEBUG_LOG("HeadsetTopology_ProcedureEnableConnectableHandsetStart ENABLE");

        HandsetService_ConnectableRequest(HeadsetTopProcEnableConnectableHandsetGetTask());
    }
    else
    {
        DEBUG_LOG("HeadsetTopology_ProcedureEnableConnectableHandsetStart DISABLE");

        HandsetService_CancelConnectableRequest(HeadsetTopProcEnableConnectableHandsetGetTask());
    }

    proc_start_cfm_fn(hs_topology_procedure_enable_connectable_handset, procedure_result_success);

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn,
                                          hs_topology_procedure_enable_connectable_handset,
                                          procedure_result_success);
}

static void HeadsetTopology_ProcedureEnableConnectableHandsetCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    headsetTopProcEnableConnectableHandsetTaskData* td = HeadsetTopProcEnableConnectableHandsetGetTaskData();

    if (td->params.enable)
    {
        DEBUG_LOG("HeadsetTopology_ProcedureEnableConnectableHandsetCancel cancel enable");

        HandsetService_CancelConnectableRequest(HeadsetTopProcEnableConnectableHandsetGetTask());
    }
    else
    {
        DEBUG_LOG("HeadsetTopology_ProcedureEnableConnectableHandsetCancel cancel disable");

        HandsetService_ConnectableRequest(HeadsetTopProcEnableConnectableHandsetGetTask());
    }

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                        hs_topology_procedure_enable_connectable_handset,
                                        procedure_result_success);

}

static void headsetTopology_ProcEnableConnectableHandsetHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);
    UNUSED(id);
}

