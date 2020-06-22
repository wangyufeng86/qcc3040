/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    connection_handover.c

DESCRIPTION
    TWS Marshaling interface for the Connection Library
    
NOTES
    See handover_if.h for further interface description
    
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER    
*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"
#include "init.h"
#include "handover_if.h"

#include <vm.h>
#include <string.h>


static bool connectionVeto(void);

static bool connectionMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written);

static bool connectionUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed);

static void connectionHandoverCommit(const tp_bdaddr *tp_bd_addr, 
                                     const bool newRole );

static void connectionHandoverComplete( const bool newRole );

static void connectionHandoverAbort( void );

extern const handover_interface connection_handover_if =  {
        &connectionVeto,
        &connectionMarshal,
        &connectionUnmarshal,
        &connectionHandoverCommit,
        &connectionHandoverComplete,
        &connectionHandoverAbort};


/****************************************************************************
NAME    
    connectionVeto

DESCRIPTION
    Veto check for Connection library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the Connection Library wishes to veto the handover attempt.
*/
bool connectionVeto( void )
{
    uint16 cmTaskMsgs = 0;
    
    /* Check the connection library is initialized */
    if(!connectionGetInitState())
        return TRUE;

    /* Check Lock Status */
    if(connectionGetLockState())
        return TRUE;
    
    /* Check for outstanding scan requests */
    if(connectionOutstandingWriteScanEnableReqsGet() != 0)
        return TRUE;

    /* Check for pending messages on the application task */
    if(MessagesPendingForTask(connectionGetAppTask(), NULL) != 0)
    {
       return TRUE;
    }
    
    /* Check for pending messages on the Connection library task,
       One DM_PRIM message is allowed as this is expected at the
       time of veto check. */
    cmTaskMsgs = MessagesPendingForTask(connectionGetCmTask(), NULL);
    if (MessagePendingFirst(connectionGetCmTask(), MESSAGE_BLUESTACK_DM_PRIM, NULL))
    {
        if (cmTaskMsgs > 1)
        {
            return TRUE;
        }
    }
    else if(cmTaskMsgs != 0)
    {
       return TRUE;
    }

    return FALSE;
}

/****************************************************************************
NAME    
    connectionMarshal

DESCRIPTION
    Marshal the data associated with the Connection Library

RETURNS
    bool TRUE Connection library marshalling complete, otherwise FALSE
*/
bool connectionMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(written);
    /* Nothing to be done */
    return TRUE;
}

/****************************************************************************
NAME    
    connectionUnmarshal

DESCRIPTION
    Unmarshal the data associated with the Connection Library

RETURNS
    bool TRUE if Connection Library unmarshalling complete, otherwise FALSE
*/
bool connectionUnmarshal(const tp_bdaddr *tp_bd_addr,
                                   const uint8 *buf,
                                   uint16 length,
                                   uint16 *consumed)
{
    UNUSED(tp_bd_addr);
    UNUSED(buf);
    UNUSED(length);
    UNUSED(consumed);
    /* Nothing to be done */
    return TRUE;
}

/****************************************************************************
NAME    
    connectionHandoverCommit

DESCRIPTION
    The Connection library performs time-critical actions to commit to specified
    new role (primary or secondary)

RETURNS
    void
*/
void connectionHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    UNUSED(tp_bd_addr);
    UNUSED(newRole);

    return;
}

/****************************************************************************
NAME    
    connectionHandoverComplete

DESCRIPTION
    The Connection library performs pending actions and completes transition to
    specified new role (primary or secondary)

RETURNS
    void
*/
void connectionHandoverComplete( const bool newRole )
{
    UNUSED(newRole);

    return;
}

/****************************************************************************
NAME    
    connectionHandoverAbort

DESCRIPTION
    Abort the Connection library Handover process, free any memory
    associated with the marshalling process.

RETURNS
    void
*/
void connectionHandoverAbort( void )
{
    return;
}

