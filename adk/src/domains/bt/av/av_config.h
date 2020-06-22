/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Configuration related definitions for AV state machines.
*/

#ifndef AV_CONFIG_H_
#define AV_CONFIG_H_

/*! Number of volume steps to use per AV UI volume event.
 The full volume range is 0-127 */
#define avConfigGetAvVolumeStep()  (8)

/*! Time to wait to connect AVRCP after a remotely initiated A2DP connection
    indication if the remote device does not initiate a AVRCP connection */
#define appConfigAvrcpConnectDelayAfterRemoteA2dpConnectMs() D_SEC(3)

/*! This config controls if aptX A2DP support is enabled.  A valid license is also
 * required for aptX to enable this CODEC */
#ifdef INCLUDE_STEREO
#define appConfigAptxEnabled() FALSE
#else
#define appConfigAptxEnabled() TRUE
#endif

/*! This config controls if AAC A2DP support is enabled */
#ifdef INCLUDE_STEREO
#define appConfigAacEnabled() FALSE
#else
#define appConfigAacEnabled() TRUE
#endif

/*! When the earbuds handover when A2DP audio is streaming, the new master earbud
    sends an AVRCP media play command to the handset when both AVRCP and A2DP media
    are connected. Some handsets may emit a burst of audio from their local speaker
    if the play command is sent to the handset too soon. To avoid this audio burst,
    the play command is delayed. */
#define appConfigHandoverMediaPlayDelay() D_SEC(1)

/* APTX adaptive is not supported along with stereo configuration */
#if (defined INCLUDE_APTX_ADAPTIVE) && (!defined INCLUDE_STEREO)
#define appConfigAptxAdaptiveEnabled() TRUE
#else
#define appConfigAptxAdaptiveEnabled() FALSE
#endif

#ifndef INCLUDE_MIRRORING
#define avConfigA2dpSourceEnabled() TRUE
#else
#define avConfigA2dpSourceEnabled() FALSE
#endif

#endif /* AV_CONFIG_H_ */
