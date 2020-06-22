/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       peer_signalling_handover.c
\brief      Peer Signalling Handover related interfaces

*/

/* Only compile if Mirroring is defined */
#ifdef INCLUDE_MIRRORING
#include "peer_signalling_private.h"
#include "domain_marshal_types.h"
#include "app_handover_if.h"
#include "adk_log.h"
#include <panic.h>

/******************************************************************************
 * Local Function Prototypes
 ******************************************************************************/
static bool peerSignalling_Veto(void);

static void peerSignalling_Commit(bool is_primary);

/******************************************************************************
 * Global Declarations
 ******************************************************************************/

REGISTER_HANDOVER_INTERFACE_NO_MARSHALLING(PEER_SIG, peerSignalling_Veto, peerSignalling_Commit);

/******************************************************************************
 * Local Function Definitions
 ******************************************************************************/

/*! 
    \brief Handle Veto check during handover
    \return TRUE: If peer signalling component wants to Veto handover.
            FALSE: If peer signalling component doesn't want to Veto handover.
*/
static bool peerSignalling_Veto(void)
{
    peerSigTaskData *the_inst = PeerSigGetTaskData();
    bool veto = FALSE;

    /* Check if there are any pending messages in queue */
    if (MessagesPendingForTask(&the_inst->task, NULL))
    {
        DEBUG_LOG("peerSignalling_Veto, Messages pending for Peer signalling task");
        veto = TRUE;
    }
    else
    {
        /*
         * Veto if any of the following conditions are met
         * 1. State of peer-signalling is not connected
         * 2. Ongoing operations
         * 3. Client task waiting for on-going operation
         * 4. Client tasks has initiated disconnect of peer signalling
         * 5. Messages pending in any of the channel specific message queues
         */
        if (the_inst->state != PEER_SIG_STATE_CONNECTED) 
        {
            DEBUG_LOG("peerSignalling_Veto, Not in connected state(%d)",the_inst->state);
            veto = TRUE;
        }
        else if (TaskList_Size(&the_inst->disconnect_tasks))
        {
            DEBUG_LOG("peerSignalling_Veto, Client task has initiated disconnect of peer-signalling");
            veto = TRUE;
        }
        else if (appPeerSigCheckForPendingMarshalledMsg())
        {
            DEBUG_LOG("peerSignalling_Veto, Pending messages");
            veto = TRUE;
        }

    }

    return veto;
}

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
static void peerSignalling_Commit(bool is_primary)
{
    peerSigTaskData *the_inst = PeerSigGetTaskData();

    /* Set the secondary and handset address in primary role */
    if(is_primary)
    {
        if(!appDeviceGetSecondaryBdAddr(&the_inst->peer_addr))
        {
            DEBUG_LOG("peerSignalling_Commit: Failed to get secondary peer device address ");
            Panic();
        }
    }
    else
    {
        /* Set the primary address as peer address in secondary role */
        PanicFalse(appDeviceGetPrimaryBdAddr(&the_inst->peer_addr));
    }

}
#endif /* INCLUDE_MIRRORING */

