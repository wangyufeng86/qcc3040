/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure for allowing/blocking Bluetooth activities. 

            \note this allows the activities... but does not create them

            The procedure affects traditional Bluetooth (BREDR) and also Bluetooth
            Low Energy activities.

            BREDR is allowing by enabling scanning, used for creating connections.
            LE is allowed by enabling scanning and advertising. 

            Connections are blocked without the need to specifically block activity 
            in the connection manager.

            Internally the requested enables are all run asynchronously. The 
            confirmation messages are handled separately and a conditional message
            send is used to confirm the overall permission.
*/

#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedures.h"

#include <le_advertising_manager.h>
#include <le_scan_manager_protected.h>
#include <bredr_scan_manager_protected.h>

#include <logging.h>

#include <message.h>
#include <panic.h>

static void twsTopology_ProcPermitBtHandleMessage(Task task, MessageId id, Message message);
void TwsTopology_ProcedurePermitBtStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedurePermitBtCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

extern const PERMIT_BT_OPERATIONS_T proc_permit_bt_enable = {.enable = TRUE};
extern const PERMIT_BT_OPERATIONS_T proc_permit_bt_disable = {.enable = FALSE};


const procedure_fns_t proc_permit_bt_fns = {
    TwsTopology_ProcedurePermitBtStart,
    TwsTopology_ProcedurePermitBtCancel,
};

typedef struct
{
    TaskData task;      /*! The task used for this procedures messages */
                        /*!  Function passed into start, that should receive the completion status */
    procedure_complete_func_t complete_fn;
    uint16 pending;     /*!< Activities we are waiting for a response for */
    bool enabling;      /*!< Whether we are enabling or disabling */
} twsTopProcPermitBtTaskData;

twsTopProcPermitBtTaskData twstop_proc_permit_bt = {twsTopology_ProcPermitBtHandleMessage};

/*! Enables run in parallel and we don't need confirmation of their success
    Bitmasks are used to record the status for debug purposes only */
#define TWS_TOP_PROC_PERMIT_BT_BREDR_SCAN   (1 << 0)
#define TWS_TOP_PROC_PERMIT_BT_LE_SCAN      (1 << 1)
#define TWS_TOP_PROC_PERMIT_BT_ADVERTISING  (1 << 2)

/*! over ride time to catch the LE Scan failing to disable. */
#define TWS_TOP_PROC_PERMIT_LE_SCAN_DISABLE_TIME_MS     (D_SEC(10))

#define TwsTopProcPermitBtGetTaskData()     (&twstop_proc_permit_bt)
#define TwsTopProcPermitBtGetTask()         (&twstop_proc_permit_bt.task)

typedef enum
{
        /*! All permit functions have completed */
    TWS_TOP_PROC_PERMIT_BT_INTERNAL_COMPLETED,
        /*! We have failed to disable LE Scan in a reasonable time */
    TWS_TOP_PROC_PERMIT_BT_INTERNAL_LE_SCAN_DISABLE_TIMEOUT,
} tws_top_proc_permit_bt_internal_message_t;


void TwsTopology_ProcedurePermitBtStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    PERMIT_BT_OPERATIONS_T *params = (PERMIT_BT_OPERATIONS_T *)goal_data;
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    UNUSED(result_task);

    DEBUG_LOG("TwsTopology_ProcedurePermitBtStart");

    PanicNull(params);

    permitData->complete_fn = proc_complete_fn;
    permitData->enabling = params->enable;

    if (params->enable)
    {
        BredrScanManager_ScanEnable();
        LeAdvertisingManager_AllowAdvertising(TwsTopProcPermitBtGetTask(), TRUE);
        LeScanManager_Enable(TwsTopProcPermitBtGetTask());

        permitData->pending =   TWS_TOP_PROC_PERMIT_BT_LE_SCAN
                              | TWS_TOP_PROC_PERMIT_BT_ADVERTISING;
    }
    else
    {
        BredrScanManager_ScanDisable(TwsTopProcPermitBtGetTask());
        LeAdvertisingManager_AllowAdvertising(TwsTopProcPermitBtGetTask(), FALSE);

            /* The LE Scan Manager can return busy, requiring a retry
               use a LONG timer to track this, so we can Panic if needed */
        LeScanManager_Disable(TwsTopProcPermitBtGetTask());
        MessageSendLater(TwsTopProcPermitBtGetTask(),
                        TWS_TOP_PROC_PERMIT_BT_INTERNAL_LE_SCAN_DISABLE_TIMEOUT,
                        NULL,
                        TWS_TOP_PROC_PERMIT_LE_SCAN_DISABLE_TIME_MS);

        permitData->pending =  TWS_TOP_PROC_PERMIT_BT_BREDR_SCAN
                             | TWS_TOP_PROC_PERMIT_BT_LE_SCAN
                             | TWS_TOP_PROC_PERMIT_BT_ADVERTISING;
    }
    MessageSendConditionally(TwsTopProcPermitBtGetTask(),
                             TWS_TOP_PROC_PERMIT_BT_INTERNAL_COMPLETED, NULL, 
                             &permitData->pending);

    /* procedure started synchronously so return TRUE */
    proc_start_cfm_fn(tws_topology_procedure_permit_bt, procedure_result_success);
}

void TwsTopology_ProcedurePermitBtCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedurePermitBtCancel");

    MessageCancelAll(TwsTopProcPermitBtGetTask(), TWS_TOP_PROC_PERMIT_BT_INTERNAL_COMPLETED);
    MessageCancelAll(TwsTopProcPermitBtGetTask(), TWS_TOP_PROC_PERMIT_BT_INTERNAL_LE_SCAN_DISABLE_TIMEOUT);

    permitData->pending = 0;
    permitData->complete_fn = NULL;

    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_permit_bt, procedure_result_success);
}


static void twsTopology_ProcPermitBtHandleBredrScanManagerDisableCfm(const BREDR_SCAN_MANAGER_DISABLE_CFM_T *cfm)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    DEBUG_LOG("twsTopology_ProcPermitBtHandleBredrScanManagerDisableCfm %u", cfm->disabled);

    if (!permitData->enabling)
    {
        if (cfm->disabled)
        {
            permitData->pending &= ~TWS_TOP_PROC_PERMIT_BT_BREDR_SCAN;
        }
        else
        {
            Panic();
        }
    }
    else
    {
        DEBUG_LOG("twsTopology_ProcPermitBtHandleBredrScanManagerDisableCfm Disabled, but waiting for enable !!");
    }
}


static void twsTopology_ProcPermitBtHandleLeAdvManagerAllowCfm(const LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T *cfm)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    DEBUG_LOG("twsTopology_ProcPermitBtHandleLeAdvManagerAllowCfm %u", cfm->status);

    if (le_adv_mgr_status_success == cfm->status)
    {
        if (cfm->allow == permitData->enabling)
        {
            permitData->pending &= ~TWS_TOP_PROC_PERMIT_BT_ADVERTISING;
        }
        else
        {
            DEBUG_LOG("twsTopology_ProcPermitBtHandleLeAdvManagerAllowCfm Response not for expected operation");
        }
    }
    else
    {
        Panic();
    }
}

static void twsTopology_ProcPermitBtHandleLeScanManagerEnableCfm(const LE_SCAN_MANAGER_ENABLE_CFM_T *cfm)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    DEBUG_LOG("twsTopology_ProcPermitBtHandleLeScanManagerEnableCfm %u", cfm->status);

    if (LE_SCAN_MANAGER_RESULT_SUCCESS == cfm->status)
    {
        if (permitData->enabling)
        {
            permitData->pending &= ~TWS_TOP_PROC_PERMIT_BT_LE_SCAN;
        }
        else
        {
            DEBUG_LOG("twsTopology_ProcPermitBtHandleLeScanManagerEnableCfm Enabled, but waiting for disable !!");
        }
    }
    else
    {
        Panic();
    }
}


static void twsTopology_ProcPermitBtHandleLeScanManagerDisableCfm(const LE_SCAN_MANAGER_DISABLE_CFM_T *cfm)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    DEBUG_LOG("twsTopology_ProcPermitBtHandleLeScanManagerDisableCfm %u", cfm->status);

    if (!permitData->enabling)
    {
        switch (cfm->status)
        {
        case LE_SCAN_MANAGER_RESULT_SUCCESS:
            permitData->pending &= ~TWS_TOP_PROC_PERMIT_BT_LE_SCAN;
            MessageCancelAll(TwsTopProcPermitBtGetTask(), TWS_TOP_PROC_PERMIT_BT_INTERNAL_LE_SCAN_DISABLE_TIMEOUT);
            break;

        case LE_SCAN_MANAGER_RESULT_BUSY:
            /* Retry the disable until it works. */
            LeScanManager_Disable(TwsTopProcPermitBtGetTask());
            break;

        default:
            Panic();
        }
    }
    else
    {
        DEBUG_LOG("twsTopology_ProcPermitBtHandleLeScanManagerDisableCfm Disabled, but waiting for enable !!");
    }
}


static void twsTopology_ProcPermitBtHandleMessage(Task task, MessageId id, Message message)
{
    twsTopProcPermitBtTaskData *permitData = TwsTopProcPermitBtGetTaskData();

    UNUSED(task);

    if (!permitData->complete_fn)
    {
        return;
    }

    switch (id)
    {
        case TWS_TOP_PROC_PERMIT_BT_INTERNAL_COMPLETED:
            /*! \todo This is naive. Cancel needs to be dealt with */
            permitData->complete_fn(tws_topology_procedure_permit_bt, procedure_result_success);
            permitData->complete_fn = NULL;
            break;

        case TWS_TOP_PROC_PERMIT_BT_INTERNAL_LE_SCAN_DISABLE_TIMEOUT:
            DEBUG_LOG("twsTopology_ProcPermitBtHandleMessage Failed to disable LE scan within %d ms",
                        TWS_TOP_PROC_PERMIT_LE_SCAN_DISABLE_TIME_MS);
            Panic();
            break;

        case BREDR_SCAN_MANAGER_DISABLE_CFM:
            twsTopology_ProcPermitBtHandleBredrScanManagerDisableCfm((const BREDR_SCAN_MANAGER_DISABLE_CFM_T*)message);
            break;

        case LE_ADV_MGR_ALLOW_ADVERTISING_CFM:
            twsTopology_ProcPermitBtHandleLeAdvManagerAllowCfm((const LE_ADV_MGR_ALLOW_ADVERTISING_CFM_T *)message);
            break;

        case LE_SCAN_MANAGER_ENABLE_CFM:
            twsTopology_ProcPermitBtHandleLeScanManagerEnableCfm((const LE_SCAN_MANAGER_ENABLE_CFM_T *)message);
            break;

        case LE_SCAN_MANAGER_DISABLE_CFM:
            twsTopology_ProcPermitBtHandleLeScanManagerDisableCfm((const LE_SCAN_MANAGER_DISABLE_CFM_T *)message);
            break;

        default:
            break;
    }
}

