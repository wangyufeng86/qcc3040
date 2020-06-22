/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_.c
\brief      
*/

/* local includes */
#include "state_proxy.h"
#include "state_proxy_private.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_pairing.h"
#include "state_proxy_flags.h"

/* framework includes */
#include <pairing.h>
#include <peer_signalling.h>
#include <bt_device.h>

/* system includes */
#include <panic.h>
#include <logging.h>
#include <stdlib.h>

void stateProxy_GetInitialPairingState(void)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    bdaddr active_handset_addr;
    bdaddr paired_handset_addr;

    proxy->local_state->flags.is_pairing = FALSE;
    BdaddrSetZero(&active_handset_addr);
    BdaddrSetZero(&paired_handset_addr);
    if (appDeviceGetHandsetBdAddr(&paired_handset_addr))
    {
        if (appDeviceIsHandsetAnyProfileConnected())
        {
            active_handset_addr = paired_handset_addr;
        }
    }
    proxy->local_state->flags.has_handset_pairing = !BdaddrIsZero(&paired_handset_addr);;
    proxy->local_state->handset_addr = active_handset_addr;
}

void stateProxy_HandlePairingHandsetActivity(const PAIRING_ACTIVITY_T* pha)
{
    DEBUG_LOG("stateProxy_HandlePairingHandsetActivity %u", pha->status);

    /* only interested in (Not)InProgress for notifying pairing
     * activity */
    if (pha->status == pairingInProgress ||
        pha->status == pairingNotInProgress)
    {
        bool is_pairing = pha->status == pairingInProgress;
        
        stateProxy_FlagIndicationHandler(MARSHAL_TYPE_PAIRING_ACTIVITY_T,
                                         is_pairing, pha,
                                         sizeof(PAIRING_ACTIVITY_T));
    }

}

void stateProxy_HandleRemotePairingHandsetActivity(const PAIRING_ACTIVITY_T* pha)
{
    stateProxy_RemoteFlagIndicationHandler(MARSHAL_TYPE_PAIRING_ACTIVITY_T,
                                           pha->status == pairingInProgress ? TRUE:FALSE,
                                           pha);
}


void stateProxy_handleActiveHandsetAddr(const state_proxy_active_handset_addr_t* aha)
{
    state_proxy_task_data_t *proxy = stateProxy_GetTaskData();
    proxy->remote_state->handset_addr = aha->active_handset_addr;
}

