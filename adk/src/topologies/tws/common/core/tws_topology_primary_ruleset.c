/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#include "tws_topology_primary_ruleset.h"
#include "tws_topology_primary_rules_table.h"

#include <logging.h>

/*! Get pointer to the connection rules task data structure. */
#define TwsTopologyPrimaryRulesGetTaskData()  (&tws_topology_primary_rules_task_data)

TwsTopologyPrimaryRulesTaskData tws_topology_primary_rules_task_data;

/*! \brief Initialise the primary rules module. */
bool TwsTopologyPrimaryRules_Init(Task result_task)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    rule_set_init_params_t rule_params;

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = tws_topology_primary_rules;
    rule_params.rules_count = tws_topology_primary_rules_count;
    rule_params.nop_message_id = TWSTOP_PRIMARY_GOAL_NOP;
    rule_params.event_task = result_task;
    primary_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);

    return TRUE;
}

rule_set_t TwsTopologyPrimaryRules_GetRuleSet(void)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    return primary_rules->rule_set;
}

void TwsTopologyPrimaryRules_SetEvent(rule_events_t event_mask)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    RulesEngine_SetEvent(primary_rules->rule_set, event_mask);
}

void TwsTopologyPrimaryRules_ResetEvent(rule_events_t event)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    RulesEngine_ResetEvent(primary_rules->rule_set, event);
}

rule_events_t TwsTopologyPrimaryRules_GetEvents(void)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    return RulesEngine_GetEvents(primary_rules->rule_set);
}

void TwsTopologyPrimaryRules_SetRuleComplete(MessageId message)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    RulesEngine_SetRuleComplete(primary_rules->rule_set, message);
}

void TwsTopologyPrimaryRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(primary_rules->rule_set, message, event);
}

rule_action_t twsTopologyPrimaryRules_CopyRunParams(const void* param, size_t size_param)
{
    TwsTopologyPrimaryRulesTaskData *primary_rules = TwsTopologyPrimaryRulesGetTaskData();
    RulesEngine_CopyRunParams(primary_rules->rule_set, param, size_param);
    return rule_action_run_with_param;
}
