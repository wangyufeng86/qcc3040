/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#include "connection_manager_qos.h"
#include "connection_manager_params.h"
#include "connection_manager_list.h"
#include "connection_manager_msg.h"

#include <logging.h>
#include <panic.h>
#include <local_addr.h>

static cm_qos_t cm_default_qos;
static cm_qos_t cm_max_qos;

/******************************************************************************/
static bool conManagerGetParamsToUse(cm_qos_t qos, ble_connection_params* params)
{
    if(cm_qos_params[qos])
    {
        *params = *cm_qos_params[qos];
        params->own_address_type = LocalAddr_GetBleType();
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
static void conManagerSendParameterUpdate(const tp_bdaddr* tpaddr, cm_qos_t qos)
{
    if(tpaddr && tpaddr->transport == TRANSPORT_BLE_ACL)
    {
        DEBUG_LOG("conManagerSendParameterUpdate qos:%d", qos);
        ConManagerDebugAddressVerbose(tpaddr);

        if(qos != cm_qos_passive)
        {
            ble_connection_params params;
            
            if(conManagerGetParamsToUse(qos, &params))
            {
                /* NULL AppTask as we can't do much if this fails anyway */
                ConnectionDmBleConnectionParametersUpdateReq(
                    NULL, 
                    (typed_bdaddr*)&tpaddr->taddr, 
                    params.conn_interval_min, 
                    params.conn_interval_max, 
                    params.conn_latency, 
                    params.supervision_timeout, 
                    LE_CON_EVENT_LENGTH_MIN, 
                    LE_CON_EVENT_LENGTH_MAX);
            }
        }
    }
}

/******************************************************************************/
static void conManagerUpdateConnectionParameters(cm_connection_t* connection, cm_qos_t qos)
{
    if(conManagerGetConnectionState(connection) == ACL_CONNECTED)
    {
        const tp_bdaddr* tpaddr = ConManagerGetConnectionTpAddr(connection);
        conManagerSendParameterUpdate(tpaddr, qos);
    }
}

/******************************************************************************/
static cm_qos_t conManagerGetConnectionQos(cm_connection_t* connection)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        cm_qos_t qos;
        
        for(qos = cm_qos_max - 1; qos > cm_qos_invalid; qos--)
        {
            if(qos_list[qos] > 0)
            {
                return qos;
            }
        }
    }
    
    return cm_qos_invalid;
}

/******************************************************************************/
static void conManagerRecordQos(cm_connection_t* connection, cm_qos_t qos)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        qos_list[qos]++;
    }
}

/******************************************************************************/
static void conManagerReleaseQos(cm_connection_t* connection, cm_qos_t qos)
{
    uint8* qos_list = conManagerGetQosList(connection);
    
    if(qos_list)
    {
        PanicFalse(qos_list[qos] > 0);
        
        qos_list[qos]--;
    }
}

/******************************************************************************/
static cm_qos_t conManagerGetQosToUse(cm_connection_t* connection)
{
    cm_qos_t qos = conManagerGetConnectionQos(connection);
    
    if(qos == cm_qos_invalid)
    {
        qos = cm_default_qos;
    }
    
    return MIN(qos, cm_max_qos);
}

/******************************************************************************/
static bool conManagerConnectionQosIsDefault(cm_connection_t* connection)
{
    cm_qos_t qos = conManagerGetConnectionQos(connection);
    
    if(qos == cm_qos_invalid || qos == cm_default_qos)
    {
        return TRUE;
    }
    
    return FALSE;
}

/******************************************************************************/
static void conManagerValidateQos(cm_transport_t transport_mask, cm_qos_t qos)
{
    PanicFalse(transport_mask == cm_transport_ble);
    PanicFalse(qos > cm_qos_invalid);
    PanicFalse(qos < cm_qos_max);
}

/******************************************************************************/
void ConManagerApplyQosOnConnect(const tp_bdaddr *tpaddr)
{
    cm_qos_t qos_to_use;
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    if(!connection)
    {
        /* Update received after disconnect */
        return;
    }
    
    /* Locally initiated connection will already be using default parameters*/
    if(conManagerConnectionIsLocallyInitiated(connection))
    {
        if(conManagerConnectionQosIsDefault(connection))
        {
            return;
        }
    }
    
    qos_to_use = conManagerGetQosToUse(connection);
    conManagerUpdateConnectionParameters(connection, qos_to_use);
}

/******************************************************************************/
void ConnectionManagerQosInit(void)
{
    cm_default_qos = cm_qos_invalid;
    cm_max_qos = cm_qos_max;
    ConManagerRequestDefaultQos(cm_transport_ble, cm_qos_low_power);
}

/******************************************************************************/
void ConManagerRequestDefaultQos(cm_transport_t transport_mask, cm_qos_t qos)
{
    conManagerValidateQos(transport_mask, qos);
    
    if(qos > cm_default_qos)
    {
        cm_qos_t qos_to_use;
        ble_connection_params params;
        
        cm_default_qos = qos;
        
        qos_to_use = conManagerGetQosToUse(NULL);
        if(conManagerGetParamsToUse(qos_to_use, &params))
        {
            ConnectionDmBleSetConnectionParametersReq(&params);
        }
    }
}

/******************************************************************************/
void ConManagerApplyQosPreConnect(const tp_bdaddr *tpaddr)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);

    if (connection)
    {
        ble_connection_params params;
        cm_qos_t qos = conManagerGetQosToUse(connection);

        DEBUG_LOG("ConManagerApplyQosPreConnect (%d). Connection:%p", qos, connection);
        ConManagerDebugAddressVerbose(tpaddr);

        if(conManagerGetParamsToUse(qos, &params))
        {
            ConnectionDmBleSetConnectionParametersReq(&params);
        }
    }
}

/******************************************************************************/
void ConManagerRequestDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    conManagerValidateQos(TransportToCmTransport(tpaddr->transport), qos);
    
    if(connection)
    {
        cm_qos_t prev_qos = conManagerGetConnectionQos(connection);
        conManagerRecordQos(connection, qos);
        /* This can result in repeated updates if qos == prev_qos.
           Assumption here is that sending too many is harmless, but
           too few and we might not correct a failed update */
        if(qos >= prev_qos)
        {
            conManagerSendInternalMsgUpdateQos(connection);
        }
    }
}

/******************************************************************************/
void ConManagerReleaseDeviceQos(const tp_bdaddr *tpaddr, cm_qos_t qos)
{
    cm_connection_t* connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    conManagerValidateQos(TransportToCmTransport(tpaddr->transport), qos);
    
    if(connection)
    {
        cm_qos_t fallback_qos;
        cm_qos_t prev_qos = conManagerGetConnectionQos(connection);
        
        conManagerReleaseQos(connection, qos);
        fallback_qos = conManagerGetQosToUse(connection);
        
        if(fallback_qos != prev_qos)
        {
            conManagerSendInternalMsgUpdateQos(connection);
        }
    }
}

/******************************************************************************/
void ConManagerSetMaxQos(cm_qos_t qos)
{
    cm_list_iterator_t iterator;
    cm_connection_t* connection = ConManagerListHeadConnection(&iterator);
    DEBUG_LOG("ConManagerSetMaxQos");
    
    PanicFalse(qos > cm_qos_invalid);
    PanicFalse(qos != cm_qos_passive);
    
    cm_max_qos = qos;
    
    while(connection)
    {
        conManagerSendInternalMsgUpdateQos(connection);
        connection = ConManagerListNextConnection(&iterator);
    }
}
