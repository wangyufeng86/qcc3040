/****************************************************************************
Copyright (c) 2004 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionGetRole.c        

DESCRIPTION
    This file contains the implementation of the link policy management 
    entity. This is responsible for arbitrating between the different low 
    power mode requirements of the connection library clients.

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"


/*****************************************************************************/
void ConnectionGetRole(Task theAppTask, Sink sink)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_GET_ROLE_REQ);
    message->theAppTask = theAppTask;
    message->sink = sink;
	/* message->bd_addr does not need to be set. */
    MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_GET_ROLE_REQ, message);
}

void ConnectionGetRoleBdaddr(Task theAppTask, const bdaddr *bd_addr)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_GET_ROLE_REQ);
    message->theAppTask = theAppTask;
    message->sink = (Sink)NULL; /* make sink to NULL to use bd_addr instead */
    message->bd_addr = *bd_addr;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_GET_ROLE_REQ, message);
}

