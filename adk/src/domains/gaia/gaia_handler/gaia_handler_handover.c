/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Gaia Handover interfaces

*/
#if defined(INCLUDE_MIRRORING) && defined (INCLUDE_DFU)
#include "app_handover_if.h"
#include "gaia_framework_internal.h"

#include <logging.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool gaiaHandlerHandover_Veto(void);

static void gaiaHandlerHandover_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/

/* This macro will register the interfaces to be called during handover. No marshalling
 * will take place */
 REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(GAIA, gaiaHandlerHandover_Veto, gaiaHandlerHandover_Commit);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/
/*!
    \brief Check if GAIA has a BREDR connection
    \return bool
*/
static bool gaiaHandlerHandover_IsBredrConnection(void)
{
    bool is_bredr = FALSE;

    GAIA_TRANSPORT *transport = GaiaGetTransport();

    if(transport)
    {
        switch(GaiaTransportGetType(transport))
        {
            case gaia_transport_gatt:
            case gaia_transport_none:
                break;
            default:
                is_bredr = TRUE;
                break;
        }
    }
    return is_bredr;
}

/*!
    \brief Handle Veto check during handover

    If GAIA is connected over BREDR and we attempt a handover,
    handover will fail. Veto the handover and disconnect GAIA.

    \return bool
*/
static bool gaiaHandlerHandover_Veto(void)
{
    bool veto = FALSE;

    DEBUG_LOG("gaiaHandlerHandover_Veto");

    /* This function MUST NOT prevent GAIA connections. */

    /* Only disconnect GAIA and Veto if connected over BREDR. If GAIA is
     * connected over LE it will be disconnected elsewhere */
    if(gaiaHandlerHandover_IsBredrConnection())
    {
        gaiaFrameworkInternal_GaiaDisconnect();
        veto = TRUE;
        DEBUG_LOG("gaiaHandlerHandover_Veto, Vetoed");
    }

    return veto;
}

/*!
    \brief Component commits to the specified role

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void gaiaHandlerHandover_Commit(bool is_primary)
{
    DEBUG_LOG("gaiaHandlerHandover_Commit");
    UNUSED(is_primary);
}

#endif /* defined(INCLUDE_MIRRORING) && defined (INCLUDE_DFU) */
