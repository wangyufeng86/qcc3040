/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#include "connection_manager_list.h"
#include "connection_manager_qos.h"
#include "connection_manager_msg.h"
#include "connection_manager_protected.h"

#include <bdaddr.h>
#include <logging.h>
#include <panic.h>

cm_connection_t connections[CON_MANAGER_MAX_CONNECTIONS];

#define for_all_connections(connection) for(connection = &connections[0]; connection < &connections[CON_MANAGER_MAX_CONNECTIONS]; connection++)

/******************************************************************************/
static void conManagerResolveTpaddr(const tp_bdaddr * tpaddr, tp_bdaddr * resolved_tpaddr)
{
    /* Attempt to resolve RPA to a public address where applicable */
    if(tpaddr->transport == TRANSPORT_BLE_ACL)
    {
        if(tpaddr->taddr.type == TYPED_BDADDR_RANDOM)
        {
            if(VmGetPublicAddress(tpaddr, resolved_tpaddr))
            {
                return;
            }
        }
    }
    
    /* Use the original tpaddr (whatever transport/type) */
    memcpy(resolved_tpaddr, tpaddr, sizeof(tp_bdaddr));
}

/******************************************************************************/
Task ConManagerGetTask(cm_connection_t* connection)
{
    if(connection)
    {
        if(connection->task_data.handler)
        {
            return &connection->task_data;
        }
    }
    return NULL;
}

/******************************************************************************/

static void conManagerDebugAddressWithLevel(debug_log_level_t debug_level, const tp_bdaddr *tpaddr)
{
    tp_bdaddr resolved_tpaddr;
    UNUSED(tpaddr);
    UNUSED(debug_level);

    if (tpaddr->taddr.type == TYPED_BDADDR_RANDOM)
    {
        if (VmGetPublicAddress(tpaddr, &resolved_tpaddr))
        {
            DEBUG_LOG_WITH_LEVEL(debug_level,
                        "ConManagerDebugAddress, transport %02x, type %02x, random addr %04x,%02x,%06lx, public addr %04x,%02x,%06lx",
                        tpaddr->transport,
                        tpaddr->taddr.type,
                        tpaddr->taddr.addr.nap,
                        tpaddr->taddr.addr.uap,
                        tpaddr->taddr.addr.lap,
                        resolved_tpaddr.taddr.addr.nap,
                        resolved_tpaddr.taddr.addr.uap,
                        resolved_tpaddr.taddr.addr.lap);
        }
        else
        {
            DEBUG_LOG_WITH_LEVEL(debug_level,
                        "ConManagerDebugAddress, transport %02x, type %02x, random addr %04x,%02x,%06lx, public addr unresolved",
                        tpaddr->transport,
                        tpaddr->taddr.type,
                        tpaddr->taddr.addr.nap,
                        tpaddr->taddr.addr.uap,
                        tpaddr->taddr.addr.lap);
        }
    }
    else
    {
        DEBUG_LOG_WITH_LEVEL(debug_level,
                    "ConManagerDebugAddress, transport %02x, type %02x, public addr %04x,%02x,%06lx",
                    tpaddr->transport,
                    tpaddr->taddr.type,
                    tpaddr->taddr.addr.nap,
                    tpaddr->taddr.addr.uap,
                    tpaddr->taddr.addr.lap);
    }
}

void ConManagerDebugAddress(const tp_bdaddr *tpaddr)
{
    conManagerDebugAddressWithLevel(DEBUG_LOG_LEVEL_DEBUG, tpaddr);
}

void ConManagerDebugAddressVerbose(const tp_bdaddr *tpaddr)
{
    conManagerDebugAddressWithLevel(DEBUG_LOG_LEVEL_VERBOSE, tpaddr);
}

/******************************************************************************/

static void conManagerDebugConnectionWithLevel(debug_log_level_t debug_level, const cm_connection_t *connection)
{
    UNUSED(debug_level);

    _DEBUG_LOG_L(debug_level,
            "ConManagerDebugConnection, state %u, lock %u, users %u",
                 connection->bitfields.state, connection->lock, connection->bitfields.users);
    conManagerDebugAddressWithLevel(debug_level, &connection->tpaddr);
}

void ConManagerDebugConnection(const cm_connection_t *connection)
{
    conManagerDebugConnectionWithLevel(DEBUG_LOG_LEVEL_DEBUG, connection);
}

void ConManagerDebugConnectionVerbose(const cm_connection_t *connection)
{
    conManagerDebugConnectionWithLevel(DEBUG_LOG_LEVEL_VERBOSE, connection);
}

/******************************************************************************/
void ConManagerSetConnectionState(cm_connection_t *connection, cm_connection_state_t state)
{
    connection->bitfields.state = state;
    connection->lock = (state & ACL_STATE_LOCK);

    DEBUG_LOG_VERBOSE("ConManagerSetConnectionState state %u, lock %u, users %u",
            connection->bitfields.state, connection->lock, connection->bitfields.users);
    ConManagerDebugAddressVerbose(&connection->tpaddr);
}

/******************************************************************************/
cm_connection_state_t conManagerGetConnectionState(const cm_connection_t *connection)
{
    if(connection)
        return connection->bitfields.state;
    
    return ACL_DISCONNECTED;
}

/******************************************************************************/
cm_connection_t *ConManagerFindConnectionFromBdAddr(const tp_bdaddr *tpaddr)
{
    cm_connection_t *connection;
    
    for_all_connections(connection)
    {
        tp_bdaddr resolved_tpaddr;
        tp_bdaddr resolved_conn_tpaddr;
        
        conManagerResolveTpaddr(tpaddr, &resolved_tpaddr);
        conManagerResolveTpaddr(&connection->tpaddr, &resolved_conn_tpaddr);
        
        if (BdaddrTpIsSame(&resolved_conn_tpaddr, &resolved_tpaddr))
            return connection;
    }
    return NULL;
}

/******************************************************************************/
static cm_connection_t *conManagerFindConnectionFromState(cm_transport_t transport_mask, cm_connection_state_t state)
{
    cm_connection_t *connection;
    
    for_all_connections(connection)
    {
        cm_transport_t connection_transport = (cm_transport_t)(1 << connection->tpaddr.transport);
        
        if(connection_transport & transport_mask)
        {
            if (connection->bitfields.state == state)
                return connection;
        }
    }
    return NULL;
}

/******************************************************************************/
cm_connection_t *ConManagerFindFirstActiveLink(cm_transport_t transport_mask)
{
    cm_connection_t *connection;

    for_all_connections(connection)
    {
        cm_transport_t connection_transport = TransportToCmTransport(connection->tpaddr.transport);

        if (   connection_transport & transport_mask
            && connection->bitfields.state != ACL_DISCONNECTED
            && connection->bitfields.state != ACL_DISCONNECTED_LINK_LOSS)
        {
            DEBUG_LOG("ConManagerFindFirstActiveLink. Found 0x%x. State: %x Lock:%x Users:%x Trans:0x%x",
                    connection->tpaddr.taddr.addr.lap, 
                    connection->bitfields.state, connection->lock,
                    connection->bitfields.users,
                    connection->tpaddr.transport);
            return connection;
        }
    }

    DEBUG_LOG("ConManagerFindFirstActiveLink. None found for transport mask 0x%x", transport_mask);
    return NULL;
}


/******************************************************************************/
static cm_connection_t *conManagerFindUnusedConnection(void)
{
    tp_bdaddr tpaddr;
    BdaddrTpSetEmpty(&tpaddr);
    
    DEBUG_LOG("ConManagerGetUnusedConnection");
    return ConManagerFindConnectionFromBdAddr(&tpaddr);
}

/******************************************************************************/
bool conManagerAnyLinkInState(cm_transport_t transport_mask, cm_connection_state_t state)
{
    cm_connection_t *connection = conManagerFindConnectionFromState(transport_mask, state);
    
    if(connection)
    {
        DEBUG_LOG("ConManagerAnyLinkInState %d", state);
        ConManagerDebugConnectionVerbose(connection);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************/
void ConManagerSetConnectionLocal(cm_connection_t *connection, bool local)
{
    if (connection)
        connection->bitfields.local = local;
}

/******************************************************************************/
void conManagerAddConnectionUser(cm_connection_t *connection)
{
    if(connection)
        connection->bitfields.users += 1;
}

/******************************************************************************/
void conManagerRemoveConnectionUser(cm_connection_t *connection)
{
    if(connection && connection->bitfields.users)
        connection->bitfields.users -= 1;
}

/******************************************************************************/
void conManagerResetConnectionUsers(cm_connection_t *connection)
{
    if(connection)
        connection->bitfields.users = 0;
}

/******************************************************************************/
bool conManagerConnectionIsInUse(cm_connection_t *connection)
{
    if(connection)
        return (connection->bitfields.users > 0);
    
    return FALSE;
}

/******************************************************************************/
uint16* conManagerGetConnectionLock(cm_connection_t *connection)
{
    if(connection)
        return &connection->lock;
    
    return NULL;
}

/******************************************************************************/
void conManagerGetLpState(cm_connection_t *connection, lpPerConnectionState* lp_state)
{
    if (connection)
        *lp_state = connection->lp_state;
}

/******************************************************************************/
void conManagerSetLpState(cm_connection_t *connection, lpPerConnectionState lp_state)
{
    if (connection)
        connection->lp_state = lp_state;
}

/******************************************************************************/
uint8* conManagerGetQosList(cm_connection_t* connection)
{
    if(connection)
    {
        return connection->qos_list;
    }
    
    return NULL;
}

/******************************************************************************/
bool conManagerConnectionIsLocallyInitiated(const cm_connection_t *connection)
{
    return connection ? connection->bitfields.local : FALSE;
}

/******************************************************************************/
static void conManagerResetConnection(cm_connection_t* connection)
{
    if (connection)
    {
        MessageFlushTask(&connection->task_data);
        /* The default values for mode and interval are acceptable */
        memset(connection, 0, sizeof(cm_connection_t));
        ConManagerSetConnectionState(connection, ACL_DISCONNECTED);
        BdaddrTpSetEmpty(&connection->tpaddr);
        conManagerResetConnectionUsers(connection);
        connection->lp_state.pt_index = POWERTABLE_UNASSIGNED;
    }
}

/******************************************************************************/
static cm_connection_t* conManagerReuseLinkLossConnection(void)
{
    cm_connection_t *connection = conManagerFindConnectionFromState(cm_transport_all, ACL_DISCONNECTED_LINK_LOSS);
    
    conManagerResetConnection(connection);
    
    return connection;
}

/******************************************************************************/
cm_connection_t* ConManagerAddConnection(const tp_bdaddr *tpaddr, cm_connection_state_t state, bool is_local)
{
    cm_connection_t *connection = ConManagerFindConnectionFromBdAddr(tpaddr);
    
    if (connection)
    {
        DEBUG_LOG("ConManagerAddConnection - existing found");
        ConManagerSetConnectionState(connection, state);
        return connection;
    }
    else
    {
        connection = conManagerFindUnusedConnection();
        if (connection == NULL)
        {
            DEBUG_LOG("ConManagerAddConnection - re-use link loss");
            connection = conManagerReuseLinkLossConnection();
        }

        if (connection)
        {
            DEBUG_LOG("ConManagerAddConnection - new");
            conManagerResetConnection(connection);
            
            connection->tpaddr = *tpaddr;
            ConManagerSetConnectionState(connection, state);
            ConManagerSetConnectionLocal(connection, is_local);
            connection->task_data.handler = ConManagerConnectionHandleMessage;
            return connection;
        }

        DEBUG_LOG("ConManagerAddConnection - No space!");
        Panic();
    }

    return NULL;
}

/******************************************************************************/
void conManagerRemoveConnection(cm_connection_t *connection)
{
    if (connection)
    {
        conManagerResetConnection(connection);
    }
}

/******************************************************************************/
void ConManagerConnectionInit(void)
{
    cm_connection_t* connection;
    
    for_all_connections(connection)
    {
        conManagerResetConnection(connection);
    }
}

/******************************************************************************/
cm_connection_t* ConManagerListHeadConnection(cm_list_iterator_t* iterator)
{
    if(iterator)
    {
        iterator->connection = &connections[0];
    }
    
    return &connections[0];
}

/******************************************************************************/
cm_connection_t* ConManagerListNextConnection(cm_list_iterator_t* iterator)
{
    cm_connection_t* connection = NULL;
    
    PanicNull(iterator);
    
    iterator->connection++;
    
    if(iterator->connection < &connections[CON_MANAGER_MAX_CONNECTIONS])
    {
        connection = iterator->connection;
    }
    
    return connection;
}

/******************************************************************************/
const tp_bdaddr* ConManagerGetConnectionTpAddr(cm_connection_t* connection)
{
    if(connection)
    {
        return &connection->tpaddr;
    }
    return NULL;
}

/*****************************************************************************************/
void ConManagerSetConnectionTpAddr(cm_connection_t* connection, const tp_bdaddr* new_addr)
{
    if (connection && new_addr)
    {
        connection->tpaddr = *new_addr;
    }
}

/********************************************************************************/
void ConManagerConnectionCopy(cm_connection_t* dest, const cm_connection_t* src)
{
    dest->task_data.handler = ConManagerConnectionHandleMessage;
    dest->tpaddr = src->tpaddr;
    dest->lock = src->lock;
    dest->bitfields = src->bitfields;
    dest->lp_state = src->lp_state;
    dest->mode = src->mode;
    dest->sniff_interval = src->sniff_interval;
    dest->conn_interval = src->conn_interval;
    dest->slave_latency = src->slave_latency;
    memmove(dest->qos_list, src->qos_list, sizeof(src->qos_list[0]) * cm_qos_max);
}

/********************************************************************************/
cm_connection_t * ConManagerFindConnectionFromBredrBdaddr(const bdaddr *addr)
{
    cm_connection_t *connection = NULL;
    tp_bdaddr tpaddr;

    if(addr != NULL)
    {
        BdaddrTpFromBredrBdaddr(&tpaddr, addr);
        connection = ConManagerFindConnectionFromBdAddr(&tpaddr);
    }
    return connection;
}

