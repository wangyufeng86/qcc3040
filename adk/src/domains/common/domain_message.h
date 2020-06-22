/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Header file for global message definitions.

            This header file uses X-macros to convert lists of message groups
            into enumerations of {module}_MESSAGE_GROUP and {module}_MESSAGE_BASE.

\note   When registering for domain messages using
        MessageBroker_RegisterInterestInMsgGroups the registrant much comply
        with the requirements of the domain component's message interface. In
        particular, this means the registrant _must_ respond to messages that
        require a response. Not responding to messages that require a response
        is likely to result in unexpected behavior. The comment after each
        component in #FOREACH_DOMAINS_MESSAGE_GROUP identifies the name of the
        enumeration/typedef that defines the component's interface.
*/
#ifndef DOMAIN_MESSAGE_H_
#define DOMAIN_MESSAGE_H_

#include "message_broker.h"

/*! \brief Macro to convert a message group, to a message ID */
#define MSG_GRP_TO_ID(x)        ((x)<<8)
/*! \brief Macro to convert a message ID, to a message group */
#define ID_TO_MSG_GRP(x)        ((message_group_t)((x)>>8))
/*! \brief Macro to return the last message id in a message group */
#define LAST_ID_IN_MSG_GRP(x)   (MSG_GRP_TO_ID(x)+0xFF)

/*! A table of domain component names. */
#define FOREACH_DOMAINS_MESSAGE_GROUP(X) \
    X(INTERNAL) \
    X(AV)                   /* See #av_status_messages */ \
    X(APP_HFP)              /* See #hfp_messages */ \
    X(PAIRING)              /* See #pairing_messages */ \
    X(AV_GAIA)              /* See #av_headet_gaia_messages */ \
    X(AV_UPGRADE)           /* See #av_headet_upgrade_messages */ \
    X(CON_MANAGER)          /* See #av_headset_conn_manager_messages */ \
    X(PEER_SIG)             /* See #peer_signalling_messages */ \
    X(HANDSET_SIG)          /* See #handset_signalling_messages */ \
    X(PHY_STATE)            /* See #phy_state_messages */ \
    X(BATTERY_APP)          /* See #battery_messages */ \
    X(ADV_MANAGER)          /* See #adv_mgr_messages_t */ \
    X(SFWD)                 /* See #scofwd_messages */ \
    X(MIRROR_PROFILE)       /* See #mirror_profile_msg_t */ \
    X(PROXIMITY)            /* See #proximity_messages */ \
    X(ACCELEROMETER)        /* See #accelerometer_messages */ \
    X(CHARGER)              /* See #chargerMessages */ \
    X(DEVICE)               /* See #deviceMessages */ \
    X(PROFILE_MANAGER)      /* See #profile_manager_messages */\
    X(APP_GATT)             /* See #av_headet_gatt_messages */ \
    X(POWER_APP)            /* See #powerClientMessages */ \
    X(KYMERA) \
    X(TEMPERATURE)          /* See #temperatureMessages */ \
    X(AUDIO_SYNC)           /* See #audio_sync_msg_t */ \
    X(VOLUME)               /* See #volume_domain_messages */ \
    X(SPARE_PLACE_HOLDER)   /* Temp inserted to preserve values due to hard coded tests elsewhere */ \
    X(PEER_PAIR_LE)         /* See #peer_pair_le_message_t */ \
    X(PEER_FIND_ROLE)       /* See #peer_find_role_message_t */ \
    X(KEY_SYNC)             /* See #key_sync_messages */ \
    X(BREDR_SCAN_MANAGER)   /* See #bredr_scan_manager_messages */ \
    X(UI)                   /* See #ui_message_t */ \
    X(PROMPTS) \
    X(AV_UI)                /* See #av_ui_messages */ \
    X(AV_AVRCP)             /* See #av_avrcp_messages */ \
    X(POWER_UI)             /* See #powerUiMessages */ \
    X(DEVICE_UPGRADE_PEER)  /* See #DeviceUpgradePeer_messages */ \
    X(TELEPHONY)            /* See #telephony_messages */ \
    X(LE_SCAN_MANAGER)      /* See #scan_manager_messages */ \
    X(HANDOVER_PROFILE)     /* See #handover_profile_messages */ \
    X(LOCAL_NAME)           /* See #local_name_messages */ \
    X(LOCAL_ADDR)           /* See #local_addr_messages */ \
    X(DEVICE_TEST)          /* See #device_test_service_message_t */ \
    \
    /* logical input message groups need to be consecutive */ \
    /* Note the symbol LOGICAL_INPUT_MESSAGE_BASE is assumed by ButtonParseXML.py */ \
    X(DEVICE_SPECIFIC_LOGICAL_INPUT) \
    X(LOGICAL_INPUT) \
    \
    X(ANC)                  /* See #anc_messages_t */ \
    X(LEAKTHROUGH)               /* See leakthrough_msg_t */\
    X(QCOM_CON_MANAGER)


/*! A table of service component names */
#define FOREACH_SERVICES_MESSAGE_GROUP(X) \
    X(HANDSET_SERVICE) \
    X(STATE_PROXY) \
    X(HDMA) \
    X(VOLUME_SERVICE) \
    X(VOICE_UI_SERVICE) \
    X(AUDIO_CURATION_SERVICE)

/*! A table of topology component names */
#define FOREACH_TOPOLOGY_MESSAGE_GROUP(X) \
    X(TWS_TOPOLOGY)                 /* See #tws_topology_message_t */ \
    X(TWS_TOPOLOGY_CLIENT_NOTIFIER) /* See #tws_topology_client_notifier_message_t */ \
    X(HEADSET_TOPOLOGY)             /* See #headset_topology_message_t */



/*! A table of app/system component names */
#define FOREACH_APPS_MESSAGE_GROUP(X) \
    X(SYSTEM) \
    X(CONN_RULES) \
    X(EARBUD_ROLE)

/*! A table of UI inputs names */
#define FOREACH_UI_INPUTS_MESSAGE_GROUP(X) \
    X(UI_INPUTS_TELEPHONY) \
    X(UI_INPUTS_MEDIA_PLAYER) \
    X(UI_INPUTS_PEER) \
    X(UI_INPUTS_DEVICE_STATE) \
    X(UI_INPUTS_VOLUME) \
    X(UI_INPUTS_HANDSET) \
    X(UI_INPUTS_AUDIO_CURATION) \
    X(UI_INPUTS_VOICE_UI) \
    X(UI_INPUTS_BOUNDS_CHECK)

/*! This expansion macro concatenates the component name with the text
    _MESSAGE_GROUP. For example, an item in a table 'FOO' will be expanded to the
    enumerated name FOO_MESSAGE_GROUP,.
*/
#define EXPAND_AS_MESSAGE_GROUP_ENUM(component_name) component_name##_MESSAGE_GROUP,

/*! A type to enumerate the message groups available in the system */
enum message_groups
{
    FOREACH_DOMAINS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_GROUP_ENUM)
    FOREACH_SERVICES_MESSAGE_GROUP(EXPAND_AS_MESSAGE_GROUP_ENUM)
    FOREACH_TOPOLOGY_MESSAGE_GROUP(EXPAND_AS_MESSAGE_GROUP_ENUM)
    FOREACH_APPS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_GROUP_ENUM)
    FOREACH_UI_INPUTS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_GROUP_ENUM)
};

/*! The first UI inputs message group ID */
#define UI_INPUTS_MESSAGE_GROUP_START UI_INPUTS_TELEPHONY_MESSAGE_GROUP

/*! This expansion macro concatenates the component name with the text
    _MESSAGE_BASE and assigns a value to the name using the MSG_GROUP_TO_ID
    macro (where the message group is defined in the #message_groups enum).
    For example, an item in a table 'FOO' will be expanded to the
    enumerated name FOO_MESSAGE_BASE=MSG_GROUP_TO_ID(FOO_MESSAGE_GROUP),.
*/
#define EXPAND_AS_MESSAGE_BASE_ENUM(component_name) component_name##_MESSAGE_BASE=MSG_GRP_TO_ID(component_name##_MESSAGE_GROUP),


/*!@{   @name Message ID allocations for each application module
        @brief Each module in the application that sends messages is assigned
               a base message ID. Each module then defines message IDs starting
               from that base ID.
        @note There is no checking that the messages assigned by one module do
              not overrun into the next module's message ID allocation.
*/
typedef enum
{
    FOREACH_DOMAINS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_BASE_ENUM)
    FOREACH_SERVICES_MESSAGE_GROUP(EXPAND_AS_MESSAGE_BASE_ENUM)
    FOREACH_TOPOLOGY_MESSAGE_GROUP(EXPAND_AS_MESSAGE_BASE_ENUM)
    FOREACH_APPS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_BASE_ENUM)
    FOREACH_UI_INPUTS_MESSAGE_GROUP(EXPAND_AS_MESSAGE_BASE_ENUM)
} message_base_t;

typedef enum
{
    PAGING_START = SYSTEM_MESSAGE_BASE,
    PAGING_STOP,
} sys_msg;

/*! Helper macro to create a message broker group regisration.
    Registrations created using this macro are placed in a const linker data section.
*/
#define MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(MESSAGE_GROUP_NAME, REGISTER_FUNCTION, UNREGISTER_FUNCTION) \
_Pragma("datasection message_broker_group_registrations") \
const message_broker_group_registration_t message_broker_group_registration_##MESSAGE_GROUP_NAME = \
    {MESSAGE_GROUP_NAME##_MESSAGE_GROUP, REGISTER_FUNCTION, UNREGISTER_FUNCTION }

/*! Linker defined consts referencing the location of the section containing
    the message broker group registrations. */
extern const message_broker_group_registration_t message_broker_group_registrations_begin[];
extern const message_broker_group_registration_t message_broker_group_registrations_end[];

/*@} */
#endif /* DOMAIN_MESSAGE_H_ */
