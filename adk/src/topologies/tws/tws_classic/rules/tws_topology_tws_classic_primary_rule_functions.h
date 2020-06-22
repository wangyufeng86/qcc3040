/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to TWS Classic feature specific rule functions.
*/

#ifndef _TWS_TOPOLOGY_TWS_CLASSIC_PRIMARY_RULE_FUNCTIONS_H_
#define _TWS_TOPOLOGY_TWS_CLASSIC_PRIMARY_RULE_FUNCTIONS_H_

#include <rules_engine.h>

/*! \{
    Rule function prototypes, so that they're available to external rules table. */
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriFindRole);
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriNoRoleIdle);
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriPeerLostFindRole);
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriHandoverStart);
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriEnableConnectableHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopClassicPriInCaseDisconnectHandset);
/*! \} */

#endif /* _TWS_TOPOLOGY_TWS_CLASSIC_PRIMARY_RULE_FUNCTIONS_H_ */
