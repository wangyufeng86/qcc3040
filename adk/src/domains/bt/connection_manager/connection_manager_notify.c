/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#include "connection_manager.h"
#include "connection_manager_data.h"
#include "connection_manager_notify.h"

#include <logging.h>

#include <message.h>
#include <panic.h>

/*! @{ Macros to make connection manager messages. */
#define MAKE_CONMAN_MESSAGE(TYPE) TYPE##_T *message = PanicUnlessNew(TYPE##_T);
/*! @} */

#define CONNECTION_MANAGER_CONNECTION_TASK_LIST_INIT_CAPACITY 8
#define CONNECTION_MANAGER_BREDR_OBSERVER_TASK_LIST_INIT_CAPACITY 1
#define CONNECTION_MANAGER_BLE_OBSERVER_TASK_LIST_INIT_CAPACITY 1

/*! List of client tasks which have registered to receive CON_MANAGER_CONNECTION_IND
    messages */
static TASK_LIST_WITH_INITIAL_CAPACITY(CONNECTION_MANAGER_CONNECTION_TASK_LIST_INIT_CAPACITY) connection_client_tasks;
static TASK_LIST_WITH_INITIAL_CAPACITY(CONNECTION_MANAGER_BREDR_OBSERVER_TASK_LIST_INIT_CAPACITY) bredr_observer_tasks;
static TASK_LIST_WITH_INITIAL_CAPACITY(CONNECTION_MANAGER_BLE_OBSERVER_TASK_LIST_INIT_CAPACITY) ble_observer_tasks;
static task_list_t handset_connection_observer_tasks;


#define ConManager_GetConnectionClientTasks() (task_list_flexible_t *)(&connection_client_tasks)
#define ConManager_GetBredrObserverTasks() (task_list_flexible_t *)(&bredr_observer_tasks)
#define ConManager_GetBleObserverTasks() (task_list_flexible_t *)(&ble_observer_tasks)


/******************************************************************************/
static void conManagerMsgConnectionInd(const tp_bdaddr *tpaddr, bool connected, hci_status reason)
{
    MAKE_CONMAN_MESSAGE(CON_MANAGER_CONNECTION_IND);
    message->bd_addr = tpaddr->taddr.addr;
    message->connected = connected;
    message->ble = (tpaddr->transport == TRANSPORT_BLE_ACL);
    message->reason = reason;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(ConManager_GetConnectionClientTasks()), CON_MANAGER_CONNECTION_IND, message);
}

/******************************************************************************/
static task_list_t* conManagerGetTaskListForTransport(TRANSPORT_T transport)
{
    if(transport == TRANSPORT_BLE_ACL)
        return &ble_observer_tasks;

    return &bredr_observer_tasks;
}

/******************************************************************************/
static void conManagerMsgTpConnectionInd(const tp_bdaddr *tpaddr, bool incoming)
{
    task_list_t* list = conManagerGetTaskListForTransport(tpaddr->transport);

    MAKE_CONMAN_MESSAGE(CON_MANAGER_TP_CONNECT_IND);
    message->tpaddr = *tpaddr;
    message->incoming = incoming;
    TaskList_MessageSend(list, CON_MANAGER_TP_CONNECT_IND, message);
}

/******************************************************************************/
static void conManagerMsgTpDisconnectionInd(const tp_bdaddr *tpaddr, hci_status reason)
{
    task_list_t* list = conManagerGetTaskListForTransport(tpaddr->transport);

    MAKE_CONMAN_MESSAGE(CON_MANAGER_TP_DISCONNECT_IND);
    message->tpaddr = *tpaddr;
    message->reason = reason;
    TaskList_MessageSend(list, CON_MANAGER_TP_DISCONNECT_IND, message);
}

/******************************************************************************/
static void conManagerMsgTpDisconnectRequestedInd(const tp_bdaddr *tpaddr)
{
    task_list_t* list = conManagerGetTaskListForTransport(tpaddr->transport);

    MAKE_CONMAN_MESSAGE(CON_MANAGER_TP_DISCONNECT_REQUESTED_IND);
    message->tpaddr = *tpaddr;
    TaskList_MessageSend(list, CON_MANAGER_TP_DISCONNECT_REQUESTED_IND, message);
}

/******************************************************************************/
static task_list_t* conManagerGetTaskListForHansetConnection(void)
{
    return &handset_connection_observer_tasks;
}

/******************************************************************************/
static void conManagerMsgHandsetConnectionAllowInd(void)
{
    task_list_t* list = conManagerGetTaskListForHansetConnection();

    TaskList_MessageSendId(list, CON_MANAGER_HANDSET_CONNECT_ALLOW_IND);
}

/******************************************************************************/
static void conManagerMsgHandsetConnectionDisallowInd(void)
{
    task_list_t* list = conManagerGetTaskListForHansetConnection();

    TaskList_MessageSendId(list, CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND);
}

/******************************************************************************/
static void conManagerMsgParamsUpdateInd(const tp_bdaddr *tpaddr, uint16 conn_interval, uint16 conn_latency)
{
    MAKE_CONMAN_MESSAGE(CON_MANAGER_BLE_PARAMS_UPDATE_IND);
    message->tpaddr = *tpaddr;
    message->conn_interval = conn_interval;
    message->slave_latency = conn_latency;
    TaskList_MessageSend(TaskList_GetFlexibleBaseTaskList(ConManager_GetBleObserverTasks()), CON_MANAGER_BLE_PARAMS_UPDATE_IND, message);
}

/******************************************************************************/
void conManagerNotifyObservers(const tp_bdaddr *tpaddr, cm_notify_message_t notify_message, hci_status reason)
{
    switch (notify_message)
    {
        case cm_notify_message_connected_incoming:
        case cm_notify_message_connected_outgoing:
            conManagerMsgConnectionInd(tpaddr, TRUE, reason);
            conManagerMsgTpConnectionInd(tpaddr, 
                                         notify_message == cm_notify_message_connected_incoming);
            break;
        case cm_notify_message_disconnected:
            conManagerMsgConnectionInd(tpaddr, FALSE, reason);
            conManagerMsgTpDisconnectionInd(tpaddr, reason);
            break;
        case cm_notify_message_disconnect_requested:
            conManagerMsgTpDisconnectRequestedInd(tpaddr);
            break;

        default:
            /* Unhandled notification */
            Panic();
            break;
    }
}

/******************************************************************************/
void conManagerNotifyAllowedConnectionsObservers(con_manager_allowed_notify_t notify)
{
    switch (notify)
    {
        case cm_handset_allowed:
            conManagerMsgHandsetConnectionAllowInd();
            break;
        case cm_handset_disallowed:
            conManagerMsgHandsetConnectionDisallowInd();
            break;

        default:
            /* Unhandled notification */
            Panic();
            break;
    }
}

/******************************************************************************/
void conManagerNotifyConnParamsObservers(const tp_bdaddr *tpaddr, uint16 conn_interval, uint16 conn_latency)
{
    conManagerMsgParamsUpdateInd(tpaddr, conn_interval, conn_latency);
}


/******************************************************************************/
void conManagerNotifyInit(void)
{
    /* create a task list to track tasks interested in connection
     * event indications */
    TaskList_InitialiseWithCapacity(ConManager_GetConnectionClientTasks(), CONNECTION_MANAGER_CONNECTION_TASK_LIST_INIT_CAPACITY);
    TaskList_InitialiseWithCapacity(ConManager_GetBredrObserverTasks(), CONNECTION_MANAGER_BREDR_OBSERVER_TASK_LIST_INIT_CAPACITY);
    TaskList_InitialiseWithCapacity(ConManager_GetBleObserverTasks(), CONNECTION_MANAGER_BLE_OBSERVER_TASK_LIST_INIT_CAPACITY);
    TaskList_Initialise(&handset_connection_observer_tasks);
}

/******************************************************************************/
void ConManagerRegisterConnectionsClient(Task client_task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetConnectionClientTasks()), client_task);
}

/******************************************************************************/
void ConManagerUnregisterConnectionsClient(Task client_task)
{
    TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetConnectionClientTasks()), client_task);
}

/******************************************************************************/
void ConManagerRegisterTpConnectionsObserver(cm_transport_t transport_mask, Task client_task)
{
    if((transport_mask & cm_transport_bredr) == cm_transport_bredr)
        TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetBredrObserverTasks()), client_task);

    if((transport_mask & cm_transport_ble) == cm_transport_ble)
        TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetBleObserverTasks()), client_task);
}

/******************************************************************************/
void ConManagerUnregisterTpConnectionsObserver(cm_transport_t transport_mask, Task client_task)
{
    if ((transport_mask & cm_transport_bredr) == cm_transport_bredr)
    {
        TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetBredrObserverTasks()), client_task);
    }

    if ((transport_mask & cm_transport_ble) == cm_transport_ble)
    {
        TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(ConManager_GetBleObserverTasks()), client_task);
    }
}

/******************************************************************************/
void ConManagerRegisterAllowedConnectionsObserver(Task client_task)
{
    TaskList_AddTask(&handset_connection_observer_tasks, client_task);
}

/******************************************************************************/
void ConManagerUnregisterAllowedConnectionsObserver(Task client_task)
{
    TaskList_RemoveTask(&handset_connection_observer_tasks, client_task);
}

