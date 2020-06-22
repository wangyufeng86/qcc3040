/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version
\file
\brief      Handling of the GAIA transport interface

This is a minimal implementation that only supports upgrade.
*/

/*! @todo VMCSA-3689 remove #ifdef */
#ifdef INCLUDE_DFU

#include "gaia_framework_internal.h"

#include "init.h"
#include "adk_log.h"
#include "gaia_framework_command.h"
#include "gatt_handler.h"
#include "gatt_handler_db_if.h"

#include <panic.h>


/*!< Task information for GAIA support */
gaiaTaskData    app_gaia;

/*! Enumerated type of internal message IDs used by this module */
typedef enum gaia_handler_internal_messages
{
        /*! Disconnect GAIA */
    APP_GAIA_INTERNAL_DISCONNECT = INTERNAL_MESSAGE_BASE,
}gaia_handler_internal_messages_t;

#define appGaiaGetDisconnectResponseCallbackFn()    GaiaGetTaskData()->disconnect.response
#define appGaiaGetDisconnectResponseCallbackCid()   GaiaGetTaskData()->disconnect.cid


static void gaiaFrameworkInternal_GattConnect(uint16 cid);
static void gaiaFrameworkInternal_GattDisconnect(uint16 cid);
static void gaiaFrameworkInternal_GattDisconnectRequested(uint16 cid, gatt_connect_disconnect_req_response response);
static void gaiaFrameworkInternal_MessageHandler(Task task, MessageId id, Message message);


static const gatt_connect_observer_callback_t gatt_gaia_observer_callback =
{
    .OnConnection = gaiaFrameworkInternal_GattConnect,
    .OnDisconnection = gaiaFrameworkInternal_GattDisconnect,
    .OnDisconnectRequested = gaiaFrameworkInternal_GattDisconnectRequested
};


/********************  PUBLIC FUNCTIONS  **************************/

/*! Initialise the GAIA Module */
bool GaiaFrameworkInternal_Init(Task init_task)
{
    gaiaTaskData *this = GaiaGetTaskData();

    this->gaia_task.handler = gaiaFrameworkInternal_MessageHandler;
    TaskList_InitialiseWithCapacity(GaiaGetClientList(), GAIA_CLIENT_TASK_LIST_INIT_CAPACITY);
    this->connections_allowed = TRUE;

    GaiaInit(GaiaGetTask(), 1);

    GaiaSetAppVersion(gaia_app_version_3);

    GaiaSetV3AppCommandHandler(GaiaFrameworkCommand_CommandHandler);

    Init_SetInitTask(init_task);
    return TRUE;
}


void GaiaFrameworkInternal_ClientRegister(Task task)
{
    TaskList_AddTask(TaskList_GetFlexibleBaseTaskList(GaiaGetClientList()), task);
}


static void gaiaFrameworkInternal_SendInitCfm(void)
{
    MessageSend(Init_GetInitTask(), APP_GAIA_INIT_CFM, NULL);
}

static void gaiaFrameworkInternal_SetDisconnectResponseCallback(uint16 cid, gatt_connect_disconnect_req_response response)
{
    GaiaGetTaskData()->disconnect.response = response;
    GaiaGetTaskData()->disconnect.cid = cid;
}

static void gaiaFrameworkInternal_NotifyGaiaConnected(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GaiaGetClientList()), APP_GAIA_CONNECTED);
}


static void gaiaFrameworkInternal_NotifyGaiaDisconnected(void)
{
    gatt_connect_disconnect_req_response response = appGaiaGetDisconnectResponseCallbackFn();
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GaiaGetClientList()), APP_GAIA_DISCONNECTED);
    if (response)
    {
        response(appGaiaGetDisconnectResponseCallbackCid());
        gaiaFrameworkInternal_SetDisconnectResponseCallback(0, NULL);
    }
}


static void gaiaFrameworkInternal_NotifyUpgradeConnection(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GaiaGetClientList()), APP_GAIA_UPGRADE_CONNECTED);
}


static void gaiaFrameworkInternal_NotifyUpgradeDisconnection(void)
{
    TaskList_MessageSendId(TaskList_GetFlexibleBaseTaskList(GaiaGetClientList()), APP_GAIA_UPGRADE_DISCONNECTED);
}


static void gaiaFrameworkInternal_HandleInitConfirm(const GAIA_INIT_CFM_T *init_cfm)
{
    DEBUG_LOG("gaiaFrameworkInternal_HandleInitConfirm GAIA_INIT_CFM: %d (succ)",init_cfm->success);

    PanicFalse(init_cfm->success);

    GaiaSetApiMinorVersion(GAIA_API_MINOR_VERSION);
    GaiaSetAppWillHandleCommand(GAIA_COMMAND_DEVICE_RESET, TRUE);

    /* Start the GAIA transports.
       GATT transport doesn't actually need starting, but not harmful. If GAIA
       changes in future, may help identify a problem sooner */
    GaiaStartService(gaia_transport_spp);
    GaiaStartService(gaia_transport_gatt);

    /* Successful initialisation of the library. The application needs
     * this to unblock.
     */
    gaiaFrameworkInternal_SendInitCfm();
}


/*  Accept the GAIA connection if they are allowed, and inform any clients.
 */
static void gaiaFrameworkInternal_HandleConnectInd(const GAIA_CONNECT_IND_T *ind)
{
    GAIA_TRANSPORT *transport;

    if (!ind || !ind->success)
    {
        DEBUG_LOG("gaiaFrameworkInternal_HandleConnectInd Success = FAILED");
        return;
    }

    transport = ind->transport;

    if (!transport)
    {
        DEBUG_LOG("gaiaFrameworkInternal_HandleConnectInd No transport");

        /* Can't disconnect nothing so just return */
        return;
    }

    if (!GaiaGetTaskData()->connections_allowed)
    {
        DEBUG_LOG("gaiaFrameworkInternal_HandleConnectInd GAIA not allowed");
        GaiaDisconnectRequest(transport);
        return;
    }

    DEBUG_LOG("gaiaFrameworkInternal_HandleConnectInd Success. Transport:%p",transport);

    GaiaSetTransport(transport);

    GaiaSetSessionEnable(transport, TRUE);
    GaiaOnTransportConnect(transport);

    gaiaFrameworkInternal_NotifyGaiaConnected();
}


static void gaiaFrameworkInternal_HandleDisconnectInd(const GAIA_DISCONNECT_IND_T *ind)
{
    DEBUG_LOG("gaiaFrameworkInternal_HandleDisconnectInd. Transport %p",ind->transport);

        /* GAIA can send IND with a NULL transport. Seemingly after we
           requested a disconnect (?) */
    if (ind->transport)
    {
        GaiaDisconnectResponse(ind->transport);
        GaiaSetTransport(NULL);
    }
    gaiaFrameworkInternal_NotifyGaiaDisconnected();
}


static void gaiaFrameworkInternal_InternalDisconnect(void)
{
    GAIA_TRANSPORT *transport = GaiaGetTransport();

    if (transport)
    {
        GaiaDisconnectRequest(transport);
        GaiaSetTransport(NULL);
    }
}


static void gaiaFrameworkInternal_MessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    DEBUG_LOG("gaiaFrameworkInternal_MessageHandler 0x%X (%d)",id,id);

    switch (id)
    {
        case APP_GAIA_INTERNAL_DISCONNECT:
            gaiaFrameworkInternal_InternalDisconnect();
            break;

        case GAIA_INIT_CFM:
            gaiaFrameworkInternal_HandleInitConfirm((const GAIA_INIT_CFM_T*)message);
            break;

        case GAIA_CONNECT_IND:                   /* Indication of an inbound connection */
            gaiaFrameworkInternal_HandleConnectInd((const GAIA_CONNECT_IND_T *)message);
            break;

        case GAIA_DISCONNECT_IND:                /* Indication that the connection has closed */
            gaiaFrameworkInternal_HandleDisconnectInd((const GAIA_DISCONNECT_IND_T *)message);
            break;

        case GAIA_DISCONNECT_CFM:                /* Confirmation that a requested disconnection has completed */
            /* We probably want to take note of this to send an event to the state
               machine, but it is mainly upgrade we care about. Not gaia connections. */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_DISCONNECT_CFM");
            GaiaSetTransport(NULL);
            break;

        case GAIA_START_SERVICE_CFM:             /* Confirmation that a Gaia server has started */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_START_SERVICE_CFM (nothing to do)");
            break;

        case GAIA_DEBUG_MESSAGE_IND:             /* Sent as a result of a GAIA_COMMAND_SEND_DEBUG_MESSAGE command */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_DEBUG_MESSAGE_IND");
            break;

        case GAIA_UNHANDLED_COMMAND_IND:         /* Indication that an unhandled command has been received */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_UNHANDLED_COMMAND_IND");
            GaiaFrameworkCommand_CommandHandler(task, (const GAIA_UNHANDLED_COMMAND_IND_T *) message);
            break;

        case GAIA_SEND_PACKET_CFM:               /* Confirmation that a GaiaSendPacket request has completed */
            {
                const GAIA_SEND_PACKET_CFM_T *m = (const GAIA_SEND_PACKET_CFM_T *) message;
                DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_SEND_PACKET_CFM");

                free(m->packet);
            }
            break;

        case GAIA_DFU_CFM:                       /* Confirmation of a Device Firmware Upgrade request */
            /* If the confirm is a fail, then we can raise an error, but
               not much to do */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_DFU_CFM");
            break;

        case GAIA_DFU_IND:                       /* Indication that a Device Firmware Upgrade has begun */
            /* This could be used to update the link policy for faster
               data transfer. */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_DFU_IND");
            break;

        case GAIA_UPGRADE_CONNECT_IND:           /* Indication of VM Upgrade successful connection */
            /* This tells us the type of transport connection made, so we can
               remember it if needed */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_UPGRADE_CONNECT_IND");
            gaiaFrameworkInternal_NotifyUpgradeConnection();
            break;

        case GAIA_UPGRADE_DISCONNECT_IND:
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_UPGRADE_DISCONNECT_IND");
            gaiaFrameworkInternal_NotifyUpgradeDisconnection();
            break;

        case GAIA_CONNECT_CFM:                   /* Confirmation of an outbound connection request */
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_CONNECT_CFM Unexpected");
            break;

        case GAIA_VA_START_CFM:
        case GAIA_VA_DATA_REQUEST_IND:
        case GAIA_VA_VOICE_END_CFM:
        case GAIA_VA_VOICE_END_IND:
        case GAIA_VA_ANSWER_START_IND:
        case GAIA_VA_ANSWER_END_IND:
        case GAIA_VA_CANCEL_CFM:
        case GAIA_VA_CANCEL_IND:
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler GAIA_VA_... Unexpected");
            break;


        default:
            DEBUG_LOG("gaiaFrameworkInternal_MessageHandler Unknown GAIA message 0x%x (%d)",id,id);
            break;
    }
}

static void gaiaFrameworkInternal_GattConnect(uint16 cid)
{
    if (GaiaGetTaskData()->connections_allowed)
    {
        GaiaConnectGatt(cid);
    }
}

static void gaiaFrameworkInternal_GattDisconnect(uint16 cid)
{
    GaiaDisconnectGatt(cid);
}

static void gaiaFrameworkInternal_GattDisconnectRequested(uint16 cid, gatt_connect_disconnect_req_response response)
{
    gaiaFrameworkInternal_SetDisconnectResponseCallback(cid, response);
    GaiaDisconnectGatt(cid);
}


/*! \brief Disconnect any active gaia connection
 */
void gaiaFrameworkInternal_GaiaDisconnect(void)
{
    MessageSend(GaiaGetTask(), APP_GAIA_INTERNAL_DISCONNECT, NULL);
}


void gaiaFrameworkInternal_AllowNewConnections(bool allow)
{
    GaiaGetTaskData()->connections_allowed = allow;
}


bool gaiaFrameworkInternal_GattServerInit(Task init_task)
{
    UNUSED(init_task);

    GaiaStartGattServer(HANDLE_GAIA_SERVICE, HANDLE_GAIA_SERVICE_END);

    GattConnect_RegisterObserver(&gatt_gaia_observer_callback);

    DEBUG_LOG("gaiaFrameworkInternal_GattServerInit. Server initialised");

    return TRUE;
}


#endif /* INCLUDE_DFU */
