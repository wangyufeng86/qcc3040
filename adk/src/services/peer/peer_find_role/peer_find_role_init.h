/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Initialisation functions for the Peer find role service (using LE)
*/

#ifndef PEER_FIND_ROLE_INIT_H_
#define PEER_FIND_ROLE_INIT_H_

#include "peer_find_role.h"
#include "peer_find_role_private.h"

/*! Finish the find role procedure sending a message to registered tasks 

    If required any links will be disconnected.

    \param  role    The identifier of the message to send
*/
void peer_find_role_completed(peer_find_role_message_t role);


/*! Cancel the timeout for finding a peer

    If there is a timer running after a call to PeerFindRole_FindRole(), then
    the timeout is cancelled.
 */
void peer_find_role_cancel_initial_timeout(void);


/*! inform clients that we did not find our role in the requested time

    Send #PEER_FIND_ROLE_ACTING_PRIMARY message to clients.
    This is a response in the PeerFindRole_FindRole() procedure.
 */
void peer_find_role_notify_timeout(void);


/*! if a specific response is pending inform clients that we have finished

    When a primary role is selected the clients are notified immediately.
    Secondary roles delay reporting until all activity is known to have 
    stopped */
void peer_find_role_notify_clients_if_pending(void);

/*! Send a "prepare for role selection" request to the registered client */
void peer_find_role_request_prepare_for_role_selection(void);

/*! Initiate disconnection of the peer link - if active

    \note This does not check whether the link is still in the process
        of disconnection.
    
    \return TRUE If disconnection was required
 */
bool peer_find_role_disconnect_link(void);


/*! Update the media status

    Update activities that might affect operation of find role.
    These are normally media activities, such as music or voice
    calls.

    \note When we set a media flag we queue a message to be sent when
    we later go inactive.

    \param busy Whether the mask is to be set or cleared
    \param mask The bit mask to apply
 */
void peer_find_role_update_media_flag(bool busy, peerFindRoleMediaBusyStatus_t mask);


/*! Transition to next state after link has disconnected

    We may want to restart device discovery in case the selected
    roles were not established correctly, in which case we will
    transition to #PEER_FIND_ROLE_STATE_DISCOVERY, otherwise
    #PEER_FIND_ROLE_STATE_INITIALISED.
 */
void peer_find_role_select_state_after_completion(void);

/*! Checks to see if we have a valid figure of merit from the peer.
    If so calculates the figure of merit and notifies the peer of 
    it's new role.
    If we have fixed roles the peer will ne notified immediately..
*/
void PeerFindRole_DecideRoles(void);

#endif /* PEER_FIND_ROLE_INIT_H_ */
