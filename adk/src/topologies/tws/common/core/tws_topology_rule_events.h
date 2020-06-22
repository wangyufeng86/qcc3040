/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Definition of events which can initiate TWS topology rule processing.
*/

#ifndef _TWS_TOPOLOGY_RULE_EVENTS_H_
#define _TWS_TOPOLOGY_RULE_EVENTS_H_

#define TWSTOP_RULE_EVENT_STARTUP                       (1ULL << 0)
#define TWSTOP_RULE_EVENT_NO_PEER                       (1ULL << 1)
#define TWSTOP_RULE_EVENT_PEER_PAIRED                   (1ULL << 2)

#define TWSTOP_RULE_EVENT_IN_CASE                       (1ULL << 3)
#define TWSTOP_RULE_EVENT_OUT_CASE                      (1ULL << 4)
#define TWSTOP_RULE_EVENT_PEER_IN_CASE                  (1ULL << 5)
#define TWSTOP_RULE_EVENT_PEER_OUT_CASE                 (1ULL << 6)

#define TWSTOP_RULE_EVENT_ROLE_SWITCH                   (1ULL << 7)
#define TWSTOP_RULE_EVENT_ROLE_SELECTED_PRIMARY         (1ULL << 8)
#define TWSTOP_RULE_EVENT_ROLE_SELECTED_SECONDARY       (1ULL << 9)
#define TWSTOP_RULE_EVENT_ROLE_SELECTED_ACTING_PRIMARY  (1ULL << 10)

#define TWSTOP_RULE_EVENT_PEER_LINKLOSS                 (1ULL << 11)
#define TWSTOP_RULE_EVENT_HANDSET_LINKLOSS              (1ULL << 12)

#define TWSTOP_RULE_EVENT_PEER_CONNECTED_BREDR          (1ULL << 13)
#define TWSTOP_RULE_EVENT_PEER_DISCONNECTED_BREDR       (1ULL << 14)

#define TWSTOP_RULE_EVENT_HANDSET_CONNECTED_BREDR       (1ULL << 15)
#define TWSTOP_RULE_EVENT_HANDSET_CONNECTED_A2DP        (1ULL << 16)
#define TWSTOP_RULE_EVENT_HANDSET_CONNECTED_AVRCP       (1ULL << 17)
#define TWSTOP_RULE_EVENT_HANDSET_CONNECTED_HFP         (1ULL << 18)

#define TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_BREDR    (1ULL << 19)
#define TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_A2DP     (1ULL << 20)
#define TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_AVRCP    (1ULL << 21)
#define TWSTOP_RULE_EVENT_HANDSET_DISCONNECTED_HFP      (1ULL << 22)

    /*! Event used as a command to enter the DFU role */
#define TWSTOP_RULE_EVENT_DFU_ROLE                      (1ULL << 23)
#define TWSTOP_RULE_EVENT_DFU_ROLE_COMPLETE             (1ULL << 24)

#define TWSTOP_RULE_EVENT_FAILED_PEER_CONNECT           (1ULL << 25)

#define TWSTOP_RULE_EVENT_STATIC_HANDOVER               (1ULL << 26)
#define TWSTOP_RULE_EVENT_STATIC_HANDOVER_FAILED        (1ULL << 27)

#define TWSTOP_RULE_EVENT_HANDOVER                      (1ULL << 28)
#define TWSTOP_RULE_EVENT_HANDOVER_FAILED               (1ULL << 29)
#define TWSTOP_RULE_EVENT_HANDOVER_FAILURE_HANDLED      (1ULL << 30)

#define TWSTOP_RULE_EVENT_PROHIBIT_CONNECT_TO_HANDSET   (1ULL << 31)
#define TWSTOP_RULE_EVENT_HANDSET_ACL_CONNECTED         (1ULL << 32)

/*! Generic "kick" event, used by goals to initiate reevaluation of
    rules following goal completion. Typically used where there may
    be "deferred" events that could not be handled due to the 
    in-progress goal. */
#define TWSTOP_RULE_EVENT_KICK                          (1ULL << 33)

#define TWSTOP_RULE_EVENT_DFU_PEER_CONNECTED            (1ULL << 34)

#define TWSTOP_RULE_EVENT_DFU_PRI_REBOOT_DONE           (1ULL << 35)

#define TWSTOP_RULE_EVENT_FAILED_SWITCH_SECONDARY       (1ULL << 36)
#endif /* _TWS_TOPOLOGY_RULE_EVENTS_H_ */
