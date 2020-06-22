/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    avrcp_handover.c

DESCRIPTION
    This file is a stub for AVRCP handover logic (Veto, Marshals/Unmarshals,
    Handover, etc).

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


#include "avrcp_private.h"
#include "avrcp_init.h"
#include <panic.h>

static bool avrcpVeto( void );
static bool avrcpMarshal(const tp_bdaddr *tp_bd_addr,
                         uint8 *buf,
                         uint16 length,
                         uint16 *written);
static bool avrcpUnmarshal(const tp_bdaddr *tp_bd_addr,
                           const uint8 *buf,
                           uint16 length,
                           uint16 *consumed);
static void avrcpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);
static void avrcpHandoverComplete( const bool newRole );
static void avrcpHandoverAbort(void);


const handover_interface avrcp_handover =
{
    avrcpVeto,
    avrcpMarshal,
    avrcpUnmarshal,
    avrcpHandoverCommit,
    avrcpHandoverComplete,
    avrcpHandoverAbort
};


/****************************************************************************
NAME    
    avrcpHandoverAbort

DESCRIPTION
    Abort the AVRCP library Handover process, free any memory
    associated with the marshalling process.

RETURNS
    void
*/
static void avrcpHandoverAbort(void)
{
    /* Not supported in this configuration */
    Panic();

    return;
}

/****************************************************************************
NAME    
    avrcpMarshal

DESCRIPTION
    Marshal the data associated with AVRCP connections

RETURNS
    bool TRUE if AVRCP module marshalling complete, otherwise FALSE
*/
static bool avrcpMarshal(const tp_bdaddr *tp_bd_addr,
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
    avrcpUnmarshal

DESCRIPTION
    Unmarshal the data associated with AVRCP connections

RETURNS
    bool TRUE if AVRCP unmarshalling complete, otherwise FALSE
*/
static bool avrcpUnmarshal(const tp_bdaddr *tp_bd_addr,
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
    avrcpHandoverCommit

DESCRIPTION
    The AVRCP  library performs time-critical actions to commit to the specified
    new role (primary or secondary)

RETURNS
    void
*/
static void avrcpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    UNUSED(tp_bd_addr);
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    avrcpHandoverComplete

DESCRIPTION
    The AVRCP  library performs pending actions and completes transition to 
    specified new role (primary or secondary)

RETURNS
    void
*/
static void avrcpHandoverComplete( const bool newRole )
{
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    avrcpVeto

DESCRIPTION
    Veto check for AVRCP library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the AVRCP Library wishes to veto the handover attempt.
*/
static bool avrcpVeto( void )
{
    /* Not supported in this configuration */
    Panic();
    
    return TRUE;
}





