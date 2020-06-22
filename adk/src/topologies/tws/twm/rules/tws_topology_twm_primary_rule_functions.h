/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWM feature specific rule functions.
*/

#ifndef _TWS_TOPOLOGY_TWM_PRIMARY_RULE_FUNCTIONS_H_
#define _TWS_TOPOLOGY_TWM_PRIMARY_RULE_FUNCTIONS_H_

#include <rules_engine.h>

/*! \{
    Rule function prototypes, so that they're available to external rules table. */
DEFINE_RULE_FUNCTION(ruleTwsTopTwmPriFindRole);
DEFINE_RULE_FUNCTION(ruleTwsTopTwmPriNoRoleIdle);
DEFINE_RULE_FUNCTION(ruleTwsTopTwmHandoverStart);
DEFINE_RULE_FUNCTION(ruleTwsTopTwmHandoverFailed);
DEFINE_RULE_FUNCTION(ruleTwsTopTwmHandoverFailureHandled);
/*! \} */

#endif /* _TWS_TOPOLOGY_TWM_PRIMARY_RULE_FUNCTIONS_H_ */
