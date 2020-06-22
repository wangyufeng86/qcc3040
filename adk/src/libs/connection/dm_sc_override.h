#ifndef DM_SC_OVERRIDE_H
#define DM_SC_OVERRIDE_H

#include "connection_private.h"
#include "bdaddr.h"

#include <app/bluestack/dm_prim.h>

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
        );


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
        );


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
        );


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
        );

#endif // DM_SC_OVERRIDE_H
