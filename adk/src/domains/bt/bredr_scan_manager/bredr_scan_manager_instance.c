/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\defgroup   bredr_scan_manager
\brief	    Implementation of BREDR scan instance (page or inquiry)
*/
/*!
@startuml
TITLE Internal state diagram (per scan type (page/inquiry))

[*] --> SCAN_DISABLED : Init

SCAN_ENABLING : Has clients and not paused\nOnEntry - ConnectionWriteScanEnable("TRUE")
SCAN_DISABLING : No clients or paused\nOnEntry - ConnectionWriteScanEnable("FALSE")
SCAN_DISABLED : No clients or paused\nOnForceOffEntry - PAUSED_IND/CFM\nOnForceOffExit - RESUMED_IND/CFM
SCAN_ENABLED : Has clients and not paused

SCAN_DISABLED -RIGHT-> SCAN_ENABLING : ScanResume() or\nScanRequest()
SCAN_ENABLING -RIGHT-> SCAN_ENABLED : CL_DM_WRITE_SCAN_ENABLE_CFM\n(outstanding==0)
SCAN_ENABLED --> SCAN_DISABLING : ScanPause() or\nScanRelease()
SCAN_DISABLING --> SCAN_DISABLED : CL_DM_WRITE_SCAN_ENABLE_CFM\n(outstanding==0)
SCAN_DISABLING --> SCAN_ENABLING : ScanResume() or\nScanRequest()
SCAN_ENABLING --> SCAN_DISABLING : ScanPause() or\nScanRelease()

@enduml

*/

#include "bredr_scan_manager_private.h"
#include "connection.h"
#include "logging.h"
#include "message.h"

/*! \brief Helper function to write the task's scan type to the task list data. */
static inline void bredrScanManager_ListDataSet(task_list_data_t *data,
                                                bredr_scan_manager_scan_type_t type)
{
    PanicFalse(type <= SCAN_MAN_PARAMS_TYPE_MAX);
    data->s8 = (int8)type;
}

/*! \brief Helper function to read the task's scan type from the task list data. */
static inline bredr_scan_manager_scan_type_t bredrScanManager_ListDataGet(const task_list_data_t *data)
{
    return (bredr_scan_manager_scan_type_t)(data->s8);
}

/*! \brief Wraps up logic that decides whether to send a scan paused indication. */
static void bredrScanManager_ConditionallySendPausedInd(bsm_scan_context_t *context)
{
    if (context->state == BSM_SCAN_DISABLED)
    {
        if (bredrScanManager_IsDisabled())
        {
            MessageId id;
            task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

            id = BREDR_SCAN_MANAGER_PAGE_SCAN_PAUSED_IND + context->message_offset;
            TaskList_MessageSendId(clients, id);
        }
    }
}

/*! \brief Set a new scan state. */
static void bredrScanManager_InstanceSetState(bsm_scan_context_t *context,
                                              bsm_scan_enable_state_t new_state)
{
    context->state = new_state;

    if (new_state & (BSM_SCAN_DISABLING | BSM_SCAN_ENABLING))
    {
        /* Disabling or enabling scanning requires a new call to the
           connection library ConnectionWriteScanEnable function, which may
           enable or disable scanning */
        bredrScanManager_ConnectionWriteScanEnable();
    }

    /* Indication may need to be sent when state transitions to disabled */
    bredrScanManager_ConditionallySendPausedInd(context);
}

/*! \brief Compare present and new required scan parameters and call the
           connection library function to change parameters if they differ */
static void bredrScanManager_InstanceUpdateScanActivity(bsm_scan_context_t *context,
                                                        bredr_scan_manager_scan_type_t type)
{
    const bredr_scan_manager_scan_parameters_t *params;

    PanicNull((void*)context->params);

    params = (const bredr_scan_manager_scan_parameters_t *)&context->params->sets[context->params_index].set_type[type];

    if (0 != memcmp(params, &context->scan_params, sizeof(*params)))
    {
        context->scan_params = *params;
        bredrScanManager_ConnectionWriteScanActivity(context);
    }
}

/*! \brief This function provides a simple boolean goal-based API. Callers
           can ask this function to enable or disable scanning with a required
           scan type. This function handles inspecting the current scan state
           and setting state appropriately.

           State changes made as a result of calling this function will result
           in new calls to the connection library to enable/disable scanning and
           change scan parameters.
*/
static void bredrScanManager_InstanceSetGoal(bsm_scan_context_t *context, bool enable,
                                             bredr_scan_manager_scan_type_t type)
{
    if (enable)
    {
        bredrScanManager_InstanceUpdateScanActivity(context, type);
    }

    if (context->state & (BSM_SCAN_DISABLED | BSM_SCAN_DISABLING))
    {
        if (enable)
        {
            bredrScanManager_InstanceSetState(context, BSM_SCAN_ENABLING);
        }
    }
    else if (context->state & (BSM_SCAN_ENABLED | BSM_SCAN_ENABLING))
    {
        if (!enable)
        {
            bredrScanManager_InstanceSetState(context, BSM_SCAN_DISABLING);
        }
    }
}

/*! \brief Iteration handler that determines the maximum scan type requested by all clients. */
static bool bredrScanManager_IterateFindMaxType(Task task, task_list_data_t *data, void *arg)
{
    bredr_scan_manager_scan_type_t *max_type = (bredr_scan_manager_scan_type_t *)arg;
    bredr_scan_manager_scan_type_t this_type = bredrScanManager_ListDataGet(data);

    *max_type = MAX(*max_type, this_type);

    UNUSED(task);

    /* End iteration if max type is found - cannot be exceeded */
    return (this_type != SCAN_MAN_PARAMS_TYPE_MAX);
}

/*! \brief This function is called whenever the scan client/pause/param state
    changes. It evaluates the requirements of the current set of clients
    (including paused) and sets a goal, #bredrScanManager_InstanceSetGoal
    decides whether any changes to state are required */
static void bredrScanManager_InstanceRefresh(bsm_scan_context_t *context)
{
    bredr_scan_manager_scan_type_t max_type = SCAN_MAN_PARAMS_TYPE_SLOW;
    bool goal = TRUE;
    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

    if (bredrScanManager_IsDisabled() || (TaskList_Size(clients) == 0))
    {
        goal = FALSE;
    }
    else
    {
        TaskList_IterateWithDataRawFunction(clients, bredrScanManager_IterateFindMaxType,
                                            (void*)&max_type);
    }
    bredrScanManager_InstanceSetGoal(context, goal, max_type);
}

void bredrScanManager_InstanceInit(bsm_scan_context_t *context, uint8 message_offset)
{
    context->state = BSM_SCAN_DISABLED;
    TaskList_WithDataInitialise(&context->clients);
    context->message_offset = message_offset;
}

void bredrScanManager_InstanceParameterSetRegister(bsm_scan_context_t *context,
                                                   const bredr_scan_manager_parameters_t *params)
{
    PanicNull((void*)params);
    PanicNull((void*)params->sets);
    context->params = params;
}

void bredrScanManager_InstanceParameterSetSelect(bsm_scan_context_t *context, uint8 index)
{
    PanicFalse(index < context->params->len);
    context->params_index = index;
    bredrScanManager_InstanceRefresh(context);
}

void bredrScanManager_InstanceClientAddOrUpdate(bsm_scan_context_t *context, Task client,
                                                bredr_scan_manager_scan_type_t type)
{
    task_list_data_t data;
    task_list_t *list = TaskList_GetBaseTaskList(&context->clients);

    bredrScanManager_ListDataSet(&data, type);

    if (TaskList_IsTaskOnList(list, client))
    {
        PanicFalse(TaskList_SetDataForTask(list, client, &data));
    }
    else
    {
        PanicFalse(TaskList_AddTaskWithData(list, client, &data));
        /* The new client needs to be informed if scanning is paused */
        bredrScanManager_ConditionallySendPausedInd(context);
    }
    bredrScanManager_InstanceRefresh(context);
}

void bredrScanManager_InstanceClientRemove(bsm_scan_context_t *context, Task client)
{
    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);
    if (TaskList_RemoveTask(clients, client))
    {
        bredrScanManager_InstanceRefresh(context);
    }
}

bool bredrScanManager_InstanceIsScanEnabledForClient(bsm_scan_context_t *context, Task client)
{
    task_list_data_t *data;
    bool enabled = FALSE;
    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

    if (TaskList_GetDataForTaskRaw(clients, client, &data))
    {
        bredr_scan_manager_scan_type_t type = bredrScanManager_ListDataGet(data);
        enabled = (type == SCAN_MAN_PARAMS_TYPE_SLOW || type == SCAN_MAN_PARAMS_TYPE_FAST);
    }

    return enabled;
}

void bredrScanManager_InstancePause(bsm_scan_context_t *context)
{
    /* PAUSED_IND is sent to client when state is set to disabled */
    bredrScanManager_InstanceRefresh(context);
}

void bredrScanManager_InstanceResume(bsm_scan_context_t *context)
{
    task_list_t *clients = TaskList_GetBaseTaskList(&context->clients);

    /* Only send resume message if pause has completed. This avoids sending
       RESUMED_IND if PAUSED_IND has not been sent */
    if (context->state == BSM_SCAN_DISABLED)
    {
        MessageId id;
        id = BREDR_SCAN_MANAGER_PAGE_SCAN_RESUMED_IND + context->message_offset;
        TaskList_MessageSendId(clients, id);
    }

    bredrScanManager_InstanceRefresh(context);
}

void bredrScanManager_InstanceCompleteTransition(bsm_scan_context_t *context)
{
    if (context->state & BSM_SCAN_DISABLING)
    {
        bredrScanManager_InstanceSetState(context, BSM_SCAN_DISABLED);
    }
    else if (context->state & BSM_SCAN_ENABLING)
    {
        bredrScanManager_InstanceSetState(context, BSM_SCAN_ENABLED);
    }
}
