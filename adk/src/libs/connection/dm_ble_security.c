/****************************************************************************
Copyright (c) 2011 - 2015 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_ble_security.c      

DESCRIPTION
    This file contains the implementation of Low Energy security msg scenarios.

NOTES

*/

#include "connection.h"
#include "connection_private.h"

#include "dm_ble_security.h"
#include "connection_tdl.h"

#include <bdaddr.h>
#include <vm.h>

#ifndef DISABLE_BLE
#define DM_SM_SECURITY_REFRESH_ENCRYPTION 0x1000

/****************************************************************************
NAME    
    ConnectionBleDmSecurityReq

DESCRIPTION
    Starts the BLE Sm Security message scenario.

RETURNS
   void
*/
void ConnectionDmBleSecurityReq(
        Task                    theAppTask, 
        const typed_bdaddr      *taddr, 
        ble_security_type       security,
        ble_connection_type     conn_type
        )
{
#ifdef CONNECTION_DEBUG_LIB
    /* Check parameters. */
    if (security >= ble_security_last )
        CL_DEBUG(("ble_security_type out of range\n"));

    if (conn_type >= ble_connection_last)
        CL_DEBUG(("ble_connection_type out of range\n"));
    
#endif
    {
        MAKE_CL_MESSAGE(CL_INTERNAL_SM_DM_SECURITY_REQ);

        message->theAppTask = theAppTask;
        message->taddr      = *taddr;
        message->security   = security;
        message->conn_type = conn_type;

        MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_DM_SECURITY_REQ, message);
    }
}


/****************************************************************************
NAME    
    connectionHandleInternalBleDmSecurityReq

DESCRIPTION
    Deals with the internal message (see above). State lock for the task is 
    not required as it can be passed as the context for the Security message
    scenario.

RETURNS
   void
*/
void connectionHandleInternalBleDmSecurityReq(
        const CL_INTERNAL_SM_DM_SECURITY_REQ_T *req
        )
{
    MAKE_PRIM_T(DM_SM_SECURITY_REQ);
 
    switch(req->conn_type)
    {
        case ble_connection_master_directed:
            prim->connection_flags = 
                L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_DIRECTED);
            break;
        case ble_connection_master_whitelist:
            prim->connection_flags = 
                L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_MASTER_WHITELIST);
            break;
        case ble_connection_slave_directed:
            prim->connection_flags = 
                L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_SLAVE_DIRECTED);
            break;
        case ble_connection_slave_undirected:
            prim->connection_flags =
                L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_SLAVE_UNDIRECTED);
            break;
        case ble_connection_slave_whitelist:
            prim->connection_flags = 
                L2CA_CONFLAG_ENUM(L2CA_CONNECTION_LE_SLAVE_WHITELIST);
            break;
        default:
            CL_DEBUG(("ble_connection_type %d not handled\n", req->conn_type));
            break;
    }

    /* Add flags to lock the ACL and to Time out the security scenario if it 
     * takes too long. 
     */
    prim->connection_flags |= L2CA_CONFLAG_LOCK_ACL;
    prim->connection_flags |= L2CA_CONFLAG_PAGE_TIMEOUT;

    BdaddrConvertTypedVmToBluestack(&prim->addrt, &req->taddr);

    prim->context = CTX_FROM_TASK(req->theAppTask);

    /* Secure Connections is set as an option in DM_SM_INIT and doesn't need to be
     * accounted for when performing the Security Req. Additionally, When exchanging
     * IO Capabilities, Secure Connections will be enabled by default for the BLE
     * transport.
     */
    switch (req->security)
    {
        case ble_security_encrypted:
            prim->security_requirements =
                    DM_SM_SECURITY_UNAUTHENTICATED_NO_BONDING;
            break;
        case ble_security_encrypted_bonded:
            prim->security_requirements =
                DM_SM_SECURITY_UNAUTHENTICATED_BONDING;
            break;
        case ble_security_authenticated:
            prim->security_requirements =
                DM_SM_SECURITY_AUTHENTICATED_NO_BONDING;
            break;
        case ble_security_authenticated_bonded:
            prim->security_requirements =
                DM_SM_SECURITY_AUTHENTICATED_BONDING;
            break;
        case ble_security_refresh_encryption:
            prim->security_requirements =
                DM_SM_SECURITY_REFRESH_ENCRYPTION;
            break;
        default:
            CL_DEBUG(("ble_security_type %d not handled\n", req->security));
            break;
    }

    /* Not used for the REQ */
    prim->status = 0;
    
    VmSendDmPrim(prim);
}


/****************************************************************************
NAME    
    connectionHandleDmSmSecurityCfm

DESCRIPTION
    Handle the DM_SM_SECURITY_CFM message from Bluestack.

RETURNS
   void
*/
void connectionHandleDmSmSecurityCfm(const DM_SM_SECURITY_CFM_T *cfm)
{
    MAKE_CL_MESSAGE(CL_DM_BLE_SECURITY_CFM);

    switch(cfm->status)
    {
        case HCI_SUCCESS:
            message->status = ble_security_success;
            break;

        case HCI_ERROR_HOST_BUSY_PAIRING:
            message->status = ble_security_pairing_in_progress;
            break;

        case HCI_ERROR_KEY_MISSING:
            message->status = ble_security_link_key_missing;
            break;

        default:
            message->status = ble_security_fail;
            CL_DEBUG(("ble_security_status 0x%x \n", (uint16)cfm->status));
            break;
    }
    
    BdaddrConvertTypedBluestackToVm(&message->taddr, &cfm->addrt);

    MessageSend((Task)cfm->context, CL_DM_BLE_SECURITY_CFM, message);
}

/*****************************************************************************
NAME
    connectionHandleDmSmGenerateCrossTransKeyRequestInd

DESCRIPTION
    Handle the DM_SM_GENREATE_CROSS_TRANS_KEY_REQUEST_IND message from
    Bluesack.

RETUNS
    void
*/
void connectionHandleDmSmGenerateCrossTransKeyRequestInd(
        const DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND_T *ind
        )
{
    MAKE_CL_MESSAGE(CL_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND);

    message->identifier = ind->identifier;
    message->flags = ind->flags;
    BdaddrConvertTpBluestackToVm(
                &message->tpaddr,
                &ind->tp_addrt
                );

    MessageSend(
                connectionGetAppTask(),
                CL_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND,
                message);
}

/*****************************************************************************
NAME
    ConnectionSmGenerateCrossTransKeyRequestResponse

DESCRIPTION
    Send the DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP in response
    to DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_IND to Bluestack.

RETUNS
    void
*/
void ConnectionSmGenerateCrossTransKeyRequestResponse(
        const tp_bdaddr *tpaddr,
        uint8           identifier,
        ctk_flags_type  flags
        )
{
    if (tpaddr == NULL)
    {
        CL_DEBUG(("tpaddr pointer is NULL\n"));
        return;
    }
    else if (flags >= cross_trans_key_last)
    {
        CL_DEBUG(("ctk_flags_type %d out of enum range", (int)flags));
        return;
    }
    else
    {
        MAKE_PRIM_T(DM_SM_GENERATE_CROSS_TRANS_KEY_REQUEST_RSP);

        BdaddrConvertTpVmToBluestack(
                    &prim->tp_addrt,
                    tpaddr
                    );
        prim->identifier = identifier;

        switch (flags)
        {
        case cross_trans_key_disable:
            prim->flags = DM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_DISABLE;
            break;
        case cross_trans_key_enable:
            prim->flags = DM_SM_FLAG_GENERATE_CROSS_TRANS_KEY_ENABLE;
            break;
        default:
            CL_DEBUG(("This should never happen!"));
            return;
        }
        VmSendDmPrim(prim);
    }
}


#endif /* DISABLE_BLE */


