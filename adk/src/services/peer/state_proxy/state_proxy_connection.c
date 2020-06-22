/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_connection.c
\brief      Handling of connection manager events in the state proxy.
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_connection.h"
#include "state_proxy_link_quality.h"

/* framework includes */
#include <connection_manager.h>
#include <peer_signalling.h>

/* system includes */
#include <bdaddr.h>
#include <panic.h>
#include <logging.h>
#include <stdlib.h>
#include <hydra_macros.h>
#include <limits.h>

/* forward declarations */
static void stateProxy_ConnectionUpdateState(state_proxy_data_t* data, const CON_MANAGER_TP_CONNECT_IND_T* ind);
static void stateProxy_DisconnectionUpdateState(state_proxy_data_t* data, const CON_MANAGER_TP_DISCONNECT_IND_T* ind);

/*! \brief Get connection for initial state message. */
void stateProxy_GetInitialConnectionState(void)
{
    /* connection data, shouldn't be any, but let's check */
    if (ConManagerAnyLinkConnected())
    {
        DEBUG_LOG("stateProxy_GetInitialConnectionState unexpected connections");
        Panic();
    }
}

static void stateProxy_HandleConManagerConnectIndImpl(const CON_MANAGER_TP_CONNECT_IND_T *ind,
                                                      state_proxy_source source)
{
    stateProxy_ConnectionUpdateState(stateProxy_GetData(source), ind);

    if (source == state_proxy_source_local)
    {
        stateProxy_LinkQualityKick();
        stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(CON_MANAGER_TP_CONNECT_IND_T), ind, sizeof(*ind));
        /* Connect events are not indicated to local clients. */
    }
}

static void stateProxy_HandleConManagerDisconnectIndImpl(const CON_MANAGER_TP_DISCONNECT_IND_T *ind,
                                                         state_proxy_source source)
{
    stateProxy_DisconnectionUpdateState(stateProxy_GetData(source), ind);

    if (source == state_proxy_source_local)
    {
        stateProxy_LinkQualityKick();
        stateProxy_MarshalToConnectedPeer(MARSHAL_TYPE(CON_MANAGER_TP_DISCONNECT_IND_T), ind, sizeof(*ind));
        /* Disconnect events are not indicated to local clients. */
    }
    else
    {
        bool is_peer = appDeviceIsPeer(&ind->tpaddr.taddr.addr);
        stateProxy_MsgStateProxyEventClients(state_proxy_source_remote,
                                             is_peer ? state_proxy_event_type_peer_linkloss :
                                                       state_proxy_event_type_handset_linkloss,
                                             ind);
    }
}

/*! \brief Handle local connection events.
    \param[in] ind Connect indication event message.
*/
void stateProxy_HandleConManagerConnectInd(const CON_MANAGER_TP_CONNECT_IND_T* ind)
{
    DEBUG_LOG("stateProxy_HandleConManagerConnectInd");

    stateProxy_HandleConManagerConnectIndImpl(ind, state_proxy_source_local);
}

/*! \brief Handle remote connection events.
    \param[in] ind Connect indication event message.
*/
void stateProxy_HandleRemoteConManagerConnectInd(const CON_MANAGER_TP_CONNECT_IND_T* ind)
{
    DEBUG_LOG("stateProxy_HandleRemoteConManagerConnectInd");

    stateProxy_HandleConManagerConnectIndImpl(ind, state_proxy_source_remote);
}

/*! \brief Handle local connection events.
    \param[in] ind Disconnect indication event message.
*/
void stateProxy_HandleConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind)
{
    DEBUG_LOG("stateProxy_HandleConManagerDisconnectInd");

    stateProxy_HandleConManagerDisconnectIndImpl(ind, state_proxy_source_local);
}

/*! \brief Handle remote disconnection events.
    \param[in] ind Disconnect indication event message.
*/
void stateProxy_HandleRemoteConManagerDisconnectInd(const CON_MANAGER_TP_DISCONNECT_IND_T* ind)
{
    DEBUG_LOG("stateProxy_HandleRemoteConManagerDisconnectInd");

    stateProxy_HandleConManagerDisconnectIndImpl(ind, state_proxy_source_remote);
}



state_proxy_connection_t* stateProxy_GetConnection(state_proxy_data_t *data, const tp_bdaddr *bd_addr)
{
    state_proxy_connection_t *conn = NULL;
    ARRAY_FOREACH(conn, data->connection)
    {
        if (BdaddrTpIsSame(&conn->device, bd_addr))
        {
            return conn;
        }
    }
    return NULL;
}

state_proxy_connection_t* stateProxy_GetEmptyConnection(state_proxy_data_t *data)
{
    state_proxy_connection_t *conn = NULL;
    ARRAY_FOREACH(conn, data->connection)
    {
        if (BdaddrTpIsEmpty(&conn->device))
        {
            return conn;
        }
    }
    return NULL;
}

static bool stateProxy_AnyConnections(state_proxy_data_t *data)
{
    state_proxy_connection_t *conn = NULL;
    ARRAY_FOREACH(conn, data->connection)
    {
        if (!BdaddrTpIsEmpty(&conn->device))
        {
            return TRUE;
        }
    }
    return FALSE;
}

bool stateProxy_AnyLocalConnections(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    return stateProxy_AnyConnections(proxy->local_state);
}

bool stateProxy_ConnectionUpdateRssi(state_proxy_data_t *data, const tp_bdaddr *bd_addr, int8 rssi)
{
    state_proxy_connection_t *conn = stateProxy_GetConnection(data, bd_addr);
    if (conn)
    {
        conn->rssi = rssi;
    }
    return !!conn;
}

bool stateProxy_ConnectionUpdateLinkQuality(state_proxy_data_t *data, const tp_bdaddr *bd_addr, uint8 link_quality)
{
    state_proxy_connection_t *conn = stateProxy_GetConnection(data, bd_addr);
    if (conn)
    {
        conn->link_quality = link_quality;
    }
    return !!conn;
}

/*! \brief Update connection state for a data set.
    \param[in] data Pointer to data set to update.
    \param[in] ind Connection indication event message.
*/
static void stateProxy_ConnectionUpdateState(state_proxy_data_t* data, const CON_MANAGER_TP_CONNECT_IND_T* ind)
{
    state_proxy_connection_t *conn = stateProxy_GetEmptyConnection(data);
    PanicNull(conn);
    conn->device = ind->tpaddr;
    conn->flags.is_peer = appDeviceIsPeer(&ind->tpaddr.taddr.addr);
    conn->rssi = SCHAR_MIN;
}

/*! \brief Update connection state for a data set.
    \param[in] data Pointer to data set to update.
    \param[in] ind Connection indication event message.
*/
static void stateProxy_DisconnectionUpdateState(state_proxy_data_t* data, const CON_MANAGER_TP_DISCONNECT_IND_T* ind)
{
    state_proxy_connection_t *conn = stateProxy_GetConnection(data, &ind->tpaddr);
    if (conn)
    {
        memset(conn, 0, sizeof(*conn));
        BdaddrTpSetEmpty(&conn->device);
    }
}

state_proxy_connection_t* stateProxy_GetPeerConnection(state_proxy_data_t* data)
{
    state_proxy_connection_t *conn = NULL;
    ARRAY_FOREACH(conn, data->connection)
    {
        if (conn->flags.is_peer)
        {
            return conn;
        }
    }
    return NULL;
}