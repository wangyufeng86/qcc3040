/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionReplaceIrk.c        

DESCRIPTION
    This file contains the management entity responsible duplicating root
	keys in the TDL for Primary and Secondary devices

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"
#include "connection_tdl.h"
#include "bdaddr.h"

#include    <message.h>
#include    <string.h>
#include    <vm.h>
#include    <ps.h>

/*****************************************************************************/
void ConnectionReplaceIrk(Task theAppTask, const bdaddr *peer_addr, cl_irk* irk)
{

    PanicFalse(peer_addr);
    PanicFalse(irk);

    MAKE_CL_MESSAGE(CL_INTERNAL_SM_AUTH_REPLACE_IRK_REQ);
    message->theAppTask = theAppTask;
    message->bd_addr = *peer_addr;
    message->new_irk = *irk;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_AUTH_REPLACE_IRK_REQ, message);
}
