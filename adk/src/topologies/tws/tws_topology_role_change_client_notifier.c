/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      TWS Topology Module for notifying clients via interface function calls. 
            Includes a state machine to control/monitor the role change request proposals and reponses,
            force role change and cancellation mechanisms.
*/

#include "tws_topology.h"
#include "tws_topology_private.h"
#include "tws_topology_config.h"
#include "tws_topology_role_change_client_notifier.h"

#include <logging.h>
#include <panic.h>
#include <message.h>

static void twsTopology_RoleChangeClientNotifierMessageHandler(Task task, MessageId id, Message message);
static void twsTopology_RoleChangeClientNotifierCancelPendingMessageAndResetRefCounters(void);
static void twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(role_change_proposal_state_t state);

static tws_topology_role_change_client_notifier_t notifier_state;

#define TwsTopRoleChangeClientNotifierGetTaskData() (&notifier_state)

#define TwsTopRoleChangeClientNotifierGetTask() (&notifier_state.tws_role_change_client_notifier_task)

static const uint32_t min_reconnection_delay_dynamic_handover = 500;
static const uint32_t min_reconnection_delay = 2000;

#define reconnection_delay() (TwsTopologyConfig_DynamicHandoverSupported() ?  min_reconnection_delay_dynamic_handover : min_reconnection_delay)

#define FOR_EACH_ROLE_CHANGE_CLIENT for (registration = notifier_state.registrations; \
registration < (notifier_state.registrations + notifier_state.num_registered_role_change_clients); \
registration++)

static void twsTopology_RoleChangeClientNotifierInitialiseClients(Task response_task);

/* state entry calls */
static void twsTopology_RoleChangeClientNotifierEnterNothingProposed(void);
static void twsTopology_RoleChangeClientNotifierEnterForceInProgress(void);
static void twsTopology_RoleChangeClientNotifierEnterVotingStarted(void);
static void twsTopology_RoleChangeClientNotifierEnterProposalRejected(void);
static void twsTopology_RoleChangeClientNotifierEnterPreparationInProgress(void);
static void twsTopology_RoleChangeClientNotifierEnterReadyForRoleChange(void);

void TwsTopology_RoleChangeClientNotifierInit(const role_change_client_callback_t *registrations,
                                     unsigned registrations_count)
{
    PanicFalse(registrations_count <= TWS_RC_MAX_CLIENTS);
    
    tws_topology_role_change_client_notifier_t * td = TwsTopRoleChangeClientNotifierGetTaskData(); 
    
    td->registrations = registrations;
    td->num_registered_role_change_clients = registrations_count;
    td->role_change_client_ref_count = 0;
    td->cancelIndicationFlag = FALSE;
    td->role_change_client_proposal_state = TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED;
    td->lastRoleIndication = tws_topology_role_none;
    td->tws_procedure_task = NULL;
    td->tws_role_change_client_notifier_task.handler = twsTopology_RoleChangeClientNotifierMessageHandler;
    
    TwsTopology_RegisterMessageClient(TwsTopRoleChangeClientNotifierGetTask());
    twsTopology_RoleChangeClientNotifierInitialiseClients(TwsTopRoleChangeClientNotifierGetTask());
}

unsigned TwsTopology_GetNumberRegisteredRoleChangeClients(void)
{
    return notifier_state.num_registered_role_change_clients;
}

static void twsTopology_RoleChangeClientNotifierMessageHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    switch(id)
    {
        case TWS_TOPOLOGY_ROLE_CHANGED_IND:
            TwsTopology_NotifyListenersOfRoleChangeInd((TWS_TOPOLOGY_ROLE_CHANGED_IND_T*)message);
            break;
        case TWS_ROLE_CHANGE_ACCEPTANCE_CFM:
            TwsTopology_ClientRoleChangeAcceptanceCfm((TWS_ROLE_CHANGE_ACCEPTANCE_CFM_T *)message);
            break;
        case TWS_ROLE_CHANGE_PREPARATION_CFM:
            TwsTopology_ClientRoleChangePrepartionCfm();
            break;
        case TWS_TOPOLOGY_START_CFM:
        case TWS_TOPOLOGY_HANDSET_DISCONNECTED_IND:
            /* Nothing to do here.  
             * We are only interested in Role Change Ind from the main topology module*/
            break;
        default:
            DEBUG_LOG("twsTopology_RoleChangeClientNotifierMessageHandler: Unhandled Message(%u)",id);
            break;
    }
}

void TwsTopology_ClientRoleChangeAcceptanceCfm(const TWS_ROLE_CHANGE_ACCEPTANCE_CFM_T *Cfm)
{
    tws_topology_role_change_client_notifier_t * td = TwsTopRoleChangeClientNotifierGetTaskData(); 
    if(TwsTopology_GetClientRoleChangeState() == TWS_CLIENT_NOTIFIER_VOTING_STARTED)
    {
        if(Cfm->role_change_accepted) 
        {
            td->role_change_client_ref_count++;
            if(td->role_change_client_ref_count == TwsTopology_GetNumberRegisteredRoleChangeClients())
            {
                td->role_change_client_ref_count = 0; /* reset counter for preparation counting */
                DEBUG_LOG("TwsTopology_ClientRoleChangeAcceptanceCfm: All Role Change clients have 'Accepted' proposal");
                twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_PREPARATION_IN_PROGRESS);
            }
        }
        else
        {
            DEBUG_LOG("twsTopology TwsTopology_ClientRoleChangeAcceptanceCfm: Role Change NOT ACCEPTED by client");
            if(TwsTopology_GetClientRoleChangeState() != TWS_CLIENT_NOTIFIER_PROPOSAL_REJECTED) /* only one client needs to veto */
            {
                twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_PROPOSAL_REJECTED);
            }
        }
    }
    else
    {
        DEBUG_LOG("twsTopology TwsTopology_ClientRoleChangeAcceptanceCfm UNEXPECTED");
    }
}

void TwsTopology_ClientRoleChangePrepartionCfm(void)
{   
    tws_topology_role_change_client_notifier_t * td = TwsTopRoleChangeClientNotifierGetTaskData(); 
    if(TwsTopology_GetClientRoleChangeState() == TWS_CLIENT_NOTIFIER_PREPARATION_IN_PROGRESS || 
       TwsTopology_GetClientRoleChangeState() == TWS_CLIENT_NOTIFIER_FORCE_IN_PROGRESS)
    {
        td->role_change_client_ref_count++;
        if(td->role_change_client_ref_count == TwsTopology_GetNumberRegisteredRoleChangeClients())
        {
            DEBUG_LOG("TwsTopology_ClientRoleChangePrepartionCfm: All Role Change clients are prepared for RC");
            twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_READY_FOR_ROLE_CHANGE);
        }
    }
    else
    {
        DEBUG_LOG("twsTopology TwsTopology_ClientRoleChangePrepartionCfm UNEXPECTED");
    }
}

void TwsTopology_NotifyListenersOfRoleChangeInd(const TWS_TOPOLOGY_ROLE_CHANGED_IND_T *ind)
{
    notifier_state.lastRoleIndication = ind->role;
    twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED);
}

void TwsTopology_NotifyListenersOfRoleChangeProposal(Task procedure_task)
{   
    notifier_state.tws_procedure_task = procedure_task;
    if(TwsTopology_GetNumberRegisteredRoleChangeClients() > 0)
    {
        twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_VOTING_STARTED);
    }
    else
    {
        twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_READY_FOR_ROLE_CHANGE);
    }
}

void TwsTopology_NotifyListenersOfForcedRoleChange(Task procedure_task)
{
    notifier_state.tws_procedure_task = procedure_task;
    if(TwsTopology_GetNumberRegisteredRoleChangeClients() > 0)
    {
        twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_FORCE_IN_PROGRESS);
    }
    else /* No clients registered, report back all clients prepared and set to idle state */
    {
        DEBUG_LOG("TwsTopology_NotifyListenersOfForcedRoleChange called with no registered clients");
        MessageSend(PanicNull(notifier_state.tws_procedure_task), TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED , NULL);
        twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED);
    }
}

void TwsTopology_NotifyListenersOfRoleChangeCancellation(void)
{
    notifier_state.cancelIndicationFlag = TRUE; 
    twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED);
}

/*! \brief Update the FSM as appropriate for the old and new state transision.
*/
static void twsTopology_RoleChangeClientNotifierUpdateRoleChangeState(role_change_proposal_state_t new_state)
{
    tws_topology_role_change_client_notifier_t * td = TwsTopRoleChangeClientNotifierGetTaskData(); 
    role_change_proposal_state_t old_state = TwsTopology_GetClientRoleChangeState();
    
    if (old_state == new_state )
    {
        if(old_state != TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED)
        {    
            DEBUG_LOG("twsTopology_RoleChangeClientNotifierUpdateRoleChangeState: Unexpected transition to same state:%d",old_state);
            Panic();    
            return;
        }
    }

    DEBUG_LOG("twsTopology_RoleChangeClientNotifierUpdateRoleChangeState %d->%d",old_state, new_state);
    
    /* Pattern is to run functions for exiting state first */
    switch (old_state)
    {
        case TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED:
            break;  
        case TWS_CLIENT_NOTIFIER_FORCE_IN_PROGRESS:
            break;     
        case TWS_CLIENT_NOTIFIER_VOTING_STARTED:
            break;
        case TWS_CLIENT_NOTIFIER_PROPOSAL_REJECTED:
            break;
        case TWS_CLIENT_NOTIFIER_PREPARATION_IN_PROGRESS:
            break;
        case TWS_CLIENT_NOTIFIER_READY_FOR_ROLE_CHANGE:
            break;   
    }

    td->role_change_client_proposal_state = new_state;
    
    switch (td->role_change_client_proposal_state)
    {
        case TWS_CLIENT_NOTIFIER_NOTHING_PROPOSED:
            twsTopology_RoleChangeClientNotifierEnterNothingProposed();
            break;
        case TWS_CLIENT_NOTIFIER_FORCE_IN_PROGRESS: 
            twsTopology_RoleChangeClientNotifierEnterForceInProgress();
            break;
        case TWS_CLIENT_NOTIFIER_VOTING_STARTED:
            twsTopology_RoleChangeClientNotifierEnterVotingStarted();
            break;
        case TWS_CLIENT_NOTIFIER_PROPOSAL_REJECTED:
            twsTopology_RoleChangeClientNotifierEnterProposalRejected();
            break;
        case TWS_CLIENT_NOTIFIER_PREPARATION_IN_PROGRESS:
            twsTopology_RoleChangeClientNotifierEnterPreparationInProgress();
            break;
        case TWS_CLIENT_NOTIFIER_READY_FOR_ROLE_CHANGE:
            twsTopology_RoleChangeClientNotifierEnterReadyForRoleChange();
            break;   
    }
}

/*! \brief Depending on cancelIndicationFlag, either calls the CancelRoleChange 
 * or RoleChangeIndication callback for each registered client 
*/
static void twsTopology_RoleChangeClientNotifierEnterNothingProposed(void)
{
    tws_topology_role_change_client_notifier_t * td = TwsTopRoleChangeClientNotifierGetTaskData(); 
    twsTopology_RoleChangeClientNotifierCancelPendingMessageAndResetRefCounters();
    if(td->cancelIndicationFlag)
    {   
        const role_change_client_callback_t *registration;
        FOR_EACH_ROLE_CHANGE_CLIENT
        {
            registration->CancelRoleChange();
        }       
        td->cancelIndicationFlag = FALSE;
    }
    else
    {
        const role_change_client_callback_t *registration;
        FOR_EACH_ROLE_CHANGE_CLIENT
        {
            registration->RoleChangeIndication(td->lastRoleIndication);
        } 
    }
}

/*! \brief Calls the ForceRoleChange callback for each registered client
*/
static void twsTopology_RoleChangeClientNotifierEnterForceInProgress(void)
{
    const role_change_client_callback_t *registration;
    FOR_EACH_ROLE_CHANGE_CLIENT
    {
        registration->ForceRoleChange();
    }       
}

/*! \brief Calls the ProposeRoleChange callback for each registered client
*/
static void twsTopology_RoleChangeClientNotifierEnterVotingStarted(void)
{
    const role_change_client_callback_t *registration;
    FOR_EACH_ROLE_CHANGE_CLIENT
    {
        registration->ProposeRoleChange();
    }       
}

/*! \brief Sends the TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION message to the topology server
 *
*/
static void twsTopology_RoleChangeClientNotifierEnterProposalRejected(void)
{
   MessageSend(PanicNull(notifier_state.tws_procedure_task), TWSTOP_INTERNAL_ROLE_CHANGE_CLIENT_REJECTION ,NULL); 
}

/*! \brief Calls the PrepareRoleChange callback for each registered client
*/
static void twsTopology_RoleChangeClientNotifierEnterPreparationInProgress(void)
{
    const role_change_client_callback_t *registration;
    FOR_EACH_ROLE_CHANGE_CLIENT
    {
        registration->PrepareRoleChange();
    } 
}

/*! \brief Sends the TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED message to the topology server
 *
*/
static void twsTopology_RoleChangeClientNotifierEnterReadyForRoleChange(void)
{
    MessageSend(PanicNull(notifier_state.tws_procedure_task), TWSTOP_INTERNAL_ALL_ROLE_CHANGE_CLIENTS_PREPARED , NULL);
}

/*! \brief resets the acceptance/preperation reference counters
 *  and cancels any queued Acceptance/Prepartion CFM messages
*/
static void twsTopology_RoleChangeClientNotifierCancelPendingMessageAndResetRefCounters(void)
{
    notifier_state.role_change_client_ref_count = 0;
    MessageCancelAll(TwsTopRoleChangeClientNotifierGetTask(),TWS_ROLE_CHANGE_ACCEPTANCE_CFM);
    MessageCancelAll(TwsTopRoleChangeClientNotifierGetTask(),TWS_ROLE_CHANGE_PREPARATION_CFM);
}

/*! \brief gets the current state
*/
role_change_proposal_state_t TwsTopology_GetClientRoleChangeState(void)
{
    return notifier_state.role_change_client_proposal_state;  
}

/*! \brief invokes the Initialise callback for all registered clients
*/
static void twsTopology_RoleChangeClientNotifierInitialiseClients(Task response_task)
{
    const role_change_client_callback_t *registration;
    FOR_EACH_ROLE_CHANGE_CLIENT
    {
        registration->Initialise(response_task, reconnection_delay());
    } 
}






