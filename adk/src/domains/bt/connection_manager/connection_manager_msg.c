/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#include "connection_manager_data.h"
#include "connection_manager_msg.h"
#include "connection_manager_qos.h"

#include <logging.h>
#include <panic.h>
#include <message.h>
#include <app/bluestack/dm_prim.h>

/*! @{ Macros to make connection manager messages. */
#define MAKE_CM_MSG(TYPE) MESSAGE_MAKE(message, TYPE##_T)

#define MAKE_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); \
                            prim->type = TYPE;

#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); \
                            prim->common.op_code = TYPE; \
                            prim->common.length = sizeof(TYPE##_T);
/*! @} */

/******************************************************************************/
void conManagerSendWritePageTimeout(uint16 page_timeout)
{
    DEBUG_LOG("conManagerSendWritePageTimeout");
    
    MAKE_PRIM_C(DM_HCI_WRITE_PAGE_TIMEOUT_REQ);
    prim->page_timeout = page_timeout;
    VmSendDmPrim(prim);
}

/******************************************************************************/
void conManagerSendOpenTpAclRequest(const tp_bdaddr* tpaddr)
{
    /* Send DM_ACL_OPEN_REQ to open ACL manually */
    MAKE_PRIM_T(DM_ACL_OPEN_REQ);
    BdaddrConvertTypedVmToBluestack(&prim->addrt, &tpaddr->taddr);
    
    prim->flags = 0;
    
    if(tpaddr->transport == TRANSPORT_BLE_ACL)
    {
        prim->flags |= DM_ACL_FLAG_ULP;
    }
    
    VmSendDmPrim(prim);
}

/******************************************************************************/
void conManagerSendOpenTpAclRequestInternally(cm_connection_t *connection)
{
    MAKE_CM_MSG(CON_MANAGER_INTERNAL_MSG_OPEN_TP_ACL);
    message->tpaddr = connection->tpaddr;

    DEBUG_LOG("conManagerSendOpenTpAclRequestInternally");

    MessageSend(ConManagerGetConManagerTask(), CON_MANAGER_INTERNAL_MSG_OPEN_TP_ACL, message);
    ConManagerSetConnectionState(connection, ACL_CONNECTING_INTERNAL);
}

/******************************************************************************/
void conManagerSendCloseTpAclRequest(const tp_bdaddr* tpaddr, bool force)
{
    /* Send DM_ACL_CLOSE_REQ to relinquish control of ACL */
    MAKE_PRIM_T(DM_ACL_CLOSE_REQ);
    BdaddrConvertTypedVmToBluestack(&prim->addrt, &tpaddr->taddr);

    prim->flags = 0;

    if(force)
    {
        prim->flags |= DM_ACL_FLAG_FORCE;
        prim->reason = HCI_ERROR_OETC_USER;
    }

    if(tpaddr->transport == TRANSPORT_BLE_ACL)
    {
        prim->flags |= DM_ACL_FLAG_ULP;
    }
    
    VmSendDmPrim(prim);
}

/******************************************************************************/
void ConManagerSetupRoleSwitchPolicy(void)
{
    static uint16 connectionDmRsTable = 0;
    MAKE_PRIM_T(DM_LP_WRITE_ROLESWITCH_POLICY_REQ);
    prim->version = 0;
    prim->length = 1;
    prim->rs_table = (uint16 *)&connectionDmRsTable;
    VmSendDmPrim(prim);
}

/******************************************************************************/
void ConManagerSendCloseAclRequest(const bdaddr *addr, bool force)
{
    tp_bdaddr tpaddr;
    BdaddrTpFromBredrBdaddr(&tpaddr, addr);
    conManagerSendCloseTpAclRequest(&tpaddr, force);
}

/******************************************************************************/
void conManagerSendInternalMsgUpdateQos(cm_connection_t* connection)
{
    Task task = ConManagerGetTask(connection);
    DEBUG_LOG("conManagerSendInternalMsgUpdateQos");
    
    if(task)
    {
        const tp_bdaddr* tpaddr = ConManagerGetConnectionTpAddr(connection);
        PanicNull((void *)tpaddr);

        MAKE_CM_MSG(CON_MANAGER_INTERNAL_MSG_UPDATE_QOS);
        message->tpaddr = *tpaddr;
        
        MessageCancelFirst(task, CON_MANAGER_INTERNAL_MSG_UPDATE_QOS);
        MessageSend(task, CON_MANAGER_INTERNAL_MSG_UPDATE_QOS, message);
    }
}

/******************************************************************************/
static void conManagerHandleUpdateQos(CON_MANAGER_INTERNAL_MSG_UPDATE_QOS_T* message)
{
    ConManagerApplyQosOnConnect(&message->tpaddr);
}

/******************************************************************************/
void ConManagerConnectionHandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case CON_MANAGER_INTERNAL_MSG_UPDATE_QOS:
            conManagerHandleUpdateQos((CON_MANAGER_INTERNAL_MSG_UPDATE_QOS_T*)message);
            break;
        
        default:
            break;
    }
}
