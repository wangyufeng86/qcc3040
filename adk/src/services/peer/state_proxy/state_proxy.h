/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\defgroup   state_proxy State Proxy
\ingroup    peer_service
\brief      A component providing local and remote state.
*/

#ifndef STATE_PROXY_H
#define STATE_PROXY_H

#include <phy_state.h>
#include <av.h>
#include <hfp_profile.h>
#include <pairing.h>
#include <connection_manager.h>
#include <anc_state_manager.h>
#include <aec_leakthrough.h>

/*! Types of device for which state changes are monitored. */
typedef enum
{
    /*! This device. */
    state_proxy_source_local,

    /*! Remote device for which state is being proxied. */
    state_proxy_source_remote,
} state_proxy_source;

/*! Flags related to connection events. */
typedef struct
{
    /*! Connection is to a peer device. */
    bool is_peer:1;
} state_proxy_connection_flags_t;

/*! State data for a connection. */
typedef struct
{
    /*! Received Signal Strength Indication in dB.
     *  Range -128..127. */
    int8 rssi;

    /*! Measure of the quality of the connection.
     *  Range 0(worst)..255(best). */
    uint8 link_quality;

    /*! BT address of the remote device on the connection. */
    tp_bdaddr device;

    /*! Flags indication properties of the connection record. */
    state_proxy_connection_flags_t flags;
} state_proxy_connection_t;

/*! Current state of the device battery. */
typedef enum
{
    battery_state_unknown,
    battery_state_too_low,
    battery_state_critical,
    battery_state_low,
    battery_state_ok,
} battery_state;

/*! Enumeration of event types supported by state proxy. */
typedef enum
{
    state_proxy_event_type_phystate             = 1UL << 0,
    state_proxy_event_type_a2dp_conn            = 1UL << 1,
    state_proxy_event_type_a2dp_discon          = 1UL << 2,
    state_proxy_event_type_a2dp_streaming       = 1UL << 3,
    state_proxy_event_type_a2dp_not_streaming   = 1UL << 4,
    state_proxy_event_type_avrcp_conn           = 1UL << 5,
    state_proxy_event_type_avrcp_discon         = 1UL << 6,
    state_proxy_event_type_hfp_conn             = 1UL << 7,
    state_proxy_event_type_hfp_discon           = 1UL << 8,
    state_proxy_event_type_sco_active           = 1UL << 9,
    state_proxy_event_type_sco_inactive         = 1UL << 10,
    state_proxy_event_type_is_pairing           = 1UL << 11,
    state_proxy_event_type_battery_state        = 1UL << 12,
    state_proxy_event_type_battery_voltage      = 1UL << 13,
    state_proxy_event_type_a2dp_supported       = 1UL << 14,
    state_proxy_event_type_avrcp_supported      = 1UL << 15,
    state_proxy_event_type_hfp_supported        = 1UL << 16,
    state_proxy_event_type_peer_linkloss        = 1UL << 17,
    state_proxy_event_type_handset_linkloss     = 1UL << 18,
    state_proxy_event_type_pairing              = 1UL << 19,
    state_proxy_event_type_link_quality         = 1UL << 20,
    state_proxy_event_type_mic_quality          = 1UL << 21,
    state_proxy_event_type_anc                  = 1UL << 22,
    state_proxy_event_type_leakthrough          = 1UL << 23,
} state_proxy_event_type;

/*\{*/

/*! Messages sent by the state proxy component to clients. */
enum state_proxy_messages
{
    /*! Event notification of change in state of a monitored device. */
    STATE_PROXY_EVENT = STATE_PROXY_MESSAGE_BASE,

    /*! Notification that state_proxy_initial_state_t message transmitted. */
    STATE_PROXY_EVENT_INITIAL_STATE_SENT,

    /*! Notification that state_proxy_initial_state_t message has been received. */
    STATE_PROXY_EVENT_INITIAL_STATE_RECEIVED,
};

/*! Definition of data for state_proxy_event_type_link_quality events.
 */
typedef state_proxy_connection_t STATE_PROXY_LINK_QUALITY_T;

/*! Value indicating microphone quality is unavailable i.e. SCO inactive. */
#define MIC_QUALITY_UNAVAILABLE 0xFF

/*! Definition of data for state_proxy_event_type_mic_quality events.
 */
typedef struct
{
    /*! Current microphone quality level.
     *  Valid range 0(worst)..15(best).
     *  Value of MIC_QUALITY_UNAVAILABLE indicates no microphone quality data. */
    uint8 mic_quality;
} STATE_PROXY_MIC_QUALITY_T;

/*! Definition of data for state_proxy_event_type_anc events.
 */

typedef anc_sync_data_t STATE_PROXY_ANC_DATA_T;

/*! Definition of data for state_proxy_event_type_leakthrough events. */
typedef leakthrough_sync_data_t STATE_PROXY_LEAKTHROUGH_DATA_T;

/*! Definition of message notifying clients of change in specific state. */
typedef struct
{
    /*! Source of the state change. */
    state_proxy_source source;

    /*! Type of the state change. */
    state_proxy_event_type type;

    /*! System clock time that the event was generated in ms. */
    uint32 timestamp;

    /*! Payload of the state change message.
     *  Note that not all event types have payloads, for example those generated
     *  by AV_STREAMING_ACTIVE_IND or APP_HFP_SCO_DISCONNECTED */
    union
    {
        PHY_STATE_CHANGED_IND_T phystate;
        AV_A2DP_CONNECTED_IND_T a2dp_conn;
        AV_A2DP_DISCONNECTED_IND_T a2dp_discon;
        AV_AVRCP_CONNECTED_IND_T avrcp_conn;
        AV_AVRCP_DISCONNECTED_IND_T avrcp_discon;
        APP_HFP_CONNECTED_IND_T hfp_conn;
        APP_HFP_DISCONNECTED_IND_T hfp_discon;
        PAIRING_ACTIVITY_T handset_activity;
        CON_MANAGER_TP_CONNECT_IND_T connection;
        CON_MANAGER_TP_DISCONNECT_IND_T disconnection;
        STATE_PROXY_LINK_QUALITY_T link_quality;
        STATE_PROXY_MIC_QUALITY_T mic_quality;
        STATE_PROXY_ANC_DATA_T anc_data;
        STATE_PROXY_LEAKTHROUGH_DATA_T leakthrough_data;
    } event;
} STATE_PROXY_EVENT_T;

/*! \brief Initialise the State Proxy component.

    \param[in] init_task Task of the initialisation component.

    \return bool TRUE Initialisation successful
                 FALSE Initialisation failed
*/
bool StateProxy_Init(Task init_task);

/*! \brief Register a task for event(s) updates.
 
    Register a client task to receive updates for changes in specific event types.

    \param[in] client_task Task to register for #STATE_PROXY_EVENT_T messages. 
    \param[in] event_mask Mask of state_proxy_event_type events to register.
*/
void StateProxy_EventRegisterClient(Task client_task, state_proxy_event_type event_mask);

/*! \brief Unregister event(s) updates for the specified task.
 
    \param[in] client_task Task to unregister from further #STATE_PROXY_EVENT_T message for event_mask events. 
    \param[in] event_mask Mask of events types to unregister.
*/
void StateProxy_EventUnregisterClient(Task client_task, state_proxy_event_type event_mask);

/*! \brief Register for events concerning state proxy itself. */
void StateProxy_StateProxyEventRegisterClient(Task client_task);

/*! \brief Send current device state to peer to initialise event baseline.
*/
void StateProxy_SendInitialState(void);

/*! \brief Inform state proxy of current device Primary/Secondary role.
    \param primary TRUE primary role, FALSE secondary role.
*/
void StateProxy_SetRole(bool primary);

/*! \brief Prevent State Proxy from forwarding any events to peer.
    \note A Call to StateProxy_SendInitialState restarts forwarding.
*/
void StateProxy_Stop(void);

/*! \brief Has initial state been received from peer.
    \return TRUE if initial state received, otherwise FALSE.
*/
bool StateProxy_InitialStateReceived(void);

/* Peer state access functions */
bool StateProxy_IsPeerInCase(void);
bool StateProxy_IsPeerOutOfCase(void);
bool StateProxy_IsPeerInEar(void);
bool StateProxy_IsPeerOutOfEar(void);
bool StateProxy_IsPeerHandsetTws(void);
bool StateProxy_IsPeerHandsetA2dpConnected(void);
bool StateProxy_IsPeerHandsetAvrcpConnected(void);
bool StateProxy_IsPeerHandsetHfpConnected(void);
bool StateProxy_IsPeerHandsetA2dpStreaming(void);
bool StateProxy_IsPeerScoActive(void);
bool StateProxy_IsPeerPairing(void);
bool StateProxy_HasPeerHandsetPairing(void);
bool StateProxy_IsPeerAdvertising(void);
bool StateProxy_IsPeerBleConnected(void);
bool StateProxy_IsPeerDfuInProgress(void);

/* Local state access functions */
bool StateProxy_IsInCase(void);
bool StateProxy_IsOutOfCase(void);
bool StateProxy_IsInEar(void);
bool StateProxy_IsOutOfEar(void);
bool StateProxy_IsHandsetTws(void);
bool StateProxy_IsHandsetA2dpConnected(void);
bool StateProxy_IsHandsetAvrcpConnected(void);
bool StateProxy_IsHandsetHfpConnected(void);
bool StateProxy_IsHandsetA2dpStreaming(void);
bool StateProxy_IsScoActive(void);
bool StateProxy_IsPairing(void);
bool StateProxy_HasHandsetPairing(void);
bool StateProxy_IsAdvertising(void);
bool StateProxy_IsBleConnected(void);
bool StateProxy_IsDfuInProgress(void);

void StateProxy_GetLocalAndRemoteBatteryLevels(uint16 *battery_level, uint16 *peer_battery_level);
void StateProxy_GetLocalAndRemoteBatteryStates(battery_level_state *battery_state, 
                                               battery_level_state *peer_battery_state);
void StateProxy_GetPeerHandsetAddr(bdaddr *peer_handset_addr);
bool StateProxy_IsPrimary(void);
bool StateProxy_GetPeerAncState(void);
uint8 StateProxy_GetPeerAncMode(void);
bool StateProxy_GetPeerLeakthroughState(void);
uint8 StateProxy_GetPeerLeakthroughMode(void);
/*\}*/

#endif /* STATE_PROXY_H */
