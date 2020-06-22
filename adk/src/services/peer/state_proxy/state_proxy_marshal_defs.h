/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_marshal_defs.h
\brief      Definition of messages that can be sent between proxy entities.
*/

#ifndef STATE_PROXY_MARSHAL_DEFS_H
#define STATE_PROXY_MARSHAL_DEFS_H

#include <marshal_common.h>
#include <phy_state.h>
#include <av.h>
#include <hfp_profile.h>
#include <pairing.h>
#include <connection_manager.h>

#include <marshal.h>

#include "state_proxy_private.h"

/*! Definition of the state proxy version message. */
typedef struct state_proxy_version
{
    uint16 version;
} state_proxy_version_t;

/*! Definition of the state proxy initial state message. 
    \todo we're using the same method of data transfer as a handover will do
    to send local state to peer to bootstrap it's view of this device.
    As a result this should get subsumed into a common message for sending
    this data type.
 */
typedef struct state_proxy_initial_state
{
    state_proxy_data_t state;
} state_proxy_initial_state_t;

typedef struct state_proxy_active_handset_addr
{
    bdaddr active_handset_addr;
} state_proxy_active_handset_addr_t;

/* Create base list of marshal types the state proxy will use. */
#define MARSHAL_TYPES_TABLE(ENTRY) \
    ENTRY(state_proxy_version_t) \
    ENTRY(PHY_STATE_CHANGED_IND_T) \
    ENTRY(state_proxy_initial_state_t) \
    ENTRY(state_proxy_connection_flags_t) \
    ENTRY(state_proxy_connection_t) \
    ENTRY(state_proxy_data_flags_t) \
    ENTRY(state_proxy_data_t) \
    ENTRY(AV_A2DP_CONNECTED_IND_T) \
    ENTRY(AV_A2DP_DISCONNECTED_IND_T) \
    ENTRY(AV_AVRCP_CONNECTED_IND_T) \
    ENTRY(AV_AVRCP_DISCONNECTED_IND_T) \
    ENTRY(AV_STREAMING_ACTIVE_IND_T) \
    ENTRY(AV_STREAMING_INACTIVE_IND_T) \
    ENTRY(APP_HFP_CONNECTED_IND_T) \
    ENTRY(APP_HFP_DISCONNECTED_IND_T) \
    ENTRY(APP_HFP_SCO_CONNECTED_IND_T) \
    ENTRY(APP_HFP_SCO_DISCONNECTED_IND_T) \
    ENTRY(state_proxy_msg_empty_payload_t) \
    ENTRY(PAIRING_ACTIVITY_T) \
    ENTRY(state_proxy_active_handset_addr_t) \
    ENTRY(MESSAGE_BATTERY_LEVEL_UPDATE_VOLTAGE_T) \
    ENTRY(MESSAGE_BATTERY_LEVEL_UPDATE_STATE_T) \
    ENTRY(CON_MANAGER_TP_CONNECT_IND_T) \
    ENTRY(CON_MANAGER_TP_DISCONNECT_IND_T) \
    ENTRY(STATE_PROXY_MIC_QUALITY_T) \
    ENTRY(STATE_PROXY_ANC_DATA_T) \
    ENTRY(STATE_PROXY_LEAKTHROUGH_DATA_T)

/* X-Macro generate enumeration of all marshal types */
#define EXPAND_AS_ENUMERATION(type) MARSHAL_TYPE(type),
enum MARSHAL_TYPES
{
    /* common types must be placed at the start of the enum */
    DUMMY = NUMBER_OF_COMMON_MARSHAL_OBJECT_TYPES-1,
    /* now expand the marshal types specific to this component. */
    MARSHAL_TYPES_TABLE(EXPAND_AS_ENUMERATION)
    NUMBER_OF_MARSHAL_OBJECT_TYPES
};
#undef EXPAND_AS_ENUMERATION

/*! Marshal type for link quality */
#define MARSHAL_TYPE_STATE_PROXY_LINK_QUALITY_T MARSHAL_TYPE_state_proxy_connection_t
COMPILE_TIME_ASSERT(sizeof(STATE_PROXY_LINK_QUALITY_T) == sizeof(state_proxy_connection_t), invalid_link_quality_marsal_define);

/*! Marshal type for state_proxy_event_type enum. */
#define MARSHAL_TYPE_state_proxy_event_type MARSHAL_TYPE_uint32
COMPILE_TIME_ASSERT(sizeof(state_proxy_event_type) == sizeof(uint32), invalid_event_type_marsal_define);

/*! Marshal type for battery state */
#define MARSHAL_TYPE_battery_state  MARSHAL_TYPE_uint8
COMPILE_TIME_ASSERT(sizeof(battery_state) == sizeof(uint8), invalid_battery_state_type_marsal_define);

/* Make the array of all message marshal descriptors available. */
extern const marshal_type_descriptor_t * const state_proxy_marshal_type_descriptors[];

/*! \brief Marshal a message to the peer.

    \param marshal_type The marshal type of the message.
    \param msg The message content.
    \param size The size of the message content.

    The message will be marshalled to the peer if the module is not paused, if
    it is secondary role, and if peer signalling is connected.
*/
void stateProxy_MarshalToConnectedPeer(marshal_type_t marshal_type, Message msg, size_t size);

#endif /* STATE_PROXY_MARSHAL_DEFS_H */
