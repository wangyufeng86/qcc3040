/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Provides TWS support for the earbud gaia plugin
*/

#include "earbud_gaia_tws.h"

#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdio.h>


#include "earbud_gaia_plugin.h"
#include "tws_topology_role_change_client_if.h"


typedef struct
{
    bool role_state_initialised;
    Task serverTask;
    int32_t reconnection_delay;
    tws_topology_role current_role;
} earbud_gaia_tws_data_t;

#if 0
static void earbudGaiaTws_Initialise(Task server, int32_t reconnect_delay);
static void earbudGaiaTws_RoleChangeIndication(tws_topology_role role);
static void earbudGaiaTws_ProposeRoleChange(void);
static void earbudGaiaTws_ForceRoleChange(void);
static void earbudGaiaTws_PrepareRoleChange(void);
static void earbudGaiaTws_CancelRoleChange(void);


static earbud_gaia_tws_data_t earbud_gaia_tws_data = { .role_state_initialised = FALSE, .serverTask = NULL, .reconnection_delay = D_SEC(6),
                                                       .current_role = tws_topology_role_none };

#endif

bool EarbudGaiaTws_Init(Task task)
{
    UNUSED(task);
    DEBUG_LOG("EarbudGaiaTws_Init");
    return TRUE;
}

#if 0
static void earbudGaiaTws_Initialise(Task server, int32_t reconnect_delay)
{
    DEBUG_LOG("earbudGaiaTws_Initialise server = 0x%p, reconnect delay = %d",server,reconnect_delay);
    earbud_gaia_tws_data.reconnection_delay = reconnect_delay;
    earbud_gaia_tws_data.serverTask = server;
}

static void earbudGaiaTws_RoleChangeIndication(tws_topology_role role)
{
    DEBUG_LOG("earbudGaiaTws_RoleChangeIndication role=0x%x", role);
    earbud_gaia_tws_data.current_role = role;
}

static void earbudGaiaTws_ProposeRoleChange(void)
{
    DEBUG_LOG("earbudGaiaTws_ProposeRoleChange");
}

static void earbudGaiaTws_ForceRoleChange(void)
{
    DEBUG_LOG("earbudGaiaTws_ForceRoleChange");
    EarbudGaiaPlugin_PrimaryAboutToChange(static_handover, EarbudGaiaTws_MobileAppReconnectionDelay());
}

static void earbudGaiaTws_PrepareRoleChange(void)
{
    DEBUG_LOG("earbudGaiaTws_PrepareRoleChange");
}

static void earbudGaiaTws_CancelRoleChange(void)
{
    DEBUG_LOG("earbudGaiaTws_CancelRoleChange");
}

TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(EARBUD_GAIA_TWS, earbudGaiaTws_Initialise, earbudGaiaTws_RoleChangeIndication,
                                        earbudGaiaTws_ProposeRoleChange, earbudGaiaTws_ForceRoleChange,
                                        earbudGaiaTws_PrepareRoleChange, earbudGaiaTws_CancelRoleChange);
#endif
