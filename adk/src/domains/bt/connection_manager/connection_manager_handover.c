/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       connection_manager_handover.c
\brief      Connection Manager Handover related interfaces

*/
#ifdef INCLUDE_MIRRORING
#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "connection_manager_list.h"
#include "bt_device.h"
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool connectionManager_Veto(void);

static bool connectionManager_Marshal(const bdaddr *bd_addr, 
                                      marshal_type_t type,
                                      void **marshal_obj);

static app_unmarshal_status_t connectionManager_Unmarshal(const bdaddr *bd_addr, 
                                        marshal_type_t type,
                                        void *unmarshal_obj);

static void connectionManager_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
const marshal_type_t connection_manager_marshal_types[] = {
    MARSHAL_TYPE(cm_connection_t),
};

const marshal_type_list_t connection_manager_marshal_types_list = {connection_manager_marshal_types, sizeof(connection_manager_marshal_types)/sizeof(marshal_type_t)};
REGISTER_HANDOVER_INTERFACE(CONNECTION_MANAGER, &connection_manager_marshal_types_list, connectionManager_Veto, connectionManager_Marshal, connectionManager_Unmarshal, connectionManager_Commit);


/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*! 
    \brief Handle Veto check during handover
    \return bool
*/
static bool connectionManager_Veto(void)
{
    cm_list_iterator_t iterator;
    cm_connection_state_t connection_state;
    bool veto = FALSE;

    /* Check for transient state where lock is held for all connections */
    cm_connection_t* connection = ConManagerListHeadConnection(&iterator);
    
    while (connection)
    {
        connection_state = conManagerGetConnectionState(connection);

        if (connection_state == ACL_CONNECTING)
        {
            DEBUG_LOG("connectionManager_Veto, ACL_CONNECTING");
            veto = TRUE;
            break;
        }
        else if (connection_state == ACL_CONNECTING_PENDING_PAUSE)
        {
            DEBUG_LOG("connectionManager_Veto, ACL_CONNECTING_PENDING_PAUSE");
            veto = TRUE;
            break;
        }

        connection = ConManagerListNextConnection(&iterator);
    }
    
    return veto;
}

/*!
    \brief The function shall set marshal_obj to the address of the object to 
           be marshalled.

    \param[in] bd_addr      Bluetooth address of the link to be marshalled
                            \ref bdaddr
    \param[in] type         Type of the data to be marshalled \ref marshal_type_t
    \param[out] marshal_obj Holds address of data to be marshalled.
    \return TRUE: Required data has been copied to the marshal_obj.
            FALSE: No data is required to be marshalled. marshal_obj is set to NULL.

*/
static bool connectionManager_Marshal(const bdaddr *bd_addr, 
                                      marshal_type_t type, 
                                      void **marshal_obj)
{
    bool status = FALSE;
    *marshal_obj = NULL;

    switch (type)
    {
        case MARSHAL_TYPE(cm_connection_t):
        {
            tp_bdaddr tpaddr;

            BdaddrTpFromBredrBdaddr(&tpaddr, bd_addr);

            *marshal_obj = PanicNull(ConManagerFindConnectionFromBdAddr(&tpaddr));

            status = TRUE;
            break;
        }
        default:
            Panic();
            break;
    }

    return status;
}

/*! 
    \brief The function shall copy the unmarshal_obj associated to specific 
            marshal type \ref marshal_type_t

    \param[in] bd_addr      Bluetooth address of the link to be unmarshalled
                            \ref bdaddr
    \param[in] type         Type of the unmarshalled data \ref marshal_type_t
    \param[in] unmarshal_obj Address of the unmarshalled object.
    \return unmarshalling result. Based on this, caller decides whether to free
            the marshalling object or not.

*/
static app_unmarshal_status_t connectionManager_Unmarshal(const bdaddr *bd_addr, 
                                        marshal_type_t type, 
                                        void *unmarshal_obj)
{
    tp_bdaddr tpaddr;
    app_unmarshal_status_t result = UNMARSHAL_FAILURE;
    
    BdaddrTpFromBredrBdaddr(&tpaddr, bd_addr);

    switch (type)
    {
        case MARSHAL_TYPE(cm_connection_t):
        {
            cm_connection_t* new_connection;
            tp_bdaddr empty;

            BdaddrTpSetEmpty(&empty);
            new_connection = ConManagerFindConnectionFromBdAddr(&empty);
            ConManagerConnectionCopy(new_connection, (cm_connection_t*)unmarshal_obj);
            result = UNMARSHAL_SUCCESS_FREE_OBJECT;
            break;
        }

        default:
            /* Do nothing */
            break;
    }

    return result;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void connectionManager_Commit(bool is_primary)
{
    cm_list_iterator_t iterator;
    cm_connection_t* connection = ConManagerListHeadConnection(&iterator);

    while (connection)
    {
        const tp_bdaddr *addr = ConManagerGetConnectionTpAddr(connection);

        if (is_primary)
        {
            if (appDeviceIsPrimary(&addr->taddr.addr))
            {
                /* The new primary needs to swap the address of the peer */
                const tp_bdaddr secondary_addr = *addr;
                PanicFalse(appDeviceGetSecondaryBdAddr(&secondary_addr.taddr.addr));
                ConManagerSetConnectionTpAddr(connection, &secondary_addr);
            }
        }
        else
        {
            if (appDeviceTypeIsHandset(&addr->taddr.addr))
            {
                /* The new secondary needs to remove handset connection instance(s) */
                conManagerRemoveConnection(connection);
            }
            else if (appDeviceIsSecondary(&addr->taddr.addr))
            {
                /* The new secondary needs to swap the address of the peer */
                const tp_bdaddr primary_addr = *addr;
                PanicFalse(appDeviceGetPrimaryBdAddr(&primary_addr.taddr.addr));
                ConManagerSetConnectionTpAddr(connection, &primary_addr);
            }
        }
        connection = ConManagerListNextConnection(&iterator);
    }
}

#endif /* INCLUDE_MIRRORING */
