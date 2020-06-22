/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure to swap BT address.
*/

#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedures.h"

#include <bt_device.h>
#include <link_policy.h>
#include <timestamp_event.h>
#include <connection_manager.h>

#include <logging.h>

#include <message.h>
#include <bdaddr.h>
#include <panic.h>

const SET_ADDRESS_TYPE_T proc_set_address_primary = {TRUE};
const SET_ADDRESS_TYPE_T proc_set_address_secondary = {FALSE};

void TwsTopology_ProcedureSetAddressStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
void TwsTopology_ProcedureSetAddressCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);
static void twsTopology_ProcSetAddressHandleMessage(Task task, MessageId id, Message message);

const procedure_fns_t proc_set_address_fns = {
    TwsTopology_ProcedureSetAddressStart,
    TwsTopology_ProcedureSetAddressCancel,
};

/* Define components of script set_primary_address_script */
#define SET_PRIMARY_ADDRESS_SCRIPT(ENTRY) \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_DISABLE), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_DISABLE), \
    ENTRY(proc_set_address_fns, PROC_SET_ADDRESS_TYPE_DATA_PRIMARY), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), 
/* Create the script set_primary_address_script */
DEFINE_TOPOLOGY_SCRIPT(set_primary_address, SET_PRIMARY_ADDRESS_SCRIPT);

/*! The max number of times to attempt to override the BD_ADDR */
#define OVERRIDE_BD_ADDR_MAX_ATTEMPTS   (300)
/*! Delay in ms between attempts to override address. */
#define OVERRIDE_ATTEMPT_DELAY_MS       (5)

typedef struct
{
    TaskData task;
    bdaddr addr;
    procedure_complete_func_t complete_fn;
    unsigned attempts_remaining;
    bool first_attempt;
} twsTopProcSetAddressTaskData;

twsTopProcSetAddressTaskData twstop_proc_set_address = { .task = twsTopology_ProcSetAddressHandleMessage };
#define TwsTopProcSetAddressGetTaskData()   (&twstop_proc_set_address)
#define TwsTopProcSetAddressGetTask()       (&twstop_proc_set_address.task)

/*! \brief Messages the set address procedure can send to itself. */
typedef enum
{
    /*! Internal message to delay retry of attempt to set address. */
    PROC_SET_ADDR_INTERNAL_RETRY,
} twsTopProcSetAddressInternalMessage;

static void twsTopology_ProcSetAddressReset(void)
{
    twsTopProcSetAddressTaskData* td = TwsTopProcSetAddressGetTaskData();
    td->complete_fn = NULL;
    td->attempts_remaining = OVERRIDE_BD_ADDR_MAX_ATTEMPTS;
    td->first_attempt = TRUE;
    MessageCancelFirst(TwsTopProcSetAddressGetTask(), PROC_SET_ADDR_INTERNAL_RETRY);
}

static void twsTopology_ProcSetAddressSuccess(void)
{
    twsTopProcSetAddressTaskData* td = TwsTopProcSetAddressGetTaskData();

    TimestampEvent(TIMESTAMP_EVENT_ADDRESS_SWAP_COMPLETED);

    DEBUG_LOG_INFO("twsTopology_ProcSetAddressSuccess (%u)", OVERRIDE_BD_ADDR_MAX_ATTEMPTS - td->attempts_remaining);
    /* update my address in BT device database if required */
    PanicFalse(BtDevice_SetMyAddress(&td->addr));
    Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_set_address, procedure_result_success);
    appLinkPolicyHandleAddressSwap();
    twsTopology_ProcSetAddressReset();
}

static bool twsTopology_ProcSetAddressOverrideAddress(void)
{
    twsTopProcSetAddressTaskData* td = TwsTopProcSetAddressGetTaskData();

    if (!VmOverrideBdaddr(&td->addr))
    {
        td->attempts_remaining--;
        if (td->attempts_remaining)
        {
            MessageSendLater(TwsTopProcSetAddressGetTask(), PROC_SET_ADDR_INTERNAL_RETRY,
                             NULL, OVERRIDE_ATTEMPT_DELAY_MS);
        }
        return FALSE;
    }
    return TRUE;
}

static void twsTopology_ProcSetAddressHandleInternalRetry(void)
{
    twsTopProcSetAddressTaskData* td = TwsTopProcSetAddressGetTaskData();
    
    if (twsTopology_ProcSetAddressOverrideAddress())
    {
        /* retry succeeded, report success and procedure complete */
        twsTopology_ProcSetAddressSuccess();
    }
    else
    {
        /* still failing */

        if (!td->attempts_remaining)
        {
            /* used all retry attempts, if this is the first iteration then try to
             * force close any outstanding ACLs and run another set of retries */
            if (td->first_attempt)
            {
                DEBUG_LOG_INFO("twsTopology_ProcSetAddressHandleInternalRetry (%u) FAILED first attempt, cleaning connections again", OVERRIDE_BD_ADDR_MAX_ATTEMPTS);
                td->first_attempt = FALSE;
                td->attempts_remaining = OVERRIDE_BD_ADDR_MAX_ATTEMPTS;
                ConManagerTerminateAllAcls(TwsTopProcSetAddressGetTask());
            }
            else
            {
                /* used all retries and already cleaned connections and tried again,
                 * so really have failed this time */
                DEBUG_LOG_INFO("twsTopology_ProcSetAddressHandleInternalRetry failed set address after 2nd close all attempt"); 
                Procedures_DelayedCompleteCfmCallback(td->complete_fn, tws_topology_procedure_set_address,
                        procedure_result_failed);
                twsTopology_ProcSetAddressReset();
            }
        }
        /* still attempts remaining, wait for the retry message to arrive */
    }
}

void TwsTopology_ProcedureSetAddressStart(Task result_task,
                                          procedure_start_cfm_func_t proc_start_cfm_fn,
                                          procedure_complete_func_t proc_complete_fn,
                                          Message goal_data)
{
    twsTopProcSetAddressTaskData* td = TwsTopProcSetAddressGetTaskData();
    SET_ADDRESS_TYPE_T* type = (SET_ADDRESS_TYPE_T*)goal_data;

    UNUSED(result_task);
    PanicNull(type);

    DEBUG_LOG("TwsTopology_ProcedureSetAddressStart");

    /* setup the procedure */
    twsTopology_ProcSetAddressReset();
    td->complete_fn = proc_complete_fn;
    if (type->primary)
    {
        PanicFalse(appDeviceGetPrimaryBdAddr(&td->addr));
        DEBUG_LOG_INFO("TwsTopology_ProcedureSetAddressStart, SWITCHING TO PRIMARY ADDRESS %04x,%02x,%06lx",
                   td->addr.nap, td->addr.uap, td->addr.lap);
    }
    else
    {
        PanicFalse(appDeviceGetSecondaryBdAddr(&td->addr));
        DEBUG_LOG_INFO("TwsTopology_ProcedureSetAddressStart, SWITCHING TO SECONDARY ADDRESS %04x,%02x,%06lx",
                   td->addr.nap, td->addr.uap, td->addr.lap);
    }
    proc_start_cfm_fn(tws_topology_procedure_set_address, procedure_result_success);

    TimestampEvent(TIMESTAMP_EVENT_ADDRESS_SWAP_STARTED);

    /* attempt address override for first time */
    if (twsTopology_ProcSetAddressOverrideAddress())
    {
        /* worked, report success and procedure complete already */
        twsTopology_ProcSetAddressSuccess();
    }
    else
    {
        DEBUG_LOG_INFO("TwsTopology_ProcedureSetAddressStart starting delayed retries to set address");
    }
}

void TwsTopology_ProcedureSetAddressCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    DEBUG_LOG("TwsTopology_ProcedureSetAddressCancel");
    twsTopology_ProcSetAddressReset();
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn, tws_topology_procedure_set_address, procedure_result_success);
}

static void twsTopology_ProcSetAddressHandleCloseAllCfm(void)
{
    DEBUG_LOG("twsTopology_ProcSetAddressHandleCloseAllCfm");
    
    /* Bluestack assures us all ACLs are closed, try address override again */
    if (twsTopology_ProcSetAddressOverrideAddress())
    {
        /* worked, report success and procedure complete already */
        twsTopology_ProcSetAddressSuccess();
    }

    /* if override failed retry timer message is automatically sent */
}

static void twsTopology_ProcSetAddressHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    /* guard against late message arrivel after the procedure has completed */
    if (!TwsTopProcSetAddressGetTaskData()->complete_fn)
    {
        return;
    }

    switch (id)
    {
        case CON_MANAGER_CLOSE_ALL_CFM:
            twsTopology_ProcSetAddressHandleCloseAllCfm();
            break;

        case PROC_SET_ADDR_INTERNAL_RETRY:
            twsTopology_ProcSetAddressHandleInternalRetry();
            break;

        default:
            break;
    }
}
