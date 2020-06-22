/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       link_policy_config.h
\brief      Configuration related definitions for the link policy manager.
*/

#ifndef LINK_POLICY_CONFIG_H_
#define LINK_POLICY_CONFIG_H_


/*! Link supervision timeout for ACL between Earbuds (in milliseconds) */
#define appConfigEarbudLinkSupervisionTimeout()  (2000)

/*! Default link supervision timeout for other ACLs (in milliseconds) */
#define appConfigDefaultLinkSupervisionTimeout()  (5000)

#endif /* LINK_POLICY_CONFIG_H_ */
