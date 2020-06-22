/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to common primary role TWS topology rule functions.
*/

#ifndef _TWS_TOPOLOGY_COMMON_PRIMARY_RULE_FUNCTIONS_H_
#define _TWS_TOPOLOGY_COMMON_PRIMARY_RULE_FUNCTIONS_H_

#include <rules_engine.h>

/*! \{
    Rule function prototypes, so that they're available to external rules table. */
DEFINE_RULE_FUNCTION(ruleTwsTopPriPairPeer);
DEFINE_RULE_FUNCTION(ruleTwsTopPriPeerPairedInCase);
DEFINE_RULE_FUNCTION(ruleTwsTopPriPeerPairedOutCase);

DEFINE_RULE_FUNCTION(ruleTwsTopPriConnectPeerProfiles);
DEFINE_RULE_FUNCTION(ruleTwsTopPriEnableConnectablePeer);
DEFINE_RULE_FUNCTION(ruleTwsTopPriDisableConnectablePeer);
DEFINE_RULE_FUNCTION(ruleTwsTopPriReleasePeer);

DEFINE_RULE_FUNCTION(ruleTwsTopPriSelectedPrimary);
DEFINE_RULE_FUNCTION(ruleTwsTopPriSelectedActingPrimary);
DEFINE_RULE_FUNCTION(ruleTwsTopPriNoRoleSelectedSecondary);
DEFINE_RULE_FUNCTION(ruleTwsTopPriPrimarySelectedSecondary);
DEFINE_RULE_FUNCTION(ruleTwsTopPriPeerLostFindRole);
DEFINE_RULE_FUNCTION(ruleTwsTopPriHandsetLinkLossReconnect);

DEFINE_RULE_FUNCTION(ruleTwsTopPriEnableConnectableHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopPriDisableConnectableHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopPriRoleSwitchConnectHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopPriOutCaseConnectHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopPriInCaseDisconnectHandset);
DEFINE_RULE_FUNCTION(ruleTwsTopPriAllowHandsetConnect);

DEFINE_RULE_FUNCTION(ruleTwsTopPriSwitchToDfuRole);
DEFINE_RULE_FUNCTION(ruleTwsTopPriEnableLeConnectableHandset);

DEFINE_RULE_FUNCTION(ruleTwsTopPriDisconnectHandset);
/*! \} */

#endif /* _TWS_TOPOLOGY_COMMON_PRIMARY_RULE_FUNCTIONS_H_ */
