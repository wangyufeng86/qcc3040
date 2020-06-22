/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_common_rules.c
\brief	    Earbud application rules run when in any earbud role.
*/

#include "earbud_common_rules.h"
#include "earbud_rules_config.h"

#include <earbud_config.h>
#include <earbud_init.h>
#include <adk_log.h>
#include <earbud_sm.h>

#include <domain_message.h>
#include <phy_state.h>
#include <rules_engine.h>

#include <panic.h>

#pragma unitsuppress Unused

/*! \{
    Macros for diagnostic output that can be suppressed.*/
#define COMMON_RULE_LOG(...)         DEBUG_LOG_DEBUG(__VA_ARGS__)
/*! \} */

/* Forward declaration for use in RULE_ACTION_RUN_PARAM macro below */
static rule_action_t CommonRulesCopyRunParams(const void* param, size_t size_param);

/*! \brief Macro used by rules to return RUN action with parameters to return.
    Copies the parameters/data into Common_rules where the rules engine can uses
    it when building the action message.
*/
#define RULE_ACTION_RUN_PARAM(x)   CommonRulesCopyRunParams(&(x), sizeof(x))

/*! Get pointer to the connection rules task data structure. */
#define CommonRulesGetTaskData()           (&Common_rules_task_data)

/*!< Connection rules. */
CommonRulesTaskData Common_rules_task_data;

/*****************************************************************************
 * All rules contained in this file MUST be applicable whatever the Topology
 * Role of the Earbuds. Therefore only rules which do not bear on Bluetooth or
 * relate to Bluetooth functionality should be entered here.
 *
 * An example of some rules which are applicable whatever the Topology Role
 * happens to be are the rules for setting the LEDs enabled or disabled when
 * In or Out-Of-Ear.
 *****************************************************************************/

/*! \{
    Rule function prototypes, so we can build the rule tables below. */
DEFINE_RULE(ruleOutOfEarLedsEnable);
DEFINE_RULE(ruleInEarLedsDisable);

/*! \} */

/*! \brief Set of rules to run on Earbud startup. */
const rule_entry_t Common_rules_set[] =
{
    /*! \{
        Rules to LED on/off when in/out of the ear. */
    RULE(RULE_EVENT_OUT_EAR,                    ruleOutOfEarLedsEnable,             CONN_RULES_LED_ENABLE),
    RULE(RULE_EVENT_IN_EAR,                     ruleInEarLedsDisable,               CONN_RULES_LED_DISABLE),
    /*! \} */
};

/*****************************************************************************
 * RULES FUNCTIONS
 *****************************************************************************/
/*! @brief Rule to determine if LED should be enabled when out of ear
    Rule is triggered by the 'out of ear' event
    @startuml

    start
    if (Not IsLedsInEarEnabled()) then (yes)
        :Run rule, as out of ear and LEDs were disabled in ear;
        stop
    endif
    end
    @enduml
*/
static rule_action_t ruleOutOfEarLedsEnable(void)
{
    if (!appConfigInEarLedsEnabled())
    {
        COMMON_RULE_LOG("ruleOutOfEarLedsEnable, run as out of ear");
        return rule_action_run;
    }
    else
    {
        COMMON_RULE_LOG("ruleOutOfEarLedsEnable, ignore as out of ear but in ear LEDs enabled");
        return rule_action_ignore;
    }
}

/*! @brief Rule to determine if LED should be disabled when in ear
    Rule is triggered by the 'in ear' event
    @startuml

    start
    if (Not IsLedsInEarEnabled()) then (yes)
        :Run rule, as in ear and LEDs are disabled in ear;
        stop
    endif
    end
    @enduml
*/
static rule_action_t ruleInEarLedsDisable(void)
{
    if (!appConfigInEarLedsEnabled())
    {
        COMMON_RULE_LOG("ruleInEarLedsDisable, run as in ear");
        return rule_action_run;
    }
    else
    {
        COMMON_RULE_LOG("ruleInEarLedsDisable, ignore as in ear but in ear LEDs enabled");
        return rule_action_ignore;
    }
}

/*****************************************************************************
 * END RULES FUNCTIONS
 *****************************************************************************/

/*! \brief Initialise the Common rules module. */
bool CommonRules_Init(Task init_task)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    rule_set_init_params_t rule_params;

    UNUSED(init_task);

    memset(&rule_params, 0, sizeof(rule_params));
    rule_params.rules = Common_rules_set;
    rule_params.rules_count = ARRAY_DIM(Common_rules_set);
    rule_params.nop_message_id = CONN_RULES_NOP;
    rule_params.event_task = SmGetTask();
    Common_rules->rule_set = RulesEngine_CreateRuleSet(&rule_params);

    return TRUE;
}

rule_set_t CommonRules_GetRuleSet(void)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    return Common_rules->rule_set;
}

void CommonRules_SetEvent(rule_events_t event_mask)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    RulesEngine_SetEvent(Common_rules->rule_set, event_mask);
}

void CommonRules_ResetEvent(rule_events_t event)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    RulesEngine_ResetEvent(Common_rules->rule_set, event);
}

rule_events_t CommonRules_GetEvents(void)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    return RulesEngine_GetEvents(Common_rules->rule_set);
}

void CommonRules_SetRuleComplete(MessageId message)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    RulesEngine_SetRuleComplete(Common_rules->rule_set, message);
}

void CommonRules_SetRuleWithEventComplete(MessageId message, rule_events_t event)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    RulesEngine_SetRuleWithEventComplete(Common_rules->rule_set, message, event);
}

/*! \brief Copy rule param data for the engine to put into action messages.
    \param param Pointer to data to copy.
    \param size_param Size of the data in bytes.
    \return rule_action_run_with_param to indicate the rule action message needs parameters.
 */
static rule_action_t CommonRulesCopyRunParams(const void* param, size_t size_param)
{
    CommonRulesTaskData *Common_rules = CommonRulesGetTaskData();
    RulesEngine_CopyRunParams(Common_rules->rule_set, param, size_param);
    return rule_action_run_with_param;
}
