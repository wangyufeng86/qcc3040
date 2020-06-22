/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_handover.c

DESCRIPTION
    This file is a stub for the TWS Handover and Marshaling interface 
    for A2DP
    
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER 
*/


/* Not supported in this configuration */

#include "a2dp_private.h"
#include "a2dp_init.h"

static bool a2dpVeto(void);

static bool a2dpMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written);

static bool a2dpUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed);

static void a2dpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);

static void a2dpHandoverComplete( const bool newRole );

static void a2dpHandoverAbort( void );

extern const handover_interface a2dp_handover_if =  {
        &a2dpVeto,
        &a2dpMarshal,
        &a2dpUnmarshal,
        &a2dpHandoverCommit,
        &a2dpHandoverComplete,        
        &a2dpHandoverAbort};

/****************************************************************************
NAME    
    a2dpVeto

DESCRIPTION
    Veto check for A2DP library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the A2DP Library wishes to veto the handover attempt.
*/
bool a2dpVeto( void )
{
    /* Not supported in this configuration */
    Panic();
    
    return TRUE;
}

/****************************************************************************
NAME    
    a2dpHandoverCommit

DESCRIPTION
    The A2DP library performs time-critical actions to commit to the specified 
    new role (primary or  secondary)

RETURNS
    void
*/
static void a2dpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    UNUSED(tp_bd_addr);
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    a2dpHandoverComplete

DESCRIPTION
    The A2DP library performs pending actions and completes the transition to 
    the specified new role.

RETURNS
    void
*/
static void a2dpHandoverComplete( const bool newRole )
{
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    a2dpHandoverAbort

DESCRIPTION
    Abort the A2DP Handover process, free any memory
    associated with the marshalling process.

RETURNS
    void
*/
static void a2dpHandoverAbort(void)
{
    /* Not supported in this configuration */
    Panic();

    return;
}

/****************************************************************************
NAME    
    a2dpMarshal

DESCRIPTION
    Marshal the data associated with A2DP connections

RETURNS
    bool TRUE if A2DP module marshalling complete, otherwise FALSE
*/
static bool a2dpMarshal(const tp_bdaddr *tp_bd_addr,
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
    a2dpUnmarshal

DESCRIPTION
    Unmarshal the data associated with the A2DP connections

RETURNS
    bool TRUE if A2DP unmarshalling complete, otherwise FALSE
*/
bool a2dpUnmarshal(const tp_bdaddr *tp_bd_addr,
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

