/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    connection_handover.c

DESCRIPTION
    This file is a stub for the TWS Marshaling interface
    
NOTES
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

static void connectionHandoverComplete( const bool newRole);

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
    /* Not supported in this configuration */
    Panic();
    
    return TRUE;
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
    
    /* Not supported in this configuration */
    Panic();
    
    return FALSE;
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
    
    /* Not supported in this configuration */
    Panic();
    
    return FALSE;
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
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    connectionHandoverComplete

DESCRIPTION
    The Connection library performs pending actions and complestes transition to
    specified new role (primary or secondary)

RETURNS
    void
*/
void connectionHandoverComplete( const bool newRole )
{
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
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
    /* Not supported in this configuration */
    Panic();

    return;
}

