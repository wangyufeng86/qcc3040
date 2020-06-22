/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Definition of the rules to be used in firmware upgrade mode (DFU).

            The aim of the rule set is to support the upgrade library during DFU,
            by allowing connections to the peer and handset as required.
*/

#ifndef _TWS_TOPOLOGY_DFU_RULES_H_
#define _TWS_TOPOLOGY_DFU_RULES_H_

#include "tws_topology_private.h"
#include "tws_topology_rule_events.h"

#include <rules_engine.h>

#include <message.h>

/*! List of goals that can be the outcome of a rule for the DFU
    rule set */
enum tws_topology_dfu_rules_goals
{
        /*! Exit the DFU mode, and attempt to find a role */
    TWSTOP_DFU_GOAL_NO_ROLE_FIND_ROLE = TWSTOP_INTERNAL_DFU_RULE_MSG_BASE,

        /*! Exit the DFU mode, and return to normal rules (no role) */
    TWSTOP_DFU_GOAL_NO_ROLE_IDLE,

        /*! Switch to secondary address, and enable required activities */
    TWSTOP_DFU_GOAL_SECONDARY,

        /*! Switch to primary address, and enable required activities */
    TWSTOP_DFU_GOAL_PRIMARY,

        /*! Reconnect with primary, and enable required activities */
    TWSTOP_DFU_GOAL_LINKLOSS_SECONDARY,

    /*! Perform few operations when entering dfu in case mode */
    TWSTOP_DFU_GOAL_IN_CASE,

    TWSTOP_DFU_GOAL_CONNECTABLE_HANDSET,

        /*! Allow handset BR/EDR connections in the connection manager. */
    TWSTOP_DFU_GOAL_ALLOW_HANDSET_CONNECT,

    /** Disconnect handset (BR/EDR) profiles and links (BR/EDR and/or LE) when
     * DFU transport is BLE.
     * This is required to synchronize rules for handling abort scenarios when
     * DFU transport is BLE.
     */
    TWSTOP_DFU_GOAL_LE_PRI_ABORT_CLEANUP,

    /*!
     * This goal is activated on DFU completion to turn off the page scan
     * started on Secondary to have fixed main roles (of Primary and Secondary).
     * This is goal is especially necessary when Secondary earbud is out-of-case
     * after DFU completion whereas when Secondary earbud is in-case the
     * page scan is implicitly turned off as part "No Role Idle" goal activation
     * on DFU completion. In later scenario, disable page scan is redundantly
     * done.
     */
    TWSTOP_DFU_GOAL_SEC_DISABLE_PAGE_SCAN_TO_PEER,

    /*!
     * This goal is activated when the Primary get disconnected from Secondary
     * Device after an abrupt reset during Background DFU. This goal is 
     * connect the peer profiles again after reset
     */
    TWSTOP_DFU_GOAL_CONNECT_PEER_PROFILES,

    TWSTOP_DFU_GOAL_CONNECT_HANDSET_ON_RESET,

        /*! Value used when there is no DFU goal */
    TWSTOP_DFU_GOAL_NOP,
};

/*! TWS Topology dfu rules task data. */
typedef struct
{
        /*! The rule set associated with the DFU rules */
    rule_set_t rule_set;
} TwsTopologyDfuRulesTaskData;

/*! \brief Initialise the TWS Topology dfu rules component. 

    \param[in] result_task  Task to send rule results to
 */
void TwsTopologyDfuRules_Init(Task result_task);

/*! \brief Get handle on the dfu rule set, in order to directly set/clear events.
    \return rule_set_t The dfu rule set.
 */
rule_set_t TwsTopologyDfuRules_GetRuleSet(void);

/*! \brief Set an event or events

    \param[in] event_mask  Events to set that will trigger rules to run

    This function is called to set an event or events affecting the DFU rules.
    If the event is supported by the DFU rule set then the matching
    rules in the rules table will run.  Any actions generated will be sent as 
    messages to the client_task
*/
void TwsTopologyDfuRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events

    \param[in] event Events to clear

    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void TwsTopologyDfuRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t TwsTopologyDfuRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID

    \param[in] message Message ID that rule(s) generated

    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void TwsTopologyDfuRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events

    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)

    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void TwsTopologyDfuRules_SetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* _TWS_TOPOLOGY_DFU_RULES_H_ */
