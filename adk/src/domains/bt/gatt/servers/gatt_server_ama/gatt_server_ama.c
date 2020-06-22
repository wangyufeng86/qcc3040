/****************************************************************************
Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_gatt_server_ama.c

DESCRIPTION
    Routines to handle messages sent from the GATT AMA Server Task.
*/

#include <logging.h>
#include <csrtypes.h>
#include <message.h>
#include <vm.h>

#include <gatt_handler.h>
#include <connection_manager.h>
#include <gatt_connect.h>
#include "gatt_handler_db.h"

#ifdef INCLUDE_AMA_LE
#include <ama.h>
#include <ama_private.h>
#include <ama_protocol.h>
#include <gatt_server_ama.h>
#include "gatt_ama_server.h"
#endif

#ifdef INCLUDE_AMA_LE

#define AMA_TX_MTU_SIZE    (178)

static void gattServerAma_Handler(Task task, MessageId id, Message message);
static void gattServerAma_OnConnection(uint16 cid);
static void gattServerAma_OnDisconnection(uint16 cid);

static TaskData amaTask = {gattServerAma_Handler};

static GAMASS gatt_ama_server_data;

static const gatt_connect_observer_callback_t connect_observer =
{
    .OnConnection = gattServerAma_OnConnection,
    .OnDisconnection = gattServerAma_OnDisconnection,
};

/*******************************************************************************/
bool GattServerAma_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("GattServerGaaComm_Init");

    GattConnect_RegisterObserver(&connect_observer);

    GattConnect_UpdateMinAcceptableMtu(AMA_TX_MTU_SIZE);

    return GattAmaServerInit(&gatt_ama_server_data, &amaTask,
                             HANDLE_AMA_ALEXA_SERVICE, HANDLE_AMA_ALEXA_SERVICE_END);
}

/******************************************************************************/
static void gattServerAma_Handler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    if(id == GATT_AMA_SERVER_WRITE_IND)
    {
        DEBUG_LOG(" GATT_AMA_SERVER_WRITE_IND = %d\n",id );
    }
    else if(id == GATT_AMA_SERVER_CLIENT_C_CFG)
    {
        uint16 client_config = ((GATT_AMA_SERVER_CLIENT_C_CFG_T*)message)->client_config;
        DEBUG_LOG(" GATT_AMA_SERVER_CLIENT_C_CFG = %d \n",client_config);

        if(client_config)
        {
            /* Send the AMA Version */
            AmaProtocol_TransportConnCfm(ama_transport_ble);

            // Save the public BD address of the connected client
            tp_bdaddr tpaddr = {0};

            if (VmGetBdAddrtFromCid(gatt_ama_server_data.cid, &tpaddr))
            {
                /* Request Quality of Service suitable for audio transmission */
                ConManagerRequestDeviceQos(&tpaddr, cm_qos_audio);
            }
        }        
    }
    else if(id == GATT_AMA_SERVER_INCOMING_DATA)
    {
        AmaProtocol_ParseData(((GATT_AMA_SERVER_INCOMING_DATA_T *)message)->value,
                              ((GATT_AMA_SERVER_INCOMING_DATA_T *)message)->size_value);
    }
}

/******************************************************************************/
static void gattServerAma_OnConnection(uint16 cid)
{
    if (gatt_ama_server_data.cid == 0)
    {
        gatt_ama_server_data.cid = cid;
    }

    DEBUG_LOG("gattServerAma_OnConnection cid - %d\n", cid);
}

/******************************************************************************/
static void gattServerAma_OnDisconnection(uint16 cid)
{
    if(gatt_ama_server_data.cid == cid)
    {
        gatt_ama_server_data.cid = 0;
        Ama_TransportDisconnected();

        tp_bdaddr tpaddr;
        if (VmGetBdAddrtFromCid(cid, &tpaddr))
        {
            /* Release Quality of Service suitable for audio transmission */
            ConManagerReleaseDeviceQos(&tpaddr, cm_qos_audio);
        }
    }
    DEBUG_LOG("gattServerAma_OnDisconnection cid  %04x\n",cid);
}

#endif /* INCLUDE_AMA_LE */
