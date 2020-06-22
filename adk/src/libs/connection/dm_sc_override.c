/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_sc_override.c

DESCRIPTION
    This module is for Secure Connections Override functionality.

NOTES

*/
#ifndef DM_SC_OVERRIDE_C
#define DM_SC_OVERRIDE_C

#include "dm_sc_override.h"

#include <vm.h>

/****************************************************************************
NAME
    sendScOverrideCfm

DESCRIPTION
    Static function to send the CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM message.

RETURNS
   void
*/
static void sendScOverrideCfm(
        Task                    theAppTask,
        hci_status              status,
        const bdaddr*           addr,
        sc_override_action_t    action
        )
{
    MAKE_CL_MESSAGE(CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM);
    message->status = status;
    message->addr = *addr;
    message->override_action = action;
    MessageSend(theAppTask, CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM, message);
}

/****************************************************************************
NAME
    ConnectionDmSecureConnectionsOverrideReq

DESCRIPTION
    Send an internal message so that this can be serialised to Bluestack.

RETURNS
   void
*/
void ConnectionDmSecureConnectionsOverrideReq(
            Task                    theAppTask,
            const bdaddr*           addr,
            sc_override_action_t    override_action
            )
{
    if (!theAppTask)
    {
        CL_DEBUG(("'theAppTask' is NULL\n"));
    }
    else if (!addr)
    {
        CL_DEBUG(("The 'addr' is NULL\n"));
    }
    else
    {
        switch(override_action)
        {
            case sc_override_action_disable:
                /* FALLTHROUGH */
            case sc_override_action_enable:
                /* FALLTHROUGH */
            case sc_override_action_delete:
                {
                    MAKE_CL_MESSAGE(CL_INTERNAL_DM_SC_OVERRIDE_REQ);

                    message->theAppTask = theAppTask;
                    message->addr = *addr;
                    message->override_action = (uint8)(0xFF & override_action);

                    MessageSend(
                                connectionGetCmTask(),
                                CL_INTERNAL_DM_SC_OVERRIDE_REQ,
                                message
                                );
                }
                break;

            /* Invalid Override Action */
            default:
                sendScOverrideCfm(
                            theAppTask,
                            hci_error_illegal_command,
                            addr,
                            override_action
                            );
                break;
        } /* end Switch */
    } /* end if else */
}


/****************************************************************************
NAME
    connectionHandleDmScOverrideReq

DESCRIPTION
    Handle the internal message sent by
    ConnectionDmSecureConnectionsOverrideReq().

    If there is no other Secure Connections Override scenario on-going, convert
    to a Bluestack prim and send, otherwise queue the message until the current
    scenario is completed.

RETURNS
   void
*/
void connectionHandleDmScOverrideReq(
        connectionDmScOverrideState *state,
        const CL_INTERNAL_DM_SC_OVERRIDE_REQ_T *req
        )
{
    if (!state->DmScOverrideLock)
    {
        MAKE_PRIM_T(DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ);

        state->DmScOverrideLock = req->theAppTask;

        BdaddrConvertVmToBluestack(&prim->bd_addr,&req->addr);
        prim->host_support_override = (uint8_t)req->override_action;

        VmSendDmPrim(prim);
    }
    else    /* There is a Secure Connections Override scernario on-going. */
    {
        MAKE_CL_MESSAGE(CL_INTERNAL_DM_SC_OVERRIDE_REQ);
        COPY_CL_MESSAGE(req, message);
        MessageSendConditionallyOnTask(
                    connectionGetCmTask(),
                    CL_INTERNAL_DM_SC_OVERRIDE_REQ,
                    message,
                    &state->DmScOverrideLock
                    );
    }
}


/****************************************************************************
NAME
    connectionHandleDmScOverrideCfm

DESCRIPTION
    Handle DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM from Bluestack, in response
    to writing the override for bdaddr. Converts the PRIM to a CL CFM message
    and sends it to the task that initiated the message scenario.

RETURNS
   void
*/
void connectionHandleDmScOverrideCfm(
        connectionDmScOverrideState *state,
        const DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM_T *cfm
        )
{
    if (state->DmScOverrideLock)
    {
        bdaddr addr;

        BdaddrConvertBluestackToVm(&addr, &cfm->bd_addr);

        sendScOverrideCfm(
                    state->DmScOverrideLock,
                    connectionConvertHciStatus(cfm->status),
                    &addr,
                    (sc_override_action_t)cfm->host_support_override
                    );

        /* Now that the CFM message has been sent, free the serialisation lock. */
        state->DmScOverrideLock = NULL;
    }
    else
    {
        CL_DEBUG(("CL_DM_SECURE_CONNECTIONS_OVERRIDE_CFM received without Lock\n"));
    }

}


/****************************************************************************
NAME
    ConnectionDmSecureConnectionsOverrideMaxBdaddrReq

DESCRIPTION
    Request the maximum number of BDADDR that can have an SC Override set.

RETURNS
   void
*/
void ConnectionDmSecureConnectionsOverrideMaxBdaddrReq(Task theAppTask)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_SC_OVERRIDE_MAX_BDADDR_REQ);

    message->theAppTask = theAppTask;

    MessageSend(
                connectionGetCmTask(),
                CL_INTERNAL_DM_SC_OVERRIDE_MAX_BDADDR_REQ,
                message
                );
}


/****************************************************************************
NAME
    connectionHandleDmScOverrideMaxBdaddrReq

DESCRIPTION
    Convert the internal CL message to DM Prim, or serialise if there is an
    ongoing message scenario for Secure Connections Override.

RETURNS
   void
*/
void connectionHandleDmScOverrideMaxBdaddrReq(
        connectionDmScOverrideState *state,
        const CL_INTERNAL_DM_SC_OVERRIDE_MAX_BDADDR_REQ_T* req
        )
{

    if (!state->DmScOverrideLock)
    {
        MAKE_PRIM_T(DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_REQ);

        state->DmScOverrideLock = req->theAppTask;

        VmSendDmPrim(prim);
    }
    else    /* There is a Secure Connections Override scernario on-going. */
    {
        MAKE_CL_MESSAGE(CL_INTERNAL_DM_SC_OVERRIDE_MAX_BDADDR_REQ);
        COPY_CL_MESSAGE(req, message);
        MessageSendConditionallyOnTask(
                    connectionGetCmTask(),
                    CL_INTERNAL_DM_SC_OVERRIDE_MAX_BDADDR_REQ,
                    message,
                    &state->DmScOverrideLock
                    );
    }
}


/****************************************************************************
NAME
    connectionHandleDmScOverrideMaxBdaddrCfm

DESCRIPTION
    Handle the DM SC prim for maximum number of BDADDR that can be overriden,
    convert to CL message and send to the app task that initiated the message
    scenario.

RETURNS
   void
*/
void connectionHandleDmScOverrideMaxBdaddrCfm(
        connectionDmScOverrideState *state,
        const DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM_T *cfm
        )
{
    if (state->DmScOverrideLock)
    {
        MAKE_CL_MESSAGE(CL_DM_SECURE_CONNECTIONS_OVERRIDE_MAX_BDADDR_CFM);

        message->status = connectionConvertHciStatus(cfm->status);
        message->max_override_bdaddr = (uint8)cfm->max_override_bdaddr;

        MessageSend(
                    state->DmScOverrideLock,
                    CL_DM_SECURE_CONNECTIONS_OVERRIDE_MAX_BDADDR_CFM,
                    message
                    );

        /* Now that the CFM message has been sent, free the serialisation lock. */
        state->DmScOverrideLock = NULL;
    }
    else
    {
        CL_DEBUG(("DM_READ_SC_HOST_SUPPORT_OVERRIDE_MAX_BD_ADDR_CFM_T received without Lock\n"));
    }

}

#endif // DM_SC_OVERRIDE_C
