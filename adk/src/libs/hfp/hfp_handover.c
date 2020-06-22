/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_handover.c

DESCRIPTION
    This file is a stub for HFP handover logic (Veto, Marshals/Unmarshals,
    Handover, etc).
 
NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
 */

#include "hfp_marshal_desc.h"
#include "hfp_private.h"
#include "hfp_init.h"
#include "hfp_link_manager.h"

static bool hfpVeto(void);

static bool hfpMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written);

static bool hfpUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed);

static void hfpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);

static void hfpHandoverComplete( const bool newRole );

static void hfpHandoverAbort( void );

extern const handover_interface hfp_handover_if =  {
        &hfpVeto,
        &hfpMarshal,
        &hfpUnmarshal,
        &hfpHandoverCommit,
        &hfpHandoverComplete,
        &hfpHandoverAbort};


/****************************************************************************
NAME    
    hfpHandoverAbort

DESCRIPTION
    Abort the HFP Handover process, free any memory
    associated with the marshalling process.

RETURNS
    void
*/
static void hfpHandoverAbort(void)
{
    /* Not supported in this configuration */
    Panic();

    return;
}

/****************************************************************************
NAME    
    hfpMarshal

DESCRIPTION
    Marshal the data associated with HFP connections

RETURNS
    bool TRUE if HFP module marshalling complete, otherwise FALSE
*/
static bool hfpMarshal(const tp_bdaddr *tp_bd_addr,
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
    Unmarshal the data associated with the HFP connections

RETURNS
    bool TRUE if HFP unmarshalling complete, otherwise FALSE
*/
static bool hfpUnmarshal(const tp_bdaddr *tp_bd_addr,
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
    hfpVeto

DESCRIPTION
    Veto check for HFP library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the HFP Library wishes to veto the handover attempt.
*/
bool hfpVeto( void )
{
    /* Not supported in this configuration */
    Panic();
    
    return TRUE;
}

/****************************************************************************
NAME    
    hfpHandoverCommit

DESCRIPTION
    The HFP library performs time-critical actions to commit to the specified
    new role (primary or secondary)

RETURNS
    void
*/
static void hfpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    UNUSED(tp_bd_addr);
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

/****************************************************************************
NAME    
    hfpHandoverComplete

DESCRIPTION
    The HFP library performs pending actions and completes transition to 
    specified new role (primary or secondary)

RETURNS
    void
*/
static void hfpHandoverComplete( const bool newRole )
{
    UNUSED(newRole);
    
    /* Not supported in this configuration */
    Panic();
    
    return;
}

