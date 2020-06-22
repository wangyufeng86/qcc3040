/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Application support for GATT connect/disconnect
*/

#include "gatt_connect.h"
#include "gatt_connect_observer.h"
#include "gatt_connect_list.h"
#include "gatt_connect_mtu.h"

#include <gatt.h>
#include <gatt_manager.h>
#include <logging.h>

#include <panic.h>
#include <app/bluestack/l2cap_prim.h>

#include <connection_manager.h>

static void gattConnect_MessageHandler(Task task, MessageId id, Message msg);

TaskData gatt_task_data = {gattConnect_MessageHandler};
Task gatt_connect_init_task;

static void gattConnect_HandleConnectInd(GATT_MANAGER_CONNECT_IND_T* ind)
{
    gatt_connection_t* connection;
    
    DEBUG_LOG("gattConnect_HandleConnectInd, cid 0x%04x flags 0x%04d", ind->cid, ind->flags);
    
    if(L2CA_CONFLAG_IS_LE(ind->flags))
    {
        connection = GattConnect_CreateConnection(ind->cid);
        
        if(connection)
        {
            GattConnect_SetMtu(connection, ind->mtu);
            GattConnect_SendExchangeMtuReq(&gatt_task_data, ind->cid);
            GattConnect_ObserverNotifyOnConnection(ind->cid);
            GattManagerConnectResponse(ind->cid, ind->flags, TRUE);
            return;
        }
    }
    
    GattManagerConnectResponse(ind->cid, ind->flags, FALSE);
}

static void gattConnect_HandleDisconnectInd(GATT_MANAGER_DISCONNECT_IND_T* ind)
{
    GattConnect_DestroyConnection(ind->cid);
    GattConnect_ObserverNotifyOnDisconnection(ind->cid);
}

static void gattConnect_HandleRegisterWithGattCfm(GATT_MANAGER_REGISTER_WITH_GATT_CFM_T* cfm)
{
    PanicFalse(cfm->status == gatt_manager_status_success);
    if(gatt_connect_init_task)
    {
        MessageSend(gatt_connect_init_task, GATT_CONNECT_SERVER_INIT_COMPLETE_CFM, NULL);
    }
}

static void gattConnect_HandleConManagerConnection(const CON_MANAGER_TP_CONNECT_IND_T *ind)
{
    if (ConManagerIsTpAclLocal(&ind->tpaddr))
    {
        GattManagerConnectAsCentral(&gatt_task_data, &ind->tpaddr.taddr, gatt_connection_ble_master_directed, TRUE);
    }
}

static void gattConnect_HandleConManagerDisconnection(const CON_MANAGER_TP_DISCONNECT_IND_T *ind)
{
    UNUSED(ind);
    GattManagerCancelConnectAsCentral(&gatt_task_data);
}

static void gattConnect_DisconnectRequestedResponse(uint16 cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    if (connection)
    {
        DEBUG_LOG("gattConnect_DisconnectRequestedResponse, cid 0x%04x pending_disconnects %d", cid, connection->pending_disconnects);
        if (connection->pending_disconnects)
        {
            connection->pending_disconnects--;
            if (connection->pending_disconnects == 0)
            {
                GattManagerDisconnectRequest(connection->cid);
            }
        }
    }
}

static void gattConnect_HandleConManagerDisconnectRequested(const CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T *ind)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromTpaddr(&ind->tpaddr);
    if (connection)
    {
        connection->pending_disconnects = GattConnect_ObserverGetNumberDisconnectReqCallbacksRegistered();
        DEBUG_LOG("gattConnect_HandleConManagerDisconnectRequested, cid 0x%04x pending_disconnects %d", connection->cid, connection->pending_disconnects);
        if (connection->pending_disconnects)
        {
            GattConnect_ObserverNotifyOnDisconnectRequested(connection->cid, gattConnect_DisconnectRequestedResponse);
        }
        else
        {
            GattManagerDisconnectRequest(connection->cid);
        }
    }
}

static void gattConnect_HandleGattManagerConnectAsCentral(const GATT_MANAGER_CONNECT_AS_CENTRAL_CFM_T *cfm)
{
    gatt_connection_t* connection = GattConnect_CreateConnection(cfm->cid);

    DEBUG_LOG("gattConnect_HandleGattManagerConnectAsCentral, cid 0x%04x flags 0x%04d", cfm->cid, cfm->flags);

    if(connection)
    {
        GattConnect_SetMtu(connection, cfm->mtu);
        GattConnect_SendExchangeMtuReq(&gatt_task_data, cfm->cid);
        GattConnect_ObserverNotifyOnConnection(cfm->cid);
    }
}


static void gattConnect_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);
    switch(id)
    {
        case GATT_MANAGER_CONNECT_IND:
            gattConnect_HandleConnectInd((GATT_MANAGER_CONNECT_IND_T*)msg);
        break;
        
        case GATT_MANAGER_DISCONNECT_IND:
            gattConnect_HandleDisconnectInd((GATT_MANAGER_DISCONNECT_IND_T*)msg);
        break;
        
        case GATT_EXCHANGE_MTU_IND:
            gattConnect_HandleExchangeMtuInd((GATT_EXCHANGE_MTU_IND_T*)msg);
        break;
        
        case GATT_EXCHANGE_MTU_CFM:
            gattConnect_HandleExchangeMtuCfm((GATT_EXCHANGE_MTU_CFM_T*)msg);
        break;
        
        case GATT_MANAGER_REGISTER_WITH_GATT_CFM:
            gattConnect_HandleRegisterWithGattCfm((GATT_MANAGER_REGISTER_WITH_GATT_CFM_T*)msg);
        break;
        
        case CON_MANAGER_TP_CONNECT_IND:
            gattConnect_HandleConManagerConnection((const CON_MANAGER_TP_CONNECT_IND_T *)msg);
        break;
            
        case CON_MANAGER_TP_DISCONNECT_IND:
            gattConnect_HandleConManagerDisconnection((const CON_MANAGER_TP_DISCONNECT_IND_T *)msg);
        break;
        
        case CON_MANAGER_TP_DISCONNECT_REQUESTED_IND:
            gattConnect_HandleConManagerDisconnectRequested((const CON_MANAGER_TP_DISCONNECT_REQUESTED_IND_T *)msg);
        break;
        
        case GATT_MANAGER_CONNECT_AS_CENTRAL_CFM:
            gattConnect_HandleGattManagerConnectAsCentral((const GATT_MANAGER_CONNECT_AS_CENTRAL_CFM_T *)msg);
        break;
        
        default:
        break;
    }
}

bool GattConnect_Init(Task init_task)
{
    UNUSED(init_task);
    GattConnect_MtuInit();
    GattConnect_ListInit();
    GattConnect_ObserverInit();
    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, &gatt_task_data);
    PanicFalse(GattManagerInit(&gatt_task_data));
    return TRUE;
}

bool GattConnect_ServerInitComplete(Task init_task)
{
    gatt_connect_init_task = init_task;
    GattManagerRegisterWithGatt();
    return TRUE;
}
