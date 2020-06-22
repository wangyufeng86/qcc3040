/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Tracking GATT connections
*/

#include "gatt_connect.h"
#include "gatt_connect_list.h"

#include <bdaddr.h>

static gatt_connection_t connections[GATT_CONNECT_MAX_CONNECTIONS];

gatt_connection_t* GattConnect_FindConnectionFromCid(unsigned cid)
{
    gatt_connection_t* result;
    
    for(result = &connections[0]; result <= &connections[GATT_CONNECT_MAX_CONNECTIONS - 1]; result++)
    {
        if(result->cid == cid)
            return result;
    }
    
    return NULL;
}

gatt_connection_t* GattConnect_CreateConnection(unsigned cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(0);
    
    if(connection)
        connection->cid = cid;
    
    return connection;
}

void GattConnect_DestroyConnection(unsigned cid)
{
    gatt_connection_t* connection = GattConnect_FindConnectionFromCid(cid);
    
    if(connection)
    {
        memset(connection, 0, sizeof(gatt_connection_t));
    }
}

void GattConnect_ListInit(void)
{
    memset(connections, 0, sizeof(connections));
}

gatt_connection_t* GattConnect_FindConnectionFromTpaddr(const tp_bdaddr *tpaddr_in)
{
    gatt_connection_t* result;
    tp_bdaddr tpaddr_for_cid;
    
    for(result = &connections[0]; result <= &connections[GATT_CONNECT_MAX_CONNECTIONS - 1]; result++)
    {
        if (VmGetBdAddrtFromCid(result->cid, &tpaddr_for_cid))
        {
            if (BdaddrTpIsSame(tpaddr_in, &tpaddr_for_cid))
            {
                return result;
            }
        }
    }
    
    return NULL;
}
