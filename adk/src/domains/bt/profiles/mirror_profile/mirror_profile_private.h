/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private functions and helpers for the mirror_profile module.
*/

#ifndef MIRROR_PROFILE_PRIVATE_H_
#define MIRROR_PROFILE_PRIVATE_H_

#include <logging.h>
#include <panic.h>

#include <task_list.h>

#ifdef INCLUDE_MIRRORING

#include <app/bluestack/mdm_prim.h>

#include "kymera_adaptation_voice_protected.h"
#include "audio_sync.h"

#include "mirror_profile_protected.h"
#include "mirror_profile_config.h"
#include "mirror_profile_sm.h"
#include "mirror_profile_peer_mode_sm.h"
#include "mirror_profile_peer_audio_sync_l2cap.h"


/*! \{
    Macros for diagnostic output that can be suppressed.
    Allows debug of this module at two levels. */
#define MIRROR_LOG    DEBUG_LOG
/*! \} */

/*! Code assertion that can be checked at run time. This will cause a panic. */
#define assert(x) PanicFalse(x)

/*! An invalid mirror ACL or eSCO connection handle */
#define MIRROR_PROFILE_CONNECTION_HANDLE_INVALID ((uint16)0xFFFF)

/*! Delay before kicking the SM */
#define MIRROR_PROFILE_KICK_LATER_DELAY D_SEC(1)

/*! Messages used internally only in mirror_profile.

    These messages should usually be sent conditionally on the mirror_profile
    state machine transition lock.

    This ensures that they will only be delivered when the state machine is
    in a stable state (e.g. not waiting for a connect request to complete).
*/
typedef enum
{
    /*! Trigger kicking the state machine */
    MIRROR_INTERNAL_DELAYED_KICK,

    /*! Message sent with delay when A2DP or eSCO mirroring becomes idle.
        On delivery, the link policy is setup to reduce power consumption. */
    MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT,

    /*! Message indiating QHS link to peer bud failed to start within
    #mirrorProfileConfig_QhsStartTimeout */
    MIRROR_INTERNAL_QHS_START_TIMEOUT,

    /*! After forming a connection to the peer earbud this message is
        sent to trigger placing the peer link into sniff mode after a timeout.
        This message will be cancelled if mirroring is started before it is
        delivered. The timeout is mirrorProfileConfig_IdlePeerEnterSniffTimeout. */
    MIRROR_INTERNAL_IDLE_PEER_ENTER_SNIFF,

    MIRROR_INTERNAL_MAX
} mirror_profile_internal_msg_t;

/*! \brief State related to the mirror ACL connection  */
typedef struct
{
    /*! The mirror ACL connection handle */
    uint16 conn_handle;

    /*! The mirror ACL's BD_ADDR */
    bdaddr bd_addr;

} mirror_profile_acl_t;

/*! \brief State related to the mirror eSCO connection  */
typedef struct
{
    /*! The mirror eSCO connection handle */
    uint16 conn_handle;

    /*! The mirror eSCO wesco param */
    uint8 wesco;

    /*! The mirror eSCO codec mode (forwarded from Primary) */
    hfp_codec_mode_t codec_mode;

    /*! The mirror eSCO volume (forwarded from Primary) */
    uint8 volume;
} mirror_profile_esco_t;

/*! \brief State related to the mirror A2DP connection  */
typedef struct
{
    /*! The L2CAP cid of the mirrored A2DP media channel.
        The cid is used internally when primary to determine if an A2DP media
        channel is connected. When set to L2CA_CID_INVALID there is no A2DP media
        channel connected. */
    l2ca_cid_t cid;

    /*! The L2CAP MTU. */
    uint16 mtu;

    /*! The primary earbud's active stream endpoint ID */
    uint8 seid;

    /*! The configured sample rate */
    uint32 sample_rate;

    /*! Content protection is enabled/disabled */
    bool content_protection;

    /* State of the A2DP sync */
    audio_sync_state_t state;

    uint8 q2q_mode;

} mirror_profile_a2dp_t;

/*! \brief Mirror profile peer link policy modes */
typedef enum
{
    /*! Mode for lowest power consumption */
    PEER_LP_IDLE,
    /*! Mode for A2DP active */
    PEER_LP_A2DP,
    /*! Mode for eSCO active */
    PEER_LP_ESCO
} peer_lp_mode_t;


/*! \brief Mirror Profile internal state. */
typedef struct
{
    /*! Mirror Profile task */
    TaskData task_data;

    /*! Mirror Profile state */
    mirror_profile_state_t state;

    /*! Mirror Profile target state */
    mirror_profile_state_t target_state;

    /*! Mirror Profile peer mode state */
    mirror_profile_peer_mode_state_t peer_mode_state;

    /*! Mirror Profile target peer mode state */
    mirror_profile_peer_mode_state_t target_peer_mode_state;

    /*! state machine lock */
    uint16 lock;

    /*! Lock set when starting a2dp mirroring. */
    uint16 a2dp_start_lock;

    /*! Current role of this instance */
    unsigned is_primary : 1;

    /*! Flag whether to delay before kicking the state machine after a state change. */
    unsigned delay_kick : 1;

    /*! Flag set to true if the peer link policy is idle (as opposed to active) */
    peer_lp_mode_t peer_lp_mode : 2;

    /*! Flag to Enable (TRUE) or disable (FALSE) ESCO mirroring, enabled by default */
    unsigned enable_esco_mirroring : 1;

    /*! Flag to Enable (TRUE) or disable (FALSE) A2DP mirroring, enabled by default */
    unsigned enable_a2dp_mirroring : 1;

    /*! Flag set when QHS is established between buds or failed to establish */
    unsigned buds_qhs_ready : 1;

    /*! When HFP eSCO starts on primary, if secondary is not connected, the
        mirror profile will immediately call Telephony_NotifyCallAudioConnected,
        and set this flag. Then when secondary later connects, the primary knows
        not to re-connect the voice source. If primary and secondary are connected
        when HFP eSCO starts, mirror profile clears this flag and defers calling
        Telephony_NotifyCallAudioConnected until mirroring is active. */
    unsigned hfp_voice_source_routed : 1;

    /*! The mirror ACL connection state */
    mirror_profile_acl_t acl;

    /*! The mirror eSCO connection state */
    mirror_profile_esco_t esco;

    /*! The mirror A2DP media connection state */
    mirror_profile_a2dp_t a2dp;

    /*! Init task to send init cfm to */
    Task init_task;

    /*! List of tasks registered for notifications from mirror_profile */
    task_list_t *client_tasks;

    /*! The interface to register with an AV instance that we want to sync to. */
    audio_sync_t    sync_if;

    /*! Audio sync context */
    mirror_profile_audio_sync_context_t audio_sync;
} mirror_profile_task_data_t;


extern mirror_profile_task_data_t mirror_profile;

#define MirrorProfile_Get() (&mirror_profile)

#define MirrorProfile_GetTask() (&MirrorProfile_Get()->task_data)

/*! \brief Get current mirror_profile state. */
#define MirrorProfile_GetState() (MirrorProfile_Get()->state)
/*! \brief Get current mirror_profile target state. */
#define MirrorProfile_GetTargetState() (MirrorProfile_Get()->target_state)

/*! \brief Get current mirror_profile peer mode state. */
#define MirrorProfilePeerMode_GetState() (MirrorProfile_Get()->peer_mode_state)
/*! \brief Set current mirror_profile peer mode state. */
#define MirrorProfilePeerMode_SetState(state) (MirrorProfile_Get()->peer_mode_state = (state))
/*! \brief Get target mirror_profile peer mode state. */
#define MirrorProfilePeerMode_GetTargetState() (MirrorProfile_Get()->target_peer_mode_state)
/*! \brief Set target mirror_profile peer mode state. */
#define MirrorProfilePeerMode_SetTargetState(state) (MirrorProfile_Get()->target_peer_mode_state = (state))


/*! \brief Is the mirror_profile in Primary role */
#define MirrorProfile_IsPrimary()   (MirrorProfile_Get()->is_primary)
/*! \brief Is the mirror_profile in Secondary role */
#define MirrorProfile_IsSecondary()   (!MirrorProfile_IsPrimary())
/*! \brief Get the module's audio sync interface */
#define MirrorProfile_GetSyncIf() (&(MirrorProfile_Get()->sync_if))
/*! \brief Get pointer to ACL state */
#define MirrorProfile_GetAclState() (&(MirrorProfile_Get()->acl))
/*! \brief Get pointer to A2DP state */
#define MirrorProfile_GetA2dpState() (&(MirrorProfile_Get()->a2dp))
/*! \brief Get pointer to eSCO state */
#define MirrorProfile_GetScoState() (&(MirrorProfile_Get()->esco))
/*! \brief Get pointer to L2CAP state */
#define MirrorProfile_GetAudioSyncL2capState() (&(MirrorProfile_Get()->audio_sync))

/*! \brief Set the delay kick flag */
#define MirrorProfile_SetDelayKick() MirrorProfile_Get()->delay_kick = TRUE
/*! \brief Clear the delay kick flag */
#define MirrorProfile_ClearDelayKick() MirrorProfile_Get()->delay_kick = FALSE
/*! \brief Get the delay kick flag */
#define MirrorProfile_GetDelayKick() (MirrorProfile_Get()->delay_kick)

/*!@{ \name Mirror profile lock bit masks */
#define MIRROR_PROFILE_TRANSITION_LOCK 1U
/*!@} */

/*! \brief Set mirror_profile lock bit for transition states */
#define MirrorProfile_SetTransitionLockBit() (mirror_profile.lock |= MIRROR_PROFILE_TRANSITION_LOCK)
/*! \brief Clear mirror_profile lock bit for transition states */
#define MirrorProfile_ClearTransitionLockBit() (mirror_profile.lock &= ~MIRROR_PROFILE_TRANSITION_LOCK)
/*! \brief Get mirror_profile state machine lock. */
#define MirrorProfile_GetLock() (mirror_profile.lock)

/*!@{ \name Mirror profile A2DP mirror start lock bit masks */
#define MIRROR_PROFILE_AUDIO_START_LOCK 1U
#define MIRROR_PROFILE_A2DP_MIRROR_START_LOCK 2U
/*!@} */
/*! \brief Get mirror_profile state machine lock. */
#define MirrorProfile_GetA2dpStartLockAddr() (&(mirror_profile.a2dp_start_lock))

/*! \brief Set audio start lock bit */
#define MirrorProfile_SetAudioStartLock() (mirror_profile.a2dp_start_lock |= MIRROR_PROFILE_AUDIO_START_LOCK)
/*! \brief Clear audio start lock bit */
#define MirrorProfile_ClearAudioStartLock() (mirror_profile.a2dp_start_lock &= ~MIRROR_PROFILE_AUDIO_START_LOCK)
/*! \brief Set a2dp mirror start lock bit */
#define MirrorProfile_SetA2dpMirrorStartLock() (mirror_profile.a2dp_start_lock |= MIRROR_PROFILE_A2DP_MIRROR_START_LOCK)
/*! \brief Clear a2dp mirror start lock bit */
#define MirrorProfile_ClearA2dpMirrorStartLock() (mirror_profile.a2dp_start_lock &= ~MIRROR_PROFILE_A2DP_MIRROR_START_LOCK)

/*! \brief Get a2dp mirror Q2Q mode */
#define MirrorProfile_IsQ2Q() (MirrorProfile_Get()->a2dp.q2q_mode)

/*! \brief Test if Mirror ACL connection handle is valid */
#define MirrorProfile_IsAclConnected() \
    (MirrorProfile_Get()->acl.conn_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)

/*! \brief Test if Mirror eSCO connection handle is valid */
#define MirrorProfile_IsEscoConnected() \
    (MirrorProfile_Get()->esco.conn_handle != MIRROR_PROFILE_CONNECTION_HANDLE_INVALID)

/*! \brief Test if Mirror A2DP is connected */
#define MirrorProfile_IsA2dpConnected() \
    MirrorProfile_IsSubStateA2dpConnected(MirrorProfile_GetState())

/*! \brief Test if Mirror Audio synchronisation L2CAP is connected */
#define MirrorProfile_IsAudioSyncL2capConnected() \
    (MirrorProfile_GetAudioSyncL2capState()->l2cap_state == MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED)

/*! \brief Is the mirror_profile ESCO mirroring enabled */
#define MirrorProfile_IsEscoMirroringEnabled()   (MirrorProfile_Get()->enable_esco_mirroring)

/*! \brief Is the mirror_profile A2DP mirroring enabled */
#define MirrorProfile_IsA2dpMirroringEnabled()   (MirrorProfile_Get()->enable_a2dp_mirroring)

/*! \brief Is the QHS connection between buds established */
#define MirrorProfile_IsQhsReady() (MirrorProfile_Get()->buds_qhs_ready)

/*! \brief Set QHS ready flag */
#define MirrorProfile_SetQhsReady() MirrorProfile_Get()->buds_qhs_ready = 1

/*! \brief Clear QHS ready flag */
#define MirrorProfile_ClearQhsReady() MirrorProfile_Get()->buds_qhs_ready = 0

/*! \brief Notify clients of the mirror ACL connection.

    Sends a MIRROR_PROFILE_CONNECT_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendAclConnectInd(void);

/*! \brief Notify clients of the mirror ACL dis-connection.

    Sends a MIRROR_PROFILE_DISCONNECT_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendAclDisconnectInd(void);

/*! \brief Notify clients of the mirror eSCO connection.

    Sends a MIRROR_PROFILE_ESCO_CONNECT_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendScoConnectInd(void);

/*! \brief Notify clients of the mirror eSCO dis-connection.

    Sends a MIRROR_PROFILE_ESCO_DISCONNECT_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendScoDisconnectInd(void);

/*! \brief Notify clients mirror A2DP stream is active.

    Sends a MIRROR_PROFILE_A2DP_STREAM_ACTIVE_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendA2dpStreamActiveInd(void);

/*! \brief Notify clients mirror A2DP stream is inactive .

    Sends a MIRROR_PROFILE_A2DP_STREAM_INACTIVE_IND to all clients that registered
    for notifications with #MirrorProfile_ClientRegister.
*/
void MirrorProfile_SendA2dpStreamInactiveInd(void);

/*! \brief Reset mirror SCO connection state */
void MirrorProfile_ResetEscoConnectionState(void);

/*! \brief Set the local SCO audio volume

    This is only expected to be called on the Secondary, in response to
    a forwarded volume update from the Primary.

    \param volume The HFP volume forwarded from the Primary.
*/
void MirrorProfile_SetScoVolume(uint8 volume);

/*! \brief Set the local SCO codec params

    This is only expected to be called on the Secondary, in response to
    a forwarded HFP codec from the Primary.

    \param code_mode The HFP codec mode forwarded from the Primary.
*/
void MirrorProfile_SetScoCodec(hfp_codec_mode_t codec_mode);

/*! \brief Inspect profile and internal state and decide the target state. */
void MirrorProfile_SetTargetStateFromProfileState(void);

/*! \brief Initialise the peer link policy.

    \note The default setting is active.
*/
void MirrorProfile_PeerLinkPolicyInit(void);

/*! \brief Set active A2DP mode peer link policy. */
void MirrorProfile_PeerLinkPolicySetA2dpActive(void);

/*! \brief Set active eSCO mode peer link policy. */
void MirrorProfile_PeerLinkPolicySetEscoActive(void);

/*! \brief Set idle mode peer link policy. */
void MirrorProfile_PeerLinkPolicySetIdle(void);

/*! \brief Handle MIRROR_INTERNAL_PEER_LINK_POLICY_IDLE_TIMEOUT.
    The link policy is set to a lower power consumption mode.
*/
void MirrorProfile_PeerLinkPolicyHandleIdleTimeout(void);

/*! \brief Gets the expected transmission time required, in microseonds, for peer link relay
*/
uint32 MirrorProfilePeerLinkPolicy_GetExpectedTransmissionTime(void);

/*! \brief Check if the given voice source can be mirrored to the peer

    The mirror profile does not support mirroring of SCO links or eSCO links
    with using the HV3 packet format.

    If the handset has connected to the primary with one of the unsupported
    hfp SCO link types the voice source should not be mirrored to the secondary.

    \param source The voice source to check.

    \return TRUE if it can be mirrored; FALSE otherwise.
*/
bool MirrorProfile_IsVoiceSourceSupported(voice_source_t source);

/*
    Test-only functions
*/

/* Destroy the mirror_profile instance for unit tests */
void MirrorProfile_Destroy(void);

#endif /* INCLUDE_MIRRORING */

#endif /* MIRROR_PROFILE_PRIVATE_H_ */
