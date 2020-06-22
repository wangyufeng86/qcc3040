/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       mirror_profile_peer_audio_sync_l2cap.h
\defgroup   mirror_profile Mirror Profile
\ingroup    profiles
\brief      Mirror Profile L2cap Channel creation for audio synchronisation
*/

#ifndef MIRROR_PROFILE_PEER_AUDIO_SYNC_L2CAP_H_
#define MIRROR_PROFILE_PEER_AUDIO_SYNC_L2CAP_H_

#include <sink.h>
#include <source.h>
#include <connection_manager.h>

#ifdef INCLUDE_MIRRORING

#include "mirror_profile.h"

/*! L2cap flush timeout(in us) for Audio Sync Channel */
#define MIRROR_PROFILE_AUDIO_L2CAP_FLUSH_TIMEOUT (100000u)

/*! Mirror Profile L2cap MTU Size */
#define MIRROR_PROFILE_L2CAP_MTU_SIZE            (672)

/*! Maximum number of times to try the SDP search.After this many attempts the 
    connection request will be failed. */
#define MirrorProfile_GetSdpSearchTryLimit()     (3)

/*! L2cap internal state */
typedef enum
{
     /*! L2cap channel does not exist for Audio Synchronisation */
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_NONE,

     /*! SDP Search for establishing L2cap connection */
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_SDP_SEARCH,

     /*! L2cap connection issued to secondary for Audio Synchronisation */
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_LOCAL_CONNECTING,

     /*! L2cap connection request from primary for Audio Synchronisation */
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_REMOTE_CONNECTING,

     /*! L2cap connection to be shutdown  */
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_DISCONNECTING,

     /*! L2cap connection for Audio Sync Established */ 
     MIRROR_PROFILE_STATE_AUDIO_SYNC_L2CAP_CONNECTED
} mirror_profile_audio_sync_l2cap_state_t;

/*! \brief Audio Sync L2cap context information */
typedef struct
{
    /*! Audio Sync L2cap internal state */
    mirror_profile_audio_sync_l2cap_state_t l2cap_state;

    /*! Number of SDP search attempts */
    uint8 sdp_search_attempts;

    /*! L2CAP PSM registered */
    uint16 local_psm;
    
    /*! L2CAP PSM registered by peer device */
    uint16 remote_psm;

    /*! Bluetooth address of the peer */
    bdaddr peer_addr;

    /*! Store the Task which requested a connect. */
    Task connect_task;
    
    /*! Store the Task which requested a disconnect. */
    Task disconnect_task;

     /*! The sink of the L2CAP link */
    Sink link_sink;
     
    /*! The source of the L2CAP link */
    Source link_source;

     /*! TRUE if link-loss has occurred */
    bool link_loss_occured;
} mirror_profile_audio_sync_context_t;

/*! \brief Call this function to shutdown the mirror profile connection with peer.

    \param The task to which a disconnect confirmation message shall be sent.

    This can be called on both the Primary and Secondary device.
*/
void MirrorProfile_CloseAudioSyncL2capChannel(Task task);

/*! \brief Creates L2cap channel for audio synchronisation with the peer.

    \param The task to which a connect confirmation message shall be sent.
    \param peer_addr the peer's bdaddr.

    This must be called only on Primary device.
*/
void MirrorProfile_CreateAudioSyncL2capChannel(Task task, const bdaddr *peer_addr);

/*! \brief Handles the service search attribute confirmation message.

    This must be called only on Primary device.

    \param cfm Service search attribute confirmation message.
*/
void MirrorProfile_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm);

/*! \brief Handles the L2cap Connect Confirmation message.

    This can be called on both the Primary and Secondary device

    \param cfm L2cap connect confirmation message.
*/
void MirrorProfile_HandleL2capConnectCfm(const CL_L2CAP_CONNECT_CFM_T *cfm);

/*! \brief Handles the Mirror Profile L2cap Connect request sent from Primary.

    This can be called on only Secondary device.

    \param cfm L2cap connect indication message.
*/
void MirrorProfile_HandleL2capConnectInd(const CL_L2CAP_CONNECT_IND_T *ind);

/*! \brief Handles the Mirror Profile L2cap Disconnect indication.

    \param cfm L2cap disconnect indication message.
*/
void MirrorProfile_HandleL2capDisconnectInd(const CL_L2CAP_DISCONNECT_IND_T *ind);

/*! \brief Handles the Mirror Profile L2cap Disconnect confirmation.

    \param cfm L2cap disconnect confirmation message.
*/
void MirrorProfile_HandleL2capDisconnectCfm(const CL_L2CAP_DISCONNECT_CFM_T *cfm);

/*! \brief Handle mirror_profile L2cap register confirmation.

    Handles L2cap register confirmation message for Audio synchronisation
    between peers.

    \param cfm L2cap registration confirmation message.
*/
void MirrorProfile_HandleClL2capRegisterCfm(const CL_L2CAP_REGISTER_CFM_T *cfm);

/*! \brief Handle mirror_profile SDP register confirmation. 

     \param cfm SDP registration confirmation message.
*/
void MirrorProfile_HandleClSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *message);

#endif /* INCLUDE_MIRRORING */

#endif /* MIRROR_PROFILE_PEER_AUDIO_SYNC_L2CAP_H_ */
