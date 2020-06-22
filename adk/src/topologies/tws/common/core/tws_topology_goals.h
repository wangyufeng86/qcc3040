/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWS Topology goal handling.
*/

#ifndef TWS_TOPOLOGY_GOALS_H
#define TWS_TOPOLOGY_GOALS_H

#include <message.h>


/*! Definition of goals which the topology may be instructed (by rules) to achieve. */
typedef enum
{
    /*! Empty goal. */
    tws_topology_goal_none,

    /*! Goal is to pair with a peer Earbud. */
    tws_topology_goal_pair_peer,

    /*! Goal is determine the Primary or Secondary role of the Earbud. */
    tws_topology_goal_find_role,

    /*! Goal is to create BREDR ACL to Primary. */
    tws_topology_goal_secondary_connect_peer,

    /*! Goal is to connect peer BREDR profiles to Secondary. */
    tws_topology_goal_primary_connect_peer_profiles,

    /*! Goal is to disconnect peer BREDR profiles to Secondary. */
    tws_topology_goal_primary_disconnect_peer_profiles,

    /*! Goal is to enable page scan on Primary for Secondary to connect BREDR ACL. */
    tws_topology_goal_primary_connectable_peer,

    /*! Goal is to discard current role, disconnect links and cancel any
        activity. Typically used going into the case. */
    tws_topology_goal_no_role_idle,

    /*! Goal is to set a specific role. */
    tws_topology_goal_set_role,

    /*! Goal is to connect to a handset. */
    tws_topology_goal_connect_handset,

    /*! Goal is to disconnect from a handset. */
    tws_topology_goal_disconnect_handset,

    /*! Goal is to enable pagescan to enable handset connectivity. */
    tws_topology_goal_connectable_handset,

    /*! Goal is to take on the Primary role. */
    tws_topology_goal_become_primary,

    /*! Goal is to take on the Secondary role. */
    tws_topology_goal_become_secondary,

    /*! Goal is to take on the Acting Primary role. */
    tws_topology_goal_become_acting_primary,

    /*! Goal is to set a specific BT address. */
    tws_topology_goal_set_address,

    /*! Goal is to set the primary address and start role selection. */
    tws_topology_goal_set_primary_address_and_find_role,

    /*! Goal is to switch roles to secondary (from primary role and
        potentially when currently connected to a handset). */
    tws_topology_goal_role_switch_to_secondary,

    /*! Goal is to start role selection when already in a secondary role.
        Typically this goal is used in failure cases, such as failure
        to establish peer BREDR link following previous role selection. */
    tws_topology_goal_no_role_find_role,

    /*! Goal is to cancel an active role selection process. */
    tws_topology_goal_cancel_find_role,

    /*! Goal is to start continuous role selection (low duty cycle)
        whilst in the Primary role. Typically used in failure cases
        where the Secondary has been lost, or Earbud is an acting
        Primary. */
    tws_topology_goal_primary_find_role,

    tws_topology_goal_dfu_role,
    tws_topology_goal_dfu_primary,
    tws_topology_goal_dfu_secondary,

    tws_topology_goal_dfu_in_case,

    /*! Goal to disconnect the peer and begin role selection, could
        be used on handset linkloss to determine best Earbud to reconnect. */
    tws_topology_goal_disconnect_peer_find_role,

    /*! Goal to release the peer link.
        This is needed as we otherwise keep a lock on the peer link
        until profiles are started (which won't now happen) */
    tws_topology_goal_release_peer,

    /*! Goal for Secondary to participate in a static handover to
        Primary role. */
    tws_topology_goal_secondary_static_handover,

    /*! Goal for Primary to participate in a static handover when going in the case */ 
    tws_topology_goal_primary_static_handover_in_case,

    /*! Goal for Primary to participate in a static handover staying out of the case */
    tws_topology_goal_primary_static_handover,

    /*! Goal to handle the handover recommendation from HDMA  */
    tws_topology_goal_dynamic_handover,
    
    /*! Goal to handle dynamic handover failure */
    tws_topology_goal_dynamic_handover_failure,

    /*! Goal is to enable advertising to enable LE connectivity with handset. */
    tws_topology_goal_le_connectable_handset,
	
    /*! Goal to allow or disallow handset connections */
    tws_topology_goal_allow_handset_connect,

    /* ADD ENTRIES ABOVE HERE */
    /*! Final entry to get the number of IDs */
    TWS_TOPOLOGY_GOAL_NUMBER_IDS,
} tws_topology_goal_id;


/*! \brief Query if a goal is currently active.

    \param goal The goal being queried for status.

    \return TRUE if goal is currently active.
*/
bool TwsTopology_IsGoalActive(tws_topology_goal_id goal);

/*! \brief Check if there are any pending goals.

    \return TRUE if there are one or more goals queued; FALSE otherwise.
*/
bool TwsTopology_IsAnyGoalPending(void);

/*! \brief Handler for new and queued goals.
    
    \note This is a common handler for new goals generated by a topology
          rule decision and goals queued waiting on completion or cancelling
          already in-progress goals. 
*/
void TwsTopology_HandleGoalDecision(Task task, MessageId id, Message message);

/*! \brief Initialise the tws topology goals */
void TwsTopology_GoalsInit(void);

#endif /* TWS_TOPOLOGY_GOALS_H */
