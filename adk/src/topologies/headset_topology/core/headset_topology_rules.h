/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Rules for headset topology
*/

#ifndef _HEADSET_TOPOLOGY_RULES_H_
#define _HEADSET_TOPOLOGY_RULES_H_

#include "headset_topology_rule_events.h"
#include "headset_topology_private.h"

#include <rules_engine.h>

#include <message.h>


enum headset_topology_goals
{
    HSTOP_GOAL_CONNECTABLE_HANDSET = HEADSETTOP_INTERNAL_RULE_MSG_BASE,
    HSTOP_GOAL_CONNECT_HANDSET,
    HSTOP_GOAL_ALLOW_HANDSET_CONNECT,
    HSTOP_GOAL_DISCONNECT_HANDSET,
    HSTOP_GOAL_NOP,
};

typedef struct
{
    uint8 profiles;
} HSTOP_GOAL_DISCONNECT_HANDSET_PROFILES_T;

typedef struct
{
    bool enable;
} HSTOP_GOAL_CONNECTABLE_HANDSET_T;

typedef struct
{
    bool allow;
} HSTOP_GOAL_ALLOW_HANDSET_CONNECT_T;

typedef struct
{
    uint8 profiles;
} HSTOP_GOAL_CONNECT_HANDSET_T;

/*! \brief Headset Topology rules task data. */
typedef struct
{
    rule_set_t rule_set;
} HeadsetTopologyRulesTaskData;

/*! \brief Initialise the connection rules module. */
bool HeadsetTopologyRules_Init(Task init_task);

/*! \brief Get handle on the rule set, in order to directly set/clear events.
    \return rule_set_t The rule set.
 */
rule_set_t HeadsetTopologyRules_GetRuleSet(void);

/*! \brief Set an event or events
    \param[in] event Events to set that will trigger rules to run
    This function is called to set an event or events that will cause the relevant
    rules in the rules table to run.  Any actions generated will be sent as message
    to the client_task
*/
void HeadsetTopologyRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events
    \param[in] event Events to clear
    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void HeadsetTopologyRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t HeadsetTopologyRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID
    \param[in] message Message ID that rule(s) generated
    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void HeadsetTopologyRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events
    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)
    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void HeadsetTopologyRules_SetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* _HEADSET_TOPOLOGY_RULES_H_ */
