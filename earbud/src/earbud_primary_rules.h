/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_primary_rules.h
\brief      Earbud application rules run when in a Primary earbud role.
*/

#ifndef _EARBUD_PRIMARY_RULES_H_
#define _EARBUD_PRIMARY_RULES_H_

#include <domain_message.h>
#include <rules_engine.h>
#include <kymera.h>

enum earbud_primary_rules_messages
{
    /*! Make the device connectable. */
    CONN_RULES_ENABLE_CONNECTABLE = CONN_RULES_MESSAGE_BASE,

    /*! Make the device not connectable. */
    CONN_RULES_DISABLE_CONNECTABLE,

    /*! Use Peer Signalling (AVRCP) to send sync message. */
    CONN_RULES_SEND_PEER_SYNC,

    /*! Use Peer Signalling (AVRCP) to send link-keys message. */
    CONN_RULES_PEER_SEND_LINK_KEYS,

    /*! Start handset pairing. */
    CONN_RULES_HANDSET_PAIR,

    /*! Connect HFP, A2DP and AVRCP to last connected handset. */
    CONN_RULES_CONNECT_HANDSET,

    /*! Connect A2DP and AVRCP to handset that peer is connected to (only TWS+). */
    CONN_RULES_CONNECT_PEER_HANDSET,

    /*! Connect A2DP and AVRCP for audio forwarding to peer earbud. */
    CONN_RULES_CONNECT_PEER,

    /*! Start timer to pause A2DP streaming. */
    CONN_RULES_A2DP_TIMEOUT,

    /*! Start timer to pause A2DP streaming. */
    CONN_RULES_SCO_TIMEOUT,

    /*! Enable LEDs when out of ear. */
    CONN_RULES_LED_ENABLE,

    /*! Disable LEDs when in ear. */
    CONN_RULES_LED_DISABLE,

    /*! Disconnect links to handset and DFU not pending */
    CONN_RULES_DISCONNECT_HANDSET,

    /*! Disconnect link to peer. */
    CONN_RULES_DISCONNECT_PEER,

    /*! Enter DFU mode as we have entered the case with DFU pending */
    CONN_RULES_ENTER_DFU,

    /*! Exit DFU mode as we have left the case with DFU (mode) active */
    CONN_RULES_EXIT_DFU,

    /*! Update upgrade state */
    CONN_RULES_DFU_ALLOW,

    /*! Hangup an active call */
    CONN_RULES_HANGUP_CALL,

    /*! Answer an incoming call */
    CONN_RULES_ANSWER_CALL,

    /*! Transfer SCO Audio from handset to the earbud */
    CONN_RULES_SCO_TRANSFER_TO_EARBUD,

    /*! Transfer SCO Audio from Earbud to the handset */
    CONN_RULES_SCO_TRANSFER_TO_HANDSET,

    /*! Switch microphone to use for voice call. */
    CONN_RULES_SELECT_MIC,

    /*! Control if peer SCO is enabled or disabled. */
    CONN_RULES_PEER_SCO_CONTROL,

    /*! Start ANC Tuning */
    CONN_RULES_ANC_TUNING_START,

    /*! Stop ANC Tuning */
    CONN_RULES_ANC_TUNING_STOP,

    /*! Set the current Primary or Secondary role */
    CONN_RULES_ROLE_DECISION,

    /*! Cancel running timers for pausing A2DP media */
    CONN_RULES_A2DP_TIMEOUT_CANCEL,

    /*! Start playing media */
    CONN_RULES_MEDIA_PLAY,

    /*! Set the remote audio mix */
    CONN_RULES_SET_REMOTE_AUDIO_MIX,

    /*! Set the local audio mix */
    CONN_RULES_SET_LOCAL_AUDIO_MIX,

    /*! Notify peer of MRU handset */
    CONN_RULES_NOTIFY_PEER_MRU_HANDSET,

    /*! Abort DFU */
    CONN_RULES_DFU_ABORT,

    /*! Any rules with RULE_FLAG_PROGRESS_MATTERS are no longer in progress. */
    CONN_RULES_NOP,

    /*! Accept incoming call. */
    CONN_RULES_ACCEPT_INCOMING_CALL,
};

/*! \brief Actions to take after connecting handset. */
typedef enum
{
    RULE_POST_HANDSET_CONNECT_ACTION_NONE,       /*!< Do nothing more */
    RULE_POST_HANDSET_CONNECT_ACTION_PLAY_MEDIA, /*!< Play media */
} rulePostHandsetConnectAction;

/*! \brief Definition of #CONN_RULES_CONNECT_PEER action message. */
typedef struct
{
    /*! bitmask of profiles to connect. */
    uint8 profiles;
} CONN_RULES_CONNECT_PEER_T;

/*! \brief Definition of #CONN_RULES_SELECT_MIC action message. */
typedef struct
{
    /*! TRUE use local microphone, FALSE use remote microphone. */
    micSelection selected_mic;
} CONN_RULES_SELECT_MIC_T;

/*! \brief Definition of #CONN_RULES_PEER_SCO_CONTROL action message. */
typedef struct
{
    /*! TRUE enable peer SCO, FALSE disable peer SCO. */
    bool enable;
} CONN_RULES_PEER_SCO_CONTROL_T;

/*! \brief Definition of #CONN_RULES_DFU_ALLOW action message. */
typedef struct
{
    /*! TRUE    Upgrades (DFU) are allowed.
        FALSE   Upgrades are not allowed*/
    bool enable;
} CONN_RULES_DFU_ALLOW_T;

/*! \brief Definition of #CONN_RULES_ROLE_DECISION message. */
typedef struct
{
    /*! TRUE this Earbud is Primary.
        FALSE this Earbud is Secondary. */
    bool primary;
} CONN_RULES_ROLE_DECISION_T;

/*! \brief Definition of #CONN_RULES_SET_REMOTE_AUDIO_MIX message. */
typedef struct
{
    /*! TRUE - remote earbud renders stereo mix of left and right.
        FALSE - remote earbuds renders mono left or right. */
    bool stereo_mix;
} CONN_RULES_SET_REMOTE_AUDIO_MIX_T;

/*! \brief Definition of #CONN_RULES_SET_LOCAL_AUDIO_MIX message. */
typedef struct
{
    /*! TRUE - local earbud renders stereo mix or left and right.
        FALSE - local earbuds renders mono left or right. */
    bool stereo_mix;
} CONN_RULES_SET_LOCAL_AUDIO_MIX_T;


/*! Definition of all the events that may have rules associated with them */
#define RULE_EVENT_STARTUP                       (1ULL << 0)     /*!< Startup */

#define RULE_EVENT_HANDSET_A2DP_CONNECTED        (1ULL << 1)     /*!< Handset connected */
#define RULE_EVENT_HANDSET_A2DP_DISCONNECTED     (1ULL << 2)     /*!< Handset disconnected */
#define RULE_EVENT_HANDSET_AVRCP_CONNECTED       (1ULL << 3)     /*!< Handset connected */
#define RULE_EVENT_HANDSET_AVRCP_DISCONNECTED    (1ULL << 4)     /*!< Handset disconnected */
#define RULE_EVENT_HANDSET_HFP_CONNECTED         (1ULL << 5)     /*!< Handset connected */
#define RULE_EVENT_HANDSET_HFP_DISCONNECTED      (1ULL << 6)     /*!< Handset disconnected */

#define RULE_EVENT_PEER_CONNECTED                (1ULL << 7)     /*!< Peer connected */
#define RULE_EVENT_PEER_DISCONNECTED             (1ULL << 8)     /*!< Peer disconnected */
#define RULE_EVENT_PEER_SYNC_VALID               (1ULL << 9)     /*!< Peer sync exchanged */
#define RULE_EVENT_PEER_SYNC_FAILED              (1ULL << 10)    /*!< Peer sync failed */
#define RULE_EVENT_PEER_UPDATE_LINKKEYS          (1ULL << 11)    /*!< Linkey for handset updated, potentially need to forward to peer */
#define RULE_EVENT_RX_HANDSET_LINKKEY            (1ULL << 12)    /*!< Receive derived TWS linkkey from peer device */

#define RULES_EVENT_BATTERY_CRITICAL             (1ULL << 13)    /*!< Battery voltage is critical */
#define RULES_EVENT_BATTERY_LOW                  (1ULL << 14)    /*!< Battery voltage is low */
#define RULES_EVENT_BATTERY_OK                   (1ULL << 15)    /*!< Battery voltage is OK */
#define RULES_EVENT_CHARGER_CONNECTED            (1ULL << 16)    /*!< Charger is connected */
#define RULES_EVENT_CHARGER_DISCONNECTED         (1ULL << 17)    /*!< Charger is disconnected */
#define RULES_EVENT_CHARGING_COMPLETED           (1ULL << 18)    /*!< Charging is complete */

#define RULE_EVENT_IN_EAR                        (1ULL << 19)    /*!< Earbud put in ear. */
#define RULE_EVENT_OUT_EAR                       (1ULL << 20)    /*!< Earbud taken out of ear. */
#define RULE_EVENT_IN_CASE                       (1ULL << 21)    /*!< Earbud put in the case. */
#define RULE_EVENT_OUT_CASE                      (1ULL << 22)    /*!< Earbud taken out of case. */

#define RULE_EVENT_MOTION_DETECTED               (1ULL << 23)    /*!< Earbud started moving. */
#define RULE_EVENT_NO_MOTION_DETECTED            (1ULL << 24)    /*!< Earbud stopped moving. */

#define RULE_EVENT_USER_CONNECT                  (1ULL << 25)    /*!< User has requested connect */
                                                       // 26 Unused
#define RULE_EVENT_DFU_CONNECT                   (1ULL << 27)    /*!< Connect for DFU purposes */

#define RULE_EVENT_PAGE_SCAN_UPDATE              (1ULL << 28)    /*!< 'event' for checking page scan */

#define RULE_EVENT_PEER_IN_CASE                  (1ULL << 29)    /*!< Peer put in the case */
#define RULE_EVENT_PEER_OUT_CASE                 (1ULL << 30)    /*!< Peer taken out of the case */
#define RULE_EVENT_PEER_IN_EAR                   (1ULL << 31)    /*!< Peer put in the ear */
#define RULE_EVENT_PEER_OUT_EAR                  (1ULL << 32)    /*!< Peer taken out of ear */

#define RULE_EVENT_PEER_A2DP_CONNECTED           (1ULL << 33)    /*!< Peer handset A2DP connected */
#define RULE_EVENT_PEER_A2DP_DISCONNECTED        (1ULL << 34)    /*!< Peer handset A2DP disconnected */
#define RULE_EVENT_PEER_HFP_CONNECTED            (1ULL << 35)    /*!< Peer handset HFP connected */
#define RULE_EVENT_PEER_HFP_DISCONNECTED         (1ULL << 36)    /*!< Peer handset HFP disconnected */
#define RULE_EVENT_PEER_AVRCP_CONNECTED          (1ULL << 37)    /*!< Peer handset AVRCP connected */
#define RULE_EVENT_PEER_AVRCP_DISCONNECTED       (1ULL << 38)    /*!< Peer handset AVRCP disconnected */
#define RULE_EVENT_PEER_HANDSET_CONNECTED        (1ULL << 39)    /*!< Peer has handset connection */
#define RULE_EVENT_PEER_HANDSET_DISCONNECTED     (1ULL << 40)    /*!< Peer handset has disconnected */

#define RULE_EVENT_PEER_LINK_LOSS                (1ULL << 41)    /*!< Peer link-loss has occurred */
#define RULE_EVENT_HANDSET_LINK_LOSS             (1ULL << 42)    /*!< Handset link-loss has occurred */

#define RULE_EVENT_PEER_A2DP_SUPPORTED           (1ULL << 43)    /*!< Peer handset A2DP now supported */
#define RULE_EVENT_PEER_AVRCP_SUPPORTED          (1ULL << 44)    /*!< Peer handset AVRCP now supported */
#define RULE_EVENT_PEER_HFP_SUPPORTED            (1ULL << 45)    /*!< Peer handset HFP now supported */

#define RULE_EVENT_SCO_ACTIVE                    (1ULL << 46)    /*!< SCO call active */

#define RULE_EVENT_BLE_CONNECTABLE_CHANGE        (1ULL << 47)    /*!< Check status of BLE connection creation */

#define RULE_EVENT_HANDOVER_DISCONNECT           (1ULL << 48)    /*!< Handover disconnect */
#define RULE_EVENT_HANDOVER_RECONNECT            (1ULL << 49)    /*!< Handover reconnect */
#define RULE_EVENT_HANDOVER_RECONNECT_AND_PLAY   (1ULL << 50)    /*!< Handover reconnect then play media */

#define RULE_EVENT_CHECK_DFU                     (1ULL << 51)    /*!< Check whether upgrades should be allowed */

#define RULE_EVENT_PEER_HANDSET_LINK_LOSS        (1ULL << 52)    /*!< Peer has lost link to handset */
#define RULE_EVENT_PEER_PAIRING                  (1ULL << 53)    /*!< Peer is pairing with a handset */
#define RULE_EVENT_ROLE_SWITCH                   (1ULL << 54)    /*!< Role switch occured */

/*! \brief Connection Rules task data. */
typedef struct
{
    rule_set_t rule_set;
} PrimaryRulesTaskData;

/*! \brief Initialise the connection rules module. */
bool PrimaryRules_Init(Task init_task);

/*! \brief Get handle on the primary rule set, in order to directly set/clear events.
    \return rule_set_t The primary rule set.
 */
rule_set_t PrimaryRules_GetRuleSet(void);

/*! \brief Set an event or events
    \param[in] event Events to set that will trigger rules to run
    This function is called to set an event or events that will cause the relevant
    rules in the rules table to run.  Any actions generated will be sent as message
    to the client_task
*/
void PrimaryRules_SetEvent(rule_events_t event_mask);

/*! \brief Reset/clear an event or events
    \param[in] event Events to clear
    This function is called to clear an event or set of events that was previously
    set. Clear event will reset any rule that was run for event.
*/
void PrimaryRules_ResetEvent(rule_events_t event);

/*! \brief Get set of active events
    \return The set of active events.
*/
rule_events_t PrimaryRules_GetEvents(void);

/*! \brief Mark rules as complete from messaage ID
    \param[in] message Message ID that rule(s) generated
    This function is called to mark rules as completed, the message parameter
    is used to determine which rules can be marked as completed.
*/
void PrimaryRules_SetRuleComplete(MessageId message);

/*! \brief Mark rules as complete from message ID and set of events
    \param[in] message Message ID that rule(s) generated
    \param[in] event Event or set of events that trigger the rule(s)
    This function is called to mark rules as completed, the message and event parameter
    is used to determine which rules can be marked as completed.
*/
void PrimaryRulesSetRuleWithEventComplete(MessageId message, rule_events_t event);

#endif /* _EARBUD_PRIMARY_RULES_H_ */
