/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       tws_topology_handover.c
\brief      TWS Topology Handover interfaces

*/
#ifdef INCLUDE_MIRRORING
#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "tws_topology_goals.h"
#include "tws_topology_rule_events.h"
#include "tws_topology_private.h"
#include "hdma.h"

#include <panic.h>
#include <logging.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool twsTopology_Veto(void);

static void twsTopology_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/
REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(TWS_TOPOLOGY, twsTopology_Veto, twsTopology_Commit);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! 
    \brief Handle Veto check during handover
    \return bool
*/
static bool twsTopology_Veto(void)
{
    bool veto = FALSE;

    /* Veto if there are pending goals */
    if (TwsTopology_IsAnyGoalPending())
    {
        veto = TRUE;
        DEBUG_LOG("twsTopology_Veto, Pending goals");
    }
    
    return veto;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void twsTopology_Commit(bool is_primary)
{
    if (is_primary)
    {
        DEBUG_LOG("twsTopology_Commit, Create HDMA, Set Role Primary");
        twsTopology_CreateHdma();
        twsTopology_SetRole(tws_topology_role_primary);
        twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_ROLE_SWITCH);
    }
    else
    {
        DEBUG_LOG("twsTopology_Commit, Destroy HDMA, Set Role Secondary");
        twsTopology_DestroyHdma();
        /* Don't set role here, the procedure sets the role later when handover
           script is complete */
    }
}

#endif /* INCLUDE_MIRRORING */
