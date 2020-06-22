#ifdef INCLUDE_AMA
/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Provides TWS support in the accessory domain
*/

#include "ama.h"
#include "ama_tws.h"
#include "ama_rfcomm.h"

#include <logging.h>
#include <message.h>
#include <panic.h>
#include <stdio.h>

#include "bt_device.h"
#include "tws_topology_role_change_client_if.h"
#include "ama_debug.h"


typedef struct
{
    bool role_state_initialised;
    Task serverTask;
    int32 min_reconnection_delay;
    tws_topology_role current_role;
} ama_tws_data_t;

static ama_tws_data_t ama_tws_data = { .role_state_initialised = FALSE, .serverTask = NULL, .min_reconnection_delay = D_SEC(2),
                                                    .current_role = tws_topology_role_none };

static void amaTws_SendRoleChangePrepareResponse(void)
{
    DEBUG_LOG("ama_tws_SendRoleChangePrepareResponse");
    MessageSend(PanicNull(ama_tws_data.serverTask), TWS_ROLE_CHANGE_PREPARATION_CFM, NULL);
}

void AmaTws_HandleLocalDisconnectionCompleted(void)
{
    DEBUG_LOG("AmaTws_HandleLocalDisconnectionCompleted");
    amaTws_SendRoleChangePrepareResponse();
}

static void amaTws_AllowReconnections(void)
{
    DEBUG_LOG("amaTws_AllowReconnections - current_role %d", ama_tws_data.current_role);
    if(ama_tws_data.current_role == tws_topology_role_primary)
    {
        MessageSend(AmaRfcomm_GetTask(), AMA_RFCOMM_LOCAL_ALLOW_CONNECTIONS_IND, NULL);
    }
}

static void amaTws_DisconnectIfRequired(void)
{
    DEBUG_LOG("amaTws_DisconnectIfRequired");
    if(ama_tws_data.current_role == tws_topology_role_primary)
    {
        DEBUG_LOG("amaTws_Disconnect");

        MessageSend(AmaRfcomm_GetTask(), AMA_RFCOMM_LOCAL_DISCONNECT_REQ_IND, NULL);
    }
}

static void amaTws_SendRoleChangeRequestResponse(void)
{
    DEBUG_LOG("amaTws_SendRoleChangeRequestResponse");
    MAKE_TWS_ROLE_CHANGE_ACCEPTANCE_MESSAGE(TWS_ROLE_CHANGE_ACCEPTANCE_CFM);
    message->role_change_accepted = TRUE;

    MessageSend(PanicNull(ama_tws_data.serverTask), TWS_ROLE_CHANGE_ACCEPTANCE_CFM, message);
}

static void amaTws_Initialise(Task server, int32_t reconnect_delay)
{
    DEBUG_LOG("amaTws_Initialise server = 0x%p, reconnect delay ");
    ama_tws_data.min_reconnection_delay = reconnect_delay;
    ama_tws_data.serverTask = server;
}

static void amaTws_RoleChangeIndication(tws_topology_role role)
{
    DEBUG_LOG("amaTws_RoleChangeIndication role=0x%x", role);
    ama_tws_data.current_role = role;
    if(ama_tws_data.role_state_initialised)
    {
        amaTws_AllowReconnections();
    }
    else
    {
        ama_tws_data.role_state_initialised = TRUE;
    }
}

static void amaTws_ProposeRoleChange(void)
{
    DEBUG_LOG("amaTws_ProposeRoleChange");
    amaTws_SendRoleChangeRequestResponse();
}

static void amaTws_ForceRoleChange(void)
{
    DEBUG_LOG("amaTws_ForceRoleChange");
    amaTws_DisconnectIfRequired();
}

static void amaTws_PrepareRoleChange(void)
{
    DEBUG_LOG("amaTws_PrepareRoleChange");
    amaTws_DisconnectIfRequired();
    amaTws_SendRoleChangePrepareResponse();
}

static void amaTws_CancelRoleChange(void)
{
    DEBUG_LOG("amaTws_CancelRoleChange");
    amaTws_AllowReconnections();
}



TWS_ROLE_CHANGE_CLIENT_REGISTRATION_MAKE(amaTws, amaTws_Initialise, amaTws_RoleChangeIndication,
                                                                                amaTws_ProposeRoleChange, amaTws_ForceRoleChange,
                                                                                amaTws_PrepareRoleChange, amaTws_CancelRoleChange);

#ifdef HOSTED_TEST_ENVIRONMENT

const role_change_client_callback_t * ama_tws_GetClientCallbacks(void)
{
    return &role_change_client_registrations_ama_tws;
}

void Ama_tws_Reset(void)
{
    memset(&ama_tws_data, 0, sizeof(ama_tws_data));
}

#endif /* HOSTED_TEST_ENVIRONMENT */
#endif /* INCLUDE_AMA */
