/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of events which can initiate headset topology rule processing.
*/

#ifndef HEADSET_TOPOLOGY_RULE_EVENTS_H_
#define HEADSET_TOPOLOGY_RULE_EVENTS_H_

#define HSTOP_RULE_EVENT_START                       (1ULL << 0)
#define HSTOP_RULE_EVENT_HANDSET_LINKLOSS            (1ULL << 1)

#define HSTOP_RULE_EVENT_HANDSET_DISCONNECTED_BREDR  (1ULL << 2)
#define HSTOP_RULE_EVENT_HANDSET_CONNECTED_BREDR     (1ULL << 3)

#define HSTOP_RULE_EVENT_PROHIBIT_CONNECT_TO_HANDSET (1ULL << 4)

#endif /* HEADSET_TOPOLOGY_RULE_EVENTS_H_ */
