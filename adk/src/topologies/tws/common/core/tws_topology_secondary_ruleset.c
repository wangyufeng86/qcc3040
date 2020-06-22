/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_secondary_ruleset.h"
#include "tws_topology_secondary_rules_table.h"

#include <logging.h>

/*! Get pointer to the connection rules task data structure. */
#define TwsTopologySecondaryRulesGetTaskData()  (&tws_topology_secondary_rules_task_data)

TwsTopologySecondaryRulesTaskData tws_topology_secondary_rules_task_data;

bool TwsTopologySecondaryRules_Init(Task result_task)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = tws_topology_secondary_rules;
    rule_params.rules_count = tws_topology_secondary_rules_count;
    rule_params.nop_message_id = TWSTOP_SECONDARY_GOAL_NOP;
    rule_params.event_task = result_task;
    secondary_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);

    return TRUE;
}

rule_set_t TwsTopologySecondaryRules_GetRuleSet(void)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    return secondary_rules->rule_set;
}

void TwsTopologySecondaryRules_SetEvent(rule_events_t event_mask)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    RulesEngine_SetEvent(secondary_rules->rule_set, event_mask);
}

void TwsTopologySecondaryRules_ResetEvent(rule_events_t event)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    RulesEngine_ResetEvent(secondary_rules->rule_set, event);
}

rule_events_t TwsTopologySecondaryRules_GetEvents(void)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    return RulesEngine_GetEvents(secondary_rules->rule_set);
}

void TwsTopologySecondaryRules_SetRuleComplete(MessageId message)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    RulesEngine_SetRuleComplete(secondary_rules->rule_set, message);
}

void TwsTopologySecondaryRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(secondary_rules->rule_set, message, event);
}

rule_action_t twsTopologySecondaryRules_CopyRunParams(const void* param, size_t size_param)
{
    TwsTopologySecondaryRulesTaskData *secondary_rules = TwsTopologySecondaryRulesGetTaskData();
    RulesEngine_CopyRunParams(secondary_rules->rule_set, param, size_param);
    return rule_action_run_with_param;
}
