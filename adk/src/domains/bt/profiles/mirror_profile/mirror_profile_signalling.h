/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Mirror profile channel for sending messages between Primary & Secondary.
*/

#ifndef MIRROR_PROFILE_CHANNEL_H_
#define MIRROR_PROFILE_CHANNEL_H_

#include "kymera_adaptation_voice_protected.h"
#include "peer_signalling.h"


/*! \brief Send current HFP volume to the Secondary.

    This is called by the Primary to forward on any change to the local HFP
    volume to the Secondary.

    \param volume HFP volume to forward on.
*/
void MirrorProfile_SendHfpVolumeToSecondary(uint8 volume);

/*! \brief Send current HFP codec to the Secondary.

    This is called by the Primary to forward the local HFP codec_mode
    to the Secondary.

    \param codec_code HFP codec_mode to forward on.
    \param volume HFP volume to forward on.
*/
void MirrorProfile_SendHfpCodecAndVolumeToSecondary(hfp_codec_mode_t codec_mode, uint8 volume);

/*! \brief Send current A2DP volume to the Secondary.

    This is called by the Primary to forward on any change to the local A2DP
    volume to the Secondary.

    \param volume A2DP volume to forward on.
*/
void MirrorProfile_SendA2dpVolumeToSecondary(uint8 volume);

/*! \brief Send stored A2DP media stream context to the Secondary.

    \note If peer signalling is not connected the context is not sent to the
    secondary.
*/
void MirrorProfile_SendA2dpStreamContextToSecondary(void);

/*! \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND

    Both Primary and Secondary may receive this when the other peer has sent a
    message to it.
*/
void MirrorProfile_HandlePeerSignallingMessage(const PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T *ind);

/*! \brief Handle PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM

    Both Primary and Secondary will receive this to confirm a sent message was
    acknowledged by the other peer.

    This does not do handle errors at the moment; it is for information only.
*/
void MirrorProfile_HandlePeerSignallingMessageTxConfirm(const PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T *cfm);

#endif /* MIRROR_PROFILE_CHANNEL_H_ */
