/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to disconnect profiles, in-order to ensure in-case
            (fore-ground) DFU is run in an isolated manner with no concurrent
            BT traffic owing to other profiles.
*/

#include "tws_topology_procedure_dfu_in_case.h"
#include <hfp_profile.h>
#include <av.h>
#include "tws_topology_procedures.h"

#include <logging.h>

void TwsTopology_ProcedureDfuInCaseStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureDfuInCaseCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_dfu_in_case_fns = {
    TwsTopology_ProcedureDfuInCaseStart,
    TwsTopology_ProcedureDfuInCaseCancel,
};

void TwsTopology_ProcedureDfuInCaseStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    UNUSED(goal_data);
    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedureDfuInCaseStart");

    /*
     * Disconnect HFP, A2DP and/or AVRCP profiles during in-case DFU to avoid
     * concurrent BT traffic owing to these profiles.
     *
     * This procedure is unconditionally called both for Primary and Secondary
     * but is significant for Primary and results in a NOP for Secondary.
     *
     * This procedure is invoked on both Primary and Secondary
     */
    appHfpDisconnectInternal();
    appAvDisconnectHandset();

    /* procedure started synchronously, confirm start now */
    proc_start_cfm_fn(tws_topology_procedure_dfu_in_case, procedure_result_success);

    /*
     * The above disconnection requests are asynchronous. As these APIs don't
     * have task their corresponding CFM. Also not parameterized with task
     * context to receive/handle such CFM or IND. Hence the procedure is marked
     * as synchronously completed and so indicate completed already.
     */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn, tws_topology_procedure_dfu_in_case, procedure_result_success);
}

void TwsTopology_ProcedureDfuInCaseCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureDfuInCaseCancel");

    /* nothing to cancel, just return success to keep goal engine happy */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_dfu_in_case, procedure_result_success);
}

