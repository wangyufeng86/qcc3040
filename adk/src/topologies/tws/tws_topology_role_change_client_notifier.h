/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      TWS Topology Module for registering, un-registering and notifying clients via 
            interface functions.
*/

#ifndef TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_NOTIFIER_H
#define TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_NOTIFIER_H

#include "tws_topology.h"
#include "tws_topology_role_change_client_if.h"
#include "domain_message.h"

/* states of role change proposal/indication process */
typedef enum 
{
    TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED,
    TWS_CLIENT_NOTIFIER_FORCE_IN_PROGRESS,
    TWS_CLIENT_NOTIFIER_VOTING_STARTED,  
    TWS_CLIENT_NOTIFIER_PROPOSAL_REJECTED,
    TWS_CLIENT_NOTIFIER_PREPARATION_IN_PROGRESS,
    TWS_CLIENT_NOTIFIER_READY_FOR_ROLE_CHANGE
} role_change_proposal_state_t;

#define TWS_RC_MAX_CLIENTS_BITS     (3)
#define TWS_RC_MAX_CLIENTS          ((0x1 << TWS_RC_MAX_CLIENTS_BITS)-1) 

/*! role change client internal state */
typedef struct
{    
    /*! Pointer to an array of registration interface structures. */
    const role_change_client_callback_t *registrations;

    /*! The number of registrations in the array. */
    unsigned num_registered_role_change_clients:TWS_RC_MAX_CLIENTS_BITS;
    
    /*! ref count of clients which have accepted/prepared for role change*/
    unsigned role_change_client_ref_count:TWS_RC_MAX_CLIENTS_BITS;
     
    /*! holds the state of role change proposal*/
    role_change_proposal_state_t role_change_client_proposal_state:3;
    
    /*! required to differentiate between cancel and role change indication state*/
    bool cancelIndicationFlag:1; 
    
    /*! temp holds the last role indication desemination to registered clients is performed */
    tws_topology_role lastRoleIndication:3; 
    
    /*! used to record the task of the procedure that shall recieve the CLIENTS_PREPARED/CLIENT_REJECTION messages */
    Task tws_procedure_task;
    
    TaskData tws_role_change_client_notifier_task;
} tws_topology_role_change_client_notifier_t;

/*! \brief Initialises the Role Change Client Notifier module
 *  \param registrations Pointer to an array of registration structures.
    \param registrations_count The number of registrations in the array.
*/
void TwsTopology_RoleChangeClientNotifierInit(const role_change_client_callback_t *registrations,
                                     unsigned registrations_count);

/*! \brief returns the current state of the Role Change Proposal
*/
role_change_proposal_state_t TwsTopology_GetClientRoleChangeState(void);

/*! \brief Returns to number of currently registered role change notification clients
*/
unsigned TwsTopology_GetNumberRegisteredRoleChangeClients(void);

/*! \brief Increments tally of Role Change clients that have accepted, 
 * calls for preparation notification when acceptance is unanimous.
*/
void TwsTopology_ClientRoleChangeAcceptanceCfm(const TWS_ROLE_CHANGE_ACCEPTANCE_CFM_T *ind);

/*! \brief Increments tally of prepared Role Change clients, 
 * calls for execution of Role Change if all clients have responded.
*/
void TwsTopology_ClientRoleChangePrepartionCfm(void);

/*! \brief Calls the RoleChangeIndication callback for all registered client interfaces
 *         with the updated tws_topology_role value
*/
void TwsTopology_NotifyListenersOfRoleChangeInd(const TWS_TOPOLOGY_ROLE_CHANGED_IND_T* ind);

/*! \brief Calls the ProposeRoleChange function callback for all registered client interfaces
*/
void TwsTopology_NotifyListenersOfRoleChangeProposal(Task procedure_task);
/*! \brief Calls the ForceRoleChange callback for all registered client interfaces
*/
void TwsTopology_NotifyListenersOfForcedRoleChange(Task procedure_task);
/*! \brief Calls the CancelRoleChange callback for all registered client interfaces
*/
void TwsTopology_NotifyListenersOfRoleChangeCancellation(void);
    

#endif /* TWS_TOPOLOGY_ROLE_CHANGE_CLIENT_NOTIFIER_H */
