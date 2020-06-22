/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWS Classic feature specific rule functions.
*/

#ifndef _TWS_TOPOLOGY_TWS_CLASSIC_SECONDARY_RULE_FUNCTIONS_H_
#define _TWS_TOPOLOGY_TWS_CLASSIC_SECONDARY_RULE_FUNCTIONS_H_

#include <rules_engine.h>

/*! \name Rule function prototypes

    These allow us to build the rule table twstop_secondary_rules_set. */
/*! @{ */
DEFINE_RULE_FUNCTION(ruleTwsTopClassicSecPeerLostFindRole);
DEFINE_RULE_FUNCTION(ruleTwsTopSecStaticHandoverCommand);
DEFINE_RULE_FUNCTION(ruleTwsTopSecStaticHandoverFailedOutCase);
/*! @} */

#endif /* _TWS_TOPOLOGY_TWS_CLASSIC_SECONDARY_RULE_FUNCTIONS_H_ */
