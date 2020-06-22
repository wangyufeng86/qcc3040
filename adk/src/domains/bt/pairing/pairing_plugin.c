/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Pairing plugin interface
*/

#include "pairing.h"
#include "pairing_plugin.h"

#include <panic.h>
#include <stdlib.h>

static pairing_plugin_t pairing_plugin;
static CL_SM_USER_CONFIRMATION_REQ_IND_T* pending_confirmation_ind;

static void pairing_PluginResetPending(void)
{
    if(pending_confirmation_ind)
    {
        free(pending_confirmation_ind);
        pending_confirmation_ind = NULL;
    }
}

void Pairing_PluginInit(void)
{
    memset(&pairing_plugin, 0, sizeof(pairing_plugin));
    pending_confirmation_ind = NULL;
}

bool Pairing_PluginIsRegistered(void)
{
    return pairing_plugin.handle_remote_io_capability != NULL;
}

void Pairing_PluginRegister(pairing_plugin_t plugin)
{
    PanicNull((void*)plugin.handle_remote_io_capability);
    PanicNull((void*)plugin.handle_io_capability_req);
    PanicNull((void*)plugin.handle_user_confirmation_req);
    
    PanicNotNull((void*)pairing_plugin.handle_remote_io_capability);
    PanicNotNull((void*)pairing_plugin.handle_io_capability_req);
    PanicNotNull((void*)pairing_plugin.handle_user_confirmation_req);
    
    pairing_plugin = plugin;
}

bool Pairing_PluginHandleRemoteIoCapability(const CL_SM_REMOTE_IO_CAPABILITY_IND_T *ind)
{
    if(pairing_plugin.handle_remote_io_capability)
    {
        pairing_plugin.handle_remote_io_capability(ind);
        return TRUE;
    }
    return FALSE;
}

bool Pairing_PluginHandleIoCapabilityRequest(const CL_SM_IO_CAPABILITY_REQ_IND_T *ind, pairing_io_capability_rsp_t* response)
{
    if(pairing_plugin.handle_io_capability_req)
    {
        *response = pairing_plugin.handle_io_capability_req(ind);
        return TRUE;
    }
    return FALSE;
}

bool Pairing_PluginHandleUserConfirmationRequest(const CL_SM_USER_CONFIRMATION_REQ_IND_T *ind, pairing_user_confirmation_rsp_t* response)
{
    if(!pairing_plugin.handle_user_confirmation_req)
    {
        return FALSE;
    }
    
    *response = pairing_plugin.handle_user_confirmation_req(ind);
    
    if(*response == pairing_user_confirmation_wait)
    {
        pending_confirmation_ind = malloc(sizeof(CL_SM_USER_CONFIRMATION_REQ_IND_T));
        
        if(pending_confirmation_ind)
        {
            *pending_confirmation_ind = *ind;
        }
        else
        {
            /* Couldn't wait, override plugin and reject */
            *response = pairing_user_confirmation_reject;
        }
    }
    return TRUE;
}

bool Pairing_PluginRetryUserConfirmation(void)
{
    if(pairing_plugin.handle_user_confirmation_req && pending_confirmation_ind)
    {
        pairing_user_confirmation_rsp_t response;
        Pairing_PluginHandleUserConfirmationRequest(pending_confirmation_ind, &response);
        
        if(response != pairing_user_confirmation_wait)
        {
            bool accept = (response != pairing_user_confirmation_reject);
            ConnectionSmUserConfirmationResponse(&pending_confirmation_ind->tpaddr, accept);
            pairing_PluginResetPending();
        }
        return TRUE;
    }
    return FALSE;
}

void Pairing_PluginPairingComplete(void)
{
    pairing_PluginResetPending();
}

void Pairing_PluginUnregister(pairing_plugin_t plugin)
{
    PanicFalse(pairing_plugin.handle_remote_io_capability == plugin.handle_remote_io_capability);
    PanicFalse(pairing_plugin.handle_io_capability_req == plugin.handle_io_capability_req);
    PanicFalse(pairing_plugin.handle_user_confirmation_req == plugin.handle_user_confirmation_req);
    
    memset(&pairing_plugin, 0, sizeof(pairing_plugin));
    pending_confirmation_ind = NULL;
}
