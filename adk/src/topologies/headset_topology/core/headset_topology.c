    /*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Headset Topology component core.
*/

#include "headset_topology.h"
#include "headset_topology_private.h"
#include "headset_topology_config.h"
#include "headset_topology_rules.h"
#include "headset_topology_goals.h"

#include "core/headset_topology_rules.h"

#include <logging.h>

#include <handset_service.h>
#include <bredr_scan_manager.h>
#include <connection_manager.h>
#include <power_manager.h>
#include <pairing.h>


/*! Instance of the headset Topology. */
headsetTopologyTaskData headset_topology = {0};

/*! \brief Generate handset related Connection events */
static void headsetTopology_HandleHandsetServiceConnectedInd(const HANDSET_SERVICE_CONNECTED_IND_T* ind)
{
    DEBUG_LOG("headsetTopology_HandleHandsetConnectedInd %04x,%02x,%06lx", ind->addr.nap,
                                                                           ind->addr.uap,
                                                                           ind->addr.lap);

    HeadsetTopologyRules_SetEvent(HSTOP_RULE_EVENT_HANDSET_CONNECTED_BREDR);
}


static void headsetTopology_HandlePairingActivity(const PAIRING_ACTIVITY_T *message)
{
    UNUSED(message);
    DEBUG_LOG("headsetTopology_HandlePairingActivity status=%d", message->status);
}

/*! \brief Take action following power's indication to prepare for sleep.*/
static void headsetTopology_HandlePowerSleepPrepareInd(void)
{
    DEBUG_LOG("headsetTopology_HandlePowerSleepPrepareInd ");
    /* TODO goto sleep role */
    appPowerSleepPrepareResponse(HeadsetTopologyGetTask());
}

/*! \brief Take action following power's indication of cancelling of sleep. */
static void headsetTopology_HandlePowerSleepCancelledInd(void)
{
    DEBUG_LOG("headsetTopology_HandlePowerSleepCancelledInd ");
    /* TODO leave the sleep role */
}

/*! \brief Take action following power's indication of imminent shutdown.*/
static void headsetTopology_HandlePowerShutdownPrepareInd(void)
{
    headsetTopologyTaskData *headset_taskdata = HeadsetTopologyGetTaskData();

    DEBUG_LOG("headsetTopology_HandlePowerShutdownPrepareInd");
    /* Headset should stop being connectable during shutdown. */
    headset_taskdata->shutdown_in_progress = TRUE;
    appPowerShutdownPrepareResponse(HeadsetTopologyGetTask());
}

/*! \brief Take action following power's indication of cancelling of shutdown. */
static void headsetTopology_HandlePowerShutdownCancelledInd(void)
{
    headsetTopologyTaskData *headset_taskdata = HeadsetTopologyGetTaskData();

    DEBUG_LOG("headsetTopology_HandlePowerShutdownCancelledInd");
    /* Headset can be connectable if the shutdown was cancelled */
    headset_taskdata->shutdown_in_progress = FALSE;
}

/*! \brief Generate handset related disconnection events . */
static void headsetTopology_HandleHandsetServiceDisconnectedInd(const HANDSET_SERVICE_DISCONNECTED_IND_T* ind)
{
    DEBUG_LOG("headsetTopology_HandleHandsetServiceDisconnectedInd %04x,%02x,%06lx status %u", ind->addr.nap,
                                                                                               ind->addr.uap,
                                                                                               ind->addr.lap,
                                                                                               ind->status);

    if(ind->status == handset_service_status_link_loss)
    {
        HeadsetTopologyRules_SetEvent(HSTOP_RULE_EVENT_HANDSET_LINKLOSS);
    }

    else if(ind->status == handset_service_status_disconnected)
    {
        HeadsetTopologyRules_SetEvent(HSTOP_RULE_EVENT_HANDSET_DISCONNECTED_BREDR);
    }
}


/*! \brief Headset Topology message handler.
 */
static void headsetTopology_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("headsetTopology_HandleMessage. message id %d(0x%x)",id,id);

    /* push goal decisions into goal engine */
    if((HEADSETTOP_INTERNAL_RULE_MSG_BASE <= id) && (id < HSTOP_GOAL_NOP))
    {
        HeadsetTopology_HandleGoalDecision(task, id, message);
        return;
    }

    /* handle all other messages */
    switch (id)
    {
        case PAIRING_ACTIVITY:
            headsetTopology_HandlePairingActivity(message);
            break;

        case HANDSET_SERVICE_CONNECTED_IND:
            headsetTopology_HandleHandsetServiceConnectedInd((HANDSET_SERVICE_CONNECTED_IND_T*)message);
            break;

        case HANDSET_SERVICE_DISCONNECTED_IND:
            DEBUG_LOG("headsetTopology_HandleMessage: HANDSET_SERVICE_DISCONNECTED_IND");
            headsetTopology_HandleHandsetServiceDisconnectedInd((HANDSET_SERVICE_DISCONNECTED_IND_T*)message);
            break;

        case CON_MANAGER_CONNECTION_IND:
            /*  TODO Check on what needs to be done here */
            DEBUG_LOG("headsetTopology_HandleMessage. Unhandled CON_MANAGER_CONNECTION_IND message %d(0x%x)",id,id);
            break;

        /* Power indications */
        case APP_POWER_SLEEP_PREPARE_IND:
            /* TODO Add handling for APP_POWER_SLEEP_PREPARE_IND from power manager. */
            headsetTopology_HandlePowerSleepPrepareInd();
            break;

        case APP_POWER_SLEEP_CANCELLED_IND:
            headsetTopology_HandlePowerSleepCancelledInd();
            break;

        case APP_POWER_SHUTDOWN_PREPARE_IND:
            headsetTopology_HandlePowerShutdownPrepareInd();
            DEBUG_LOG("headsetTopology_HandleMessage APP_POWER_SHUTDOWN_PREPARE_IND");
            break;

        case APP_POWER_SHUTDOWN_CANCELLED_IND:
            headsetTopology_HandlePowerShutdownCancelledInd();
            DEBUG_LOG("headsetTopology_HandleMessage APP_POWER_SHUTDOWN_CANCELLED_IND");
            break;

        default:
            DEBUG_LOG("headsetTopology_HandleMessage. Unhandled message %d(0x%x)",id,id);
            break;
      }
}


bool HeadsetTopology_Init(Task init_task)
{
    UNUSED(init_task);

    headsetTopologyTaskData *headset_taskdata = HeadsetTopologyGetTaskData();
    headset_taskdata->task.handler = headsetTopology_HandleMessage;
    headset_taskdata->prohibit_connect_to_handset = FALSE;
    headset_taskdata->start_status = FALSE;
    headset_taskdata->shutdown_in_progress = FALSE;

    /*Initialize Headset topology's goals and rules */
    HeadsetTopologyRules_Init(NULL);
    HeadsetTopology_GoalsInit();

    /* Register with power to receive sleep/shutdown messages. */
    appPowerClientRegister(HeadsetTopologyGetTask());
    /* Allow topology to sleep */
    appPowerClientAllowSleep(HeadsetTopologyGetTask());

    /* register with handset service as we need disconnect and connect notification */
    HandsetService_ClientRegister(HeadsetTopologyGetTask());
    ConManagerRegisterConnectionsClient(HeadsetTopologyGetTask());
    Pairing_ActivityClientRegister(HeadsetTopologyGetTask());
    BredrScanManager_PageScanParametersRegister(&hs_page_scan_params);
    BredrScanManager_InquiryScanParametersRegister(&hs_inquiry_scan_params);

    TaskList_InitialiseWithCapacity(HeadsetTopologyGetMessageClientTasks(), MESSAGE_CLIENT_TASK_LIST_INIT_CAPACITY);

    return TRUE;
}


bool HeadsetTopology_Start(void)
{
    DEBUG_LOG("HeadsetTopology_Start: Headset topology started");

    /* Set the rule to get the headset rolling (EnableConnectable, AllowHandsetConnect) */
    HeadsetTopologyRules_SetEvent(HSTOP_RULE_EVENT_START);

    /*! TODO Record topology start status. This is also to be updated when System state comes in */

    return TRUE;
}


void HeadsetTopology_RegisterMessageClient(Task client_task)
{
   TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(HeadsetTopologyGetMessageClientTasks()), client_task);
}


void HeadsetTopology_UnRegisterMessageClient(Task client_task)
{
   TaskList_RemoveTask(TaskList_GetFlexibleBaseTaskList(HeadsetTopologyGetMessageClientTasks()), client_task);
}


void HeadsetTopology_ProhibitHandsetConnection(bool prohibit)
{
    HeadsetTopologyGetTaskData()->prohibit_connect_to_handset = prohibit;

    if(prohibit)
    {
        HeadsetTopologyRules_SetEvent(HSTOP_RULE_EVENT_PROHIBIT_CONNECT_TO_HANDSET);
    }
    return;
}

