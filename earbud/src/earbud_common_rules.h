/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_common_rules.h
\brief      Earbud application rules run when in any earbud role.
*/

#ifndef _EARBUD_COMMON_RULES_H_
#define _EARBUD_COMMON_RULES_H_

#include <domain_message.h>
#include <rules_engine.h>
#include <kymera.h>

/*! \brief Connection Rules task data. */
typedef struct
{
    rule_set_t rule_set;
} CommonRulesTaskData;

/*! \brief Initialise the connection rules module. */
bool CommonRules_Init(Task init_task);

/*! \brief Get handle on the Common rule set, in order to directly set/clear events.
    \return rule_set_t The Common rule set.
 */
rule_set_t CommonRules_GetRuleSet(void);

/*! \brief Set an event or events
    \param[in] event Events to set that will trigger rules to run
    This function is called to set an event or events that will cause the relevant
    rules in the rules table to run.  Any actions generated will be sent as message
    to the client_task
*/
void CommonRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events
    \param[in] event Events to clear
    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void CommonRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t CommonRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID
    \param[in] message Message ID that rule(s) generated
    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void CommonRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events
    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)
    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void CommonRules_SetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* _EARBUD_COMMON_RULES_H_ */
