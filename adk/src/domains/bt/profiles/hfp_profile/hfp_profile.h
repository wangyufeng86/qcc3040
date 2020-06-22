/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   hfp_profile HFP Profile
\ingroup    profiles
\brief      Interface to application domain HFP component.

    @startuml

    [*] --> NULL
    NULL --> HFP_STATE_INITIALISING_HFP

    HFP_STATE_INITIALISING_HFP : Initialising HFP instance
    HFP_STATE_INITIALISING_HFP --> HFP_STATE_DISCONNECTED : HFP_INIT_CFM

    HFP_STATE_DISCONNECTED : No HFP connection
    state HFP_STATE_CONNECTING {
        HFP_STATE_CONNECTING_LOCAL : Locally initiated connection in progress
        HFP_STATE_CONNECTING_REMOTE : Remotely initiated connection is progress
        HFP_STATE_DISCONNECTED -down-> HFP_STATE_CONNECTING_LOCAL
        HFP_STATE_DISCONNECTED -down-> HFP_STATE_CONNECTING_REMOTE
    }

    state HFP_STATE_CONNECTED {

        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_IDLE : no_call_setup/no_call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_INCOMING : incoming_call_setup/no_call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_OUTGOING : outgoing_call_setup/no_call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_OUTGOING : outgoing_call_alerting_setup/no_call

        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_ACTIVE : no_call_setup/call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_ACTIVE : incoming_call_setup/call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_ACTIVE : outgoing_call_setup/call
        HFP_STATE_CONNECTING --> HFP_STATE_CONNECTED_ACTIVE : outgoing_call_alerting_setup/call

        HFP_STATE_CONNECTED_IDLE : HFP connected, no call in progress
        HFP_STATE_CONNECTED_IDLE -down-> HFP_STATE_CONNECTED_ACTIVE
        HFP_STATE_CONNECTED_OUTGOING : HFP connected, outgoing call in progress
        HFP_STATE_CONNECTED_INCOMING -right-> HFP_STATE_CONNECTED_ACTIVE
        HFP_STATE_CONNECTED_INCOMING : HFP connected, incoming call in progress
        HFP_STATE_CONNECTED_OUTGOING -left-> HFP_STATE_CONNECTED_ACTIVE
        HFP_STATE_CONNECTED_ACTIVE : HFP connected, active call in progress
        HFP_STATE_CONNECTED_ACTIVE -down-> HFP_STATE_DISCONNECTING
        HFP_STATE_DISCONNECTING :

        HFP_STATE_DISCONNECTING -up-> HFP_STATE_DISCONNECTED
    }

    @enduml

*/

#ifndef HFP_PROFILE_H_
#define HFP_PROFILE_H_

#include <hfp.h>
#include <task_list.h>

#include "bt_device.h"
#include "hfp_profile_typedef.h"

#include <marshal.h>

#ifdef INCLUDE_HFP

/*! \brief HFP UI Provider contexts */
typedef enum
{
    context_hfp_disconnected,
    context_hfp_connected,
    context_hfp_voice_call_incoming,
    context_hfp_voice_call_outgoing,
    context_hfp_voice_call_sco_active,
    context_hfp_voice_call_sco_inactive,

} hfp_provider_context_t;

#define HFP_CONNECT_NO_ERROR_UI (1 << 0)    /*!< Don't indicate connection error */
#define HFP_CONNECT_NO_UI       (1 << 1)    /*!< Don't indicate connection success or error */
#define	HFP_DISCONNECT_NO_UI    (1 << 2)    /*!< Don't indicate disconnection */

/*! Expose access to the app HFP taskdata, so the accessor macros work. */
extern hfpTaskData appHfp;

/*! \brief HFP settings structure

    This structure defines the HFP settings that are stored
    in persistent store.
*/
typedef struct
{
    unsigned int volume:4;          /*!< Speaker volume */
    unsigned int mic_volume:4;      /*!< Microphone volume */
} hfpPsConfigData;

/*! \brief Internal message IDs */
enum
{
    HFP_INTERNAL_CONFIG_WRITE_REQ,              /*!< Internal message to store the HFP device config */
    HFP_INTERNAL_SCO_UNENCRYPTED_IND,           /*!< Internal message to indicate SCO is unencrypted */
    HFP_INTERNAL_HSP_INCOMING_TIMEOUT,          /*!< Internal message to indicate timeout from incoming call */
    HFP_INTERNAL_HFP_CONNECT_REQ,               /*!< Internal message to connect to HFP */
    HFP_INTERNAL_HFP_DISCONNECT_REQ,            /*!< Internal message to disconnect HFP */
    HFP_INTERNAL_HFP_LAST_NUMBER_REDIAL_REQ,    /*!< Internal message to request last number redial */
    HFP_INTERNAL_HFP_VOICE_DIAL_REQ,            /*!< Internal message to request voice dial */
    HFP_INTERNAL_HFP_VOICE_DIAL_DISABLE_REQ,    /*!< Internal message to disable voice dial */
    HFP_INTERNAL_HFP_CALL_ACCEPT_REQ,           /*!< Internal message to accept an incoming call */
    HFP_INTERNAL_HFP_CALL_REJECT_REQ,           /*!< Internal message to reject an incoming call */
    HFP_INTERNAL_HFP_CALL_HANGUP_REQ,           /*!< Internal message to hang up an active call */
    HFP_INTERNAL_HFP_MUTE_REQ,                  /*!< Internal message to mute an active call */
    HFP_INTERNAL_HFP_TRANSFER_REQ,              /*!< Internal message to transfer active call between AG and device */
    HFP_INTERNAL_VOLUME_UP,                     /*!< Internal message to increase the volume on the active call */
    HFP_INTERNAL_VOLUME_DOWN,                   /*!< Internal message to decrease the volume on the active call */
    HFP_INTERNAL_NUMBER_DIAL_REQ
};

/*! \brief Message IDs from HFP to main application task */
enum hfp_messages
{
    APP_HFP_INIT_CFM = APP_HFP_MESSAGE_BASE,    /*!< Indicate HFP has been initialised */
    APP_HFP_CONNECTED_IND,                      /*!< SLC connected */
    APP_HFP_DISCONNECTED_IND,                   /*!< SLC disconnected */
    APP_HFP_SCO_CONNECTING_IND,                 /*!< Starting SCO connection */
    APP_HFP_SCO_CONNECTED_IND,                  /*!< Active SCO connected*/
    APP_HFP_SCO_DISCONNECTED_IND,               /*!< SCO channel disconnect */
    APP_HFP_SLC_STATUS_IND,                     /*!< SLC status updated */
    APP_HFP_AT_CMD_CFM,                         /*!< Result of an send AT command request */
    APP_HFP_AT_CMD_IND,                         /*!< AT command received not handled within HFP profile  */
    APP_HFP_SCO_INCOMING_RING_IND,              /*!< There is an incoming call (not connected) */
    APP_HFP_SCO_INCOMING_ENDED_IND,             /*!< Incoming call has gone away (unanswered) */
    APP_HFP_VOLUME_IND,                         /*!< New HFP volume level */
    APP_HFP_CONNECT_CFM,
    APP_HFP_DISCONNECT_CFM
};

typedef struct
{
    device_t device;
    bool successful;
} APP_HFP_CONNECT_CFM_T;

typedef struct
{
    device_t device;
    bool successful;
} APP_HFP_DISCONNECT_CFM_T;

/*! Message sent to status_notify_list clients indicating HFP profile has connected. */
typedef struct
{
    bdaddr bd_addr;                 /*!< Address of AG */
} APP_HFP_CONNECTED_IND_T;
extern const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_CONNECTED_IND_T;

/*! \brief HFP disconnect reasons */
typedef enum
{
    APP_HFP_CONNECT_FAILED,             /*!< Connect attempt failed */
    APP_HFP_DISCONNECT_LINKLOSS,        /*!< Disconnect due to link loss following supervision timeout */
    APP_HFP_DISCONNECT_NORMAL,          /*!< Disconnect initiated by local or remote device */
    APP_HFP_DISCONNECT_ERROR            /*!< Disconnect due to unknown reason */
} appHfpDisconnectReason;

/*! Message sent to status_notify_list clients indicating HFP profile has disconnected. */
typedef struct
{
    bdaddr bd_addr;                 /*!< Address of AG */
    appHfpDisconnectReason reason;  /*!< Disconnection reason */
} APP_HFP_DISCONNECTED_IND_T;
extern const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_DISCONNECTED_IND_T;

/*! Dummy message to permit marhsalling type definition. */
typedef struct
{
    uint32 reserved;
} APP_HFP_SCO_CONNECTED_IND_T;
extern const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_SCO_CONNECTED_IND_T;

/*! Dummy message to permit marhsalling type definition. */
typedef struct
{
    uint32 reserved;
} APP_HFP_SCO_DISCONNECTED_IND_T;
extern const marshal_type_descriptor_t marshal_type_descriptor_APP_HFP_SCO_DISCONNECTED_IND_T;

/*! Message sent to status_notify_list clients indicating SLC state. */
typedef struct
{
    bool slc_connected;                 /*!< SLC Connected True/False */
    hfp_link_priority priority;         /*!< Priority of the link - used for multiple hfp connections */
    bdaddr bd_addr;                     /*!< Address of AG */
} APP_HFP_SLC_STATUS_IND_T;

/*! Message sent to at_cmd_task with result of AT command transmission. */
typedef struct
{
    bool status;                        /*!< Status indicating if AT command was sent successfully */
} APP_HFP_AT_CMD_CFM_T;

/*! Message sent to at_cmd_task indicating new incoming TWS AT command. */
typedef struct
{
    hfp_link_priority priority;         /*!< Priority of the link - used for multiple hfp connections */
    uint16 size_data;                   /*!< Size of the AT command */
    uint8 data[1];                      /*!< AT command */
} APP_HFP_AT_CMD_IND_T;

/*! Definition of #APP_HFP_VOLUME_IND message sent to registered
    clients. */
typedef struct
{
    uint8 volume;           /*!< Latest HFP volume level. */
} APP_HFP_VOLUME_IND_T;

/*! Internal connect request message */
typedef struct
{
    bdaddr addr;            /*!< Address of AG */
    hfp_profile profile;    /*!< Profile to use */
    uint16 flags;           /*!< Connection flags */
} HFP_INTERNAL_HFP_CONNECT_REQ_T;

/*! Internal disconnect request message */
typedef struct
{
    bool silent;            /*!< Disconnect silent flag */
} HFP_INTERNAL_HFP_DISCONNECT_REQ_T;

/*! Internal mute request message */
typedef struct
{
    bool mute;              /*!< Mute enable/disable */
} HFP_INTERNAL_HFP_MUTE_REQ_T;

/*! Internal audio transfer request message */
typedef struct
{
    bool transfer_to_ag;    /*!< Transfer to AG/Headset */
} HFP_INTERNAL_HFP_TRANSFER_REQ_T;

typedef struct
{
    unsigned length;
    uint8  number[1];
} HFP_INTERNAL_NUMBER_DIAL_REQ_T;

/*! \brief Get application HFP component. */
#define appGetHfp()         (&appHfp)

/*! \brief Get HFP sink */
#define appHfpGetSink() (appGetHfp()->slc_sink)

/*! \brief Get HFP lock */
#define appHfpGetLock() (appGetHfp()->hfp_lock)

/*! \brief Set HFP lock */
#define appHfpSetLock(lock) (appGetHfp()->hfp_lock = (lock))

/*! \brief Get current HFP state */
#define appHfpGetState() (appGetHfp()->state)

/*! \brief Get current AG address */
#define appHfpGetAgBdAddr() (&(appGetHfp()->ag_bd_addr))

/*! \brief Get SLC status notify list */
#define appHfpGetSlcStatusNotifyList() (task_list_flexible_t *)(&(appGetHfp()->slc_status_notify_list))

/*! \brief Get status notify list */
#define appHfpGetStatusNotifyList() (task_list_flexible_t *)(&(appGetHfp()->status_notify_list))

/*! \brief Is HFP connected */
#define appHfpIsConnected() \
    ((appHfpGetState() >= HFP_STATE_CONNECTED_IDLE) && (appHfpGetState() <= HFP_STATE_CONNECTED_ACTIVE))

/*! \brief Is HFP in a call */
#define appHfpIsCall() \
    ((appHfpGetState() >= HFP_STATE_CONNECTED_OUTGOING) && (appHfpGetState() <= HFP_STATE_CONNECTED_ACTIVE))

/*! \brief Is HFP in an active call */
#define appHfpIsCallActive() \
    (appHfpGetState() == HFP_STATE_CONNECTED_ACTIVE)

/*! \brief Is HFP in an incoming call */
#define appHfpIsCallIncoming() \
    (appHfpGetState() == HFP_STATE_CONNECTED_INCOMING)

/*! \brief Is HFP in an outgoing call */
#define appHfpIsCallOutgoing() \
    (appHfpGetState() == HFP_STATE_CONNECTED_OUTGOING)

/*! \brief Is HFP disconnected */
#define appHfpIsDisconnected() \
    ((appHfpGetState() < HFP_STATE_CONNECTING_LOCAL) || (appHfpGetState() > HFP_STATE_DISCONNECTING))

/*! \brief Is HFP SCO active */
#define appHfpIsScoActive() \
    (appGetHfp()->sco_sink != 0)

/*! \brief Is HFP SCO/ACL encrypted */
#define appHfpIsEncrypted() \
    (appGetHfp()->bitfields.encrypted)

/*! \brief Is HFP voice recognition active */
#define appHfpIsVoiceRecognitionActive() \
    (appGetHfp()->bitfields.voice_recognition_active)

/*! \brief Is current profile HSP */
#define appHfpIsHsp() \
    (appGetHfp()->profile == hfp_headset_profile)

/*! \brief Is microphone muted */
#define appHfpIsMuted() \
    (appGetHfp()->bitfields.mute_active)

Task appGetHfpTask(void);

bool appHfpInit(Task init_task);
void appHfpRegisterAcceptCallCallback(bool (*accept_call_callback)(void));
bool appHfpConnectHandset(void);
bool appHfpConnectWithBdAddr(const bdaddr *bd_addr, hfp_profile profile);
bool appHfpDisconnectInternal(void);
void appHfpVolumeStart(int16 step);
void appHfpVolumeStop(int16 step);
void appHfpMuteToggle(void);
void appHfpTransferToAg(void);
void appHfpTransferToHeadset(void);
void appHfpCallVoice(void);
void appHfpCallVoiceDisable(void);
void appHfpCallAccept(void);
void appHfpCallReject(void);
void appHfpCallHangup(void);
void appHfpCallLastDialed(void);
void appHfpClientRegister(Task task);
void appHfpSendAtCmdReq(hfp_link_priority priority, char* cmd);
void appHfpRegisterAtCmdTask(Task task);
void appHfpStatusClientRegister(Task task);
uint8 appHfpGetVolume(void);
void appHfpConfigStore(void);
void appHfpVolumeNotifyClients(uint8 new_volume);
void appHfpError(MessageId id, Message message);
void appHfpHandleInternalConfigWriteRequest(void);

/*! \brief Register a task to receive HFP message group messages.
    \param task The task that will receive the messages.
    \param group Must be APP_HFP_MESSAGE_GROUP.
*/
void hfpProfile_RegisterHfpMessageGroup(Task task, message_group_t group);

/*! \brief Register a task to receive SYSTEM message group messages.
    \param task The task that will receive the messages.
    \param group Must be SYSTEM_MESSAGE_GROUP.
*/
void hfpProfile_RegisterSystemMessageGroup(Task task, message_group_t group);

void hfpProfile_Connect(const Task client_task, bdaddr *bd_addr);
void hfpProfile_Disconnect(const Task client_task, bdaddr *bd_addr);

/*! \brief Inform hfp profile of current device Primary/Secondary role.

    \param primary TRUE to set Primary role, FALSE to set Secondary role.
*/
void HfpProfile_SetRole(bool primary);

/*! \brief Query if SCO connection is currently connecting */
bool HfpProfile_IsScoConnecting(void);

#else

#define appHfpIsScoActive() (FALSE)
#define HfpProfile_SetRole(primary) UNUSED(primary)

#endif
#endif /* HFP_PROFILE_H_ */
