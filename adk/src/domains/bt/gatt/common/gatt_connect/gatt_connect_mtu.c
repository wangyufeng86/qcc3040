/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Handling GATT MTU exchange
*/

#include "gatt_connect.h"
#include "gatt_connect_list.h"
#include "gatt_connect_mtu.h"

#include <panic.h>

#define DEFAULT_MTU 65

static unsigned mtu_local;

static void updateConnectionMtu(unsigned cid, unsigned mtu_remote)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    GattConnect_SetMtu(connection, MIN(mtu_local, mtu_remote));
}

void gattConnect_HandleExchangeMtuInd(GATT_EXCHANGE_MTU_IND_T* ind)
{
    updateConnectionMtu(ind->cid, ind->mtu);
    GattExchangeMtuResponse(ind->cid, mtu_local);
}

void GattConnect_SendExchangeMtuReq(Task task, unsigned cid)
{
    GattExchangeMtuRequest(task, cid, mtu_local);
}

void gattConnect_HandleExchangeMtuCfm(GATT_EXCHANGE_MTU_CFM_T* cfm)
{
    switch (cfm->status)
    {
        case gatt_status_success:
            updateConnectionMtu(cfm->cid, cfm->mtu);
            break;
        case gatt_status_invalid_mtu:
        case gatt_status_mtu_already_exchanged:
            Panic();
            break;
        default:
            break;
    }
}

void GattConnect_UpdateMinAcceptableMtu(unsigned mtu)
{
    mtu_local = MAX(mtu_local, mtu);
}

void GattConnect_SetMtu(gatt_connection_t* connection, unsigned mtu)
{
    if(connection)
    {
        connection->mtu = mtu;
    }
}

unsigned GattConnect_GetMtu(unsigned cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    
    if(connection)
        return connection->mtu;
    
    return GATT_CONNECT_MTU_INVALID;
}

void GattConnect_MtuInit(void)
{
    mtu_local = DEFAULT_MTU;
}
