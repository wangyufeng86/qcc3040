/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       state_proxy_marshal_defs.c
\brief      Definition of marshalled messages used by State Proxy.

            Pointers
            --
            Define a struct of bitfields as a basic type the size of the struct.
*/

#include "state_proxy.h"
#include "state_proxy_marshal_defs.h"
#include "state_proxy_flags.h"

#include <marshal_common.h>
#include <phy_state.h>
#include <peer_signalling.h>
#include <logging.h>

#include <marshal.h>

/*! Marshal type descriptor for #state_proxy_connection_flags_t */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_connection_flags_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(state_proxy_connection_flags_t));
/*! Marshal type descriptor for #state_proxy_data_flags_t */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_data_flags_t =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(sizeof(state_proxy_data_flags_t));

/*----------------------------------------------------------------------------*/

/*! #state_proxy_version_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_version_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_version_t, uint16, version),
};
/*! #state_proxy_version_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_version_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_version_t, state_proxy_version_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #state_proxy_initial_state_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_initial_state_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_initial_state_t, state_proxy_data_t, state),
};
/*! #state_proxy_initial_state_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_initial_state_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_initial_state_t, state_proxy_initial_state_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #state_proxy_msg_empty_payload_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_msg_empty_payload_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_msg_empty_payload_t, state_proxy_event_type, type),
};
/*! #state_proxy_initial_state_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_msg_empty_payload_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_msg_empty_payload_t, state_proxy_msg_empty_payload_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #state_proxy_initial_state_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_active_handset_addr_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_active_handset_addr_t, bdaddr, active_handset_addr),
};
/*! #state_proxy_initial_state_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_active_handset_addr_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_active_handset_addr_t, state_proxy_active_handset_addr_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #state_proxy_connection_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_connection_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_connection_t, int8, rssi),
    MAKE_MARSHAL_MEMBER(state_proxy_connection_t, uint8, link_quality),
    MAKE_MARSHAL_MEMBER(state_proxy_connection_t, tp_bdaddr, device),
    MAKE_MARSHAL_MEMBER(state_proxy_connection_t, state_proxy_connection_flags_t, flags),
};
/*! #state_proxy_connection_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_connection_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_connection_t, state_proxy_connection_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #STATE_PROXY_MIC_QUALITY_T marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_STATE_PROXY_MIC_QUALITY_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(STATE_PROXY_MIC_QUALITY_T);

/*----------------------------------------------------------------------------*/

/*! #state_proxy_data_t message member descriptor. */
const marshal_member_descriptor_t state_proxy_data_member_descriptors[] = 
{
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, uint8, mic_quality),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, battery_state, battery),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, uint16, battery_voltage),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, state_proxy_data_flags_t, flags),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, bdaddr, handset_addr),
    MAKE_MARSHAL_MEMBER_ARRAY(state_proxy_data_t, state_proxy_connection_t, connection, STATE_PROXY_MAX_CONNECTIONS),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, uint8, anc_mode),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, uint8, anc_leakthrough_gain),
    MAKE_MARSHAL_MEMBER(state_proxy_data_t, uint8, leakthrough_mode),
};
/*! #state_proxy_data_t marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_state_proxy_data_t =
    MAKE_MARSHAL_TYPE_DEFINITION(state_proxy_data_t, state_proxy_data_member_descriptors);

/*----------------------------------------------------------------------------*/

/*! #CON_MANAGER_TP_CONNECT_IND_T marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_CON_MANAGER_TP_CONNECT_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(CON_MANAGER_TP_CONNECT_IND_T);

/*----------------------------------------------------------------------------*/

/*! #CON_MANAGER_TP_DISCONNECT_IND_T marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_CON_MANAGER_TP_DISCONNECT_IND_T =
    MAKE_MARSHAL_TYPE_DEFINITION_BASIC(CON_MANAGER_TP_DISCONNECT_IND_T);

/*----------------------------------------------------------------------------*/

/*! #STATE_PROXY_ANC_DATA_T marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_STATE_PROXY_ANC_DATA_T =
        MAKE_MARSHAL_TYPE_DEFINITION_BASIC(STATE_PROXY_ANC_DATA_T);

/*----------------------------------------------------------------------------*/

/*! #STATE_PROXY_LEAKTHROUGH_DATA_T marshal type descriptor. */
const marshal_type_descriptor_t marshal_type_descriptor_STATE_PROXY_LEAKTHROUGH_DATA_T = 
        MAKE_MARSHAL_TYPE_DEFINITION_BASIC(STATE_PROXY_LEAKTHROUGH_DATA_T);

/*----------------------------------------------------------------------------*/


/*! X-Macro generate state proxy marshal type descriptor set that can be passed to a (un)marshaller
 *  to initialise it.
 *  */
#define EXPAND_AS_TYPE_DEFINITION(type) (const marshal_type_descriptor_t *)&marshal_type_descriptor_##type,
const marshal_type_descriptor_t * const state_proxy_marshal_type_descriptors[NUMBER_OF_MARSHAL_OBJECT_TYPES] = {
    MARSHAL_COMMON_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
    MARSHAL_TYPES_TABLE(EXPAND_AS_TYPE_DEFINITION)
};
#undef EXPAND_AS_TYPE_DEFINITION


void stateProxy_MarshalToConnectedPeer(marshal_type_t marshal_type, Message msg, size_t size)
{
    bool send = !stateProxy_Paused() && appPeerSigIsConnected() && stateProxy_IsSecondary();
    DEBUG_LOG("stateProxy_MarshalToConnectedPeer stateProxy_Paused=%u, appPeerSigIsConnected=%u, stateProxy_IsSecondary=%u",
            stateProxy_Paused(), appPeerSigIsConnected(), stateProxy_IsSecondary());

    if (send)
    {
        void* copy;
        SP_LOG_VERBOSE("stateProxy_MarshalToConnectedPeer forwarding type:0x%x to primary", marshal_type);

        /* Cancel any pending messages of this type - its more important to send
        the latest state, so cancel any pending messages. */
        appPeerSigMarshalledMsgChannelTxCancelAll(stateProxy_GetTask(),
                                                  PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                                  marshal_type);

        copy = PanicUnlessMalloc(size);
        memcpy(copy, msg, size);
        appPeerSigMarshalledMsgChannelTx(stateProxy_GetTask(),
                                         PEER_SIG_MSG_CHANNEL_STATE_PROXY,
                                         copy, marshal_type);
    }
}
