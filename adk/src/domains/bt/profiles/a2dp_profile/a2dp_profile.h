/*!
\copyright  Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   a2dp_profile A2DP profile
\ingroup    profiles
\brief      A2DP profile
*/

#ifndef A2DP_PROFILE_H_
#define A2DP_PROFILE_H_

#include <a2dp.h>
#include "av_typedef.h"
#include "audio_sync.h"

/*! Note that disconnects are not shown for clarity.
@startuml
state A2DP_STATE_DISCONNECTED : No A2DP connection
state A2DP_STATE_CONNECTING_LOCAL : Locally initiated connection in progress
state A2DP_STATE_CONNECTING_REMOTE : Remotely initiated connection is progress
state   A2DP_STATE_CONNECTED_SIGNALLING : Signalling channel connected
state   A2DP_STATE_CONNECTING_MEDIA_LOCAL : Locally initiated media channel connection in progress
state   A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC : Remotely initiated media channel connection in progress synced with other instance

[*] --> A2DP_STATE_DISCONNECTED

A2DP_STATE_DISCONNECTED --> A2DP_STATE_CONNECTING_LOCAL : AV_INTERNAL_A2DP_CONNECT_REQ
A2DP_STATE_DISCONNECTED --> A2DP_STATE_CONNECTING_REMOTE : AV_INTERNAL_A2DP_SIGNALLING_CONNECT_IND/\nA2DP_SIGNALLING_CONNECT_IND

A2DP_STATE_CONNECTING_LOCAL --> A2DP_STATE_CONNECTED_SIGNALLING : A2DP_SIGNALLING_CONNECT_CFM
A2DP_STATE_CONNECTING_REMOTE --> A2DP_STATE_CONNECTED_SIGNALLING : A2DP_SIGNALLING_CONNECT_CFM

A2DP_STATE_CONNECTED_SIGNALLING -down-> A2DP_STATE_CONNECTING_MEDIA_LOCAL : AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ
A2DP_STATE_CONNECTED_SIGNALLING -down-> A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC : A2DP_MEDIA_OPEN_IND

state A2DP_STATE_CONNECTED_MEDIA {
    A2DP_STATE_CONNECTING_MEDIA_REMOTE_SYNC --> A2DP_STATE_CONNECTED_MEDIA_SUSPENDED : AV_INTERNAL_A2DP_INST_SYNC_RES&&\nA2DP_MEDIA_OPEN_CFM
    A2DP_STATE_CONNECTING_MEDIA_LOCAL --> A2DP_STATE_CONNECTED_MEDIA_SUSPENDED : A2DP_MEDIA_OPEN_CFM
    A2DP_STATE_CONNECTING_MEDIA_LOCAL --> A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC : A2DP_MEDIA_OPEN_CFM

    state A2DP_STATE_CONNECTED_MEDIA_STREAMING : Media channel streaming
    state A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL : Locally initiated media channel suspend in progress
    state A2DP_STATE_CONNECTED_MEDIA_SUSPENDED : Media channel suspended
    state A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC : Locally initiated media channel start in progress, syncing slave
    state A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC : Remotely initiated media channel start in progress, syncing slave

    A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC --> A2DP_STATE_CONNECTED_MEDIA_STREAMING : AV_INTERNAL_A2DP_INST_SYNC_RES&&\nA2DP_MEDIA_START_CFM
    A2DP_STATE_CONNECTED_MEDIA_STREAMING --> A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL : AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ
    A2DP_STATE_CONNECTED_MEDIA_STREAMING --> A2DP_STATE_CONNECTED_MEDIA_SUSPENDED : A2DP_MEDIA_SUSPEND_IND
    A2DP_STATE_CONNECTED_MEDIA_SUSPENDING_LOCAL --> A2DP_STATE_CONNECTED_MEDIA_SUSPENDED : A2DP_MEDIA_SUSPEND_CFM
    A2DP_STATE_CONNECTED_MEDIA_SUSPENDED --> A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC : AV_INTERNAL_A2DP_RESUME_MEDIA_REQ
    A2DP_STATE_CONNECTED_MEDIA_SUSPENDED --> A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC : A2DP_MEDIA_START_IND
    A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC --> A2DP_STATE_CONNECTED_MEDIA_STREAMING : AV_INTERNAL_A2DP_INST_SYNC_RES&&\nA2DP_MEDIA_START_CFM
}
@enduml
*/

/*! \brief Check if SEID is for non-TWS CODEC */
#define appA2dpIsSeidNonTwsSink(seid) \
    (((seid) >= AV_SEID_SBC_SNK) && \
     ((seid) <= AV_SEID_APTX_ADAPTIVE_SNK))

/*! \brief Check if SEID is for TWS Sink CODEC */
#define appA2dpIsSeidTwsSink(seid) \
    (((seid) >= AV_SEID_SBC_MONO_TWS_SNK) && \
     ((seid) <= AV_SEID_APTX_ADAPTIVE_TWS_SNK))

/*! \brief Check if SEID is for non-TWS Source CODEC */
#define appA2dpIsSeidNonTwsSource(seid) \
    ((seid) == AV_SEID_SBC_SRC)

/*! \brief Check if SEID is for TWS Source CODEC */
#define appA2dpIsSeidTwsSource(seid) \
    (((seid) >= AV_SEID_SBC_MONO_TWS_SRC) && \
     ((seid) <= AV_SEID_APTX_ADAPTIVE_TWS_SRC))

/*! \brief Check if SEID is for TWS CODEC */
#define appA2dpIsSeidTws(seid) \
    (appA2dpIsSeidTwsSink(seid) || appA2dpIsSeidTwsSource(seid))

/*! \brief Check if SEID is for Sink */
#define appA2dpIsSeidSink(seid) \
    (appA2dpIsSeidTwsSink(seid) || appA2dpIsSeidNonTwsSink(seid))

/*! \brief Check if SEID is for Source */
#define appA2dpIsSeidSource(seid) \
    (appA2dpIsSeidTwsSource(seid) || appA2dpIsSeidNonTwsSource(seid))

/*! \brief Check this instance is a non-TWS Sink */
#define appA2dpIsSinkNonTwsCodec(theInst) \
    (appA2dpIsSeidNonTwsSink(theInst->a2dp.current_seid))

/*! \brief Check this instance is a TWS Sink */
#define appA2dpIsSinkTwsCodec(theInst) \
    (appA2dpIsSeidTwsSink(theInst->a2dp.current_seid))

/*! \brief Check this instance is a Sink */
#define appA2dpIsSinkCodec(theInst) \
    (appA2dpIsSeidSink(theInst->a2dp.current_seid))

/*! \brief Check this instance is a Source */
#define appA2dpIsSourceCodec(theInst) \
    (appA2dpIsSeidSource(theInst->a2dp.current_seid))

/*! \brief Check this instance is a TWS Source or Sink */
#define appA2dpIsTwsCodec(theInst) \
    (appA2dpIsSeidTws(theInst->a2dp.current_seid))

/*! \brief Get A2DP lock */
#define appA2dpGetLock(theInst) ((theInst)->a2dp.lock)

/*!@{ \name A2DP lock bit masks */
#define APP_A2DP_TRANSITION_LOCK    1U
#define APP_A2DP_AUDIO_START_LOCK   2U
#define APP_A2DP_AUDIO_STOP_LOCK    4U
/*!@} */

/*! \brief Set A2DP lock bit for transition states */
#define appA2dpSetTransitionLockBit(theInst) (theInst)->a2dp.lock |= APP_A2DP_TRANSITION_LOCK
/*! \brief Clear A2DP lock bit for transition states */
#define appA2dpClearTransitionLockBit(theInst) (theInst)->a2dp.lock &= ~APP_A2DP_TRANSITION_LOCK
/*! \brief Set A2DP start lock bit waiting for kymera operations  */
#define appA2dpSetAudioStartLockBit(theInst) (theInst)->a2dp.lock |= APP_A2DP_AUDIO_START_LOCK
/*! \brief Clear A2DP start lock bit waiting for kymera operations  */
#define appA2dpClearAudioStartLockBit(theInst) (theInst)->a2dp.lock &= ~APP_A2DP_AUDIO_START_LOCK
/*! \brief Set A2DP stop lock bit waiting for kymera operations  */
#define appA2dpSetAudioStopLockBit(theInst) (theInst)->a2dp.lock |= APP_A2DP_AUDIO_STOP_LOCK
/*! \brief Clear A2DP lock bit waiting for kymera operations  */
#define appA2dpClearAudioStopLockBit(theInst) (theInst)->a2dp.lock &= ~APP_A2DP_AUDIO_STOP_LOCK
/*! \brief Check if a A2DP lock bit is set */
#define appA2dpCheckLockMaskIsSet(theInst, lock_mask) \
    (((appA2dpGetLock(theInst)) & (lock_mask)) == (lock_mask))

/*!@{ \name Masks to use to check for the current state meeting some conditions */
#define A2DP_STATE_MASK_CONNECTED_SIGNALLING            (A2DP_STATE_CONNECTED_SIGNALLING)
#define A2DP_STATE_MASK_CONNECTED_MEDIA                 (A2DP_STATE_CONNECTED_MEDIA)
#define A2DP_STATE_MASK_CONNECTED_MEDIA_STREAMING       (A2DP_STATE_CONNECTED_MEDIA | 0x0F)
/*!@} */


/*! \brief Is A2DP state 'connected signalling' */
#define appA2dpIsStateConnectedSignalling(a2dp_state) \
    (((a2dp_state) & A2DP_STATE_MASK_CONNECTED_SIGNALLING) == A2DP_STATE_CONNECTED_SIGNALLING)

/*! \brief Is A2DP state 'connected media' */
#define appA2dpIsStateConnectedMedia(a2dp_state) \
    (((a2dp_state) & A2DP_STATE_MASK_CONNECTED_MEDIA) == A2DP_STATE_CONNECTED_MEDIA)

/*! \brief Is A2DP state 'connected media streaming' */
#define appA2dpIsStateConnectedMediaStreaming(a2dp_state) \
    ((a2dp_state) == A2DP_STATE_CONNECTED_MEDIA_STREAMING)

/*! \brief Is kymera started in this current state.
    \todo This could be encoded in the state vector. */
#define appA2dpIsKymeraOnInState(a2dp_state) \
    (((a2dp_state) == A2DP_STATE_CONNECTED_MEDIA_STARTING_LOCAL_SYNC) || \
     ((a2dp_state) == A2DP_STATE_CONNECTED_MEDIA_STARTING_REMOTE_SYNC) || \
     ((a2dp_state) == A2DP_STATE_CONNECTED_MEDIA_STREAMING))

/*! \brief Is A2DP instance media channel connected */
#define appA2dpIsConnectedMedia(theInst) \
    appA2dpIsStateConnectedMedia((theInst)->a2dp.state)

/*! \brief Is A2DP instance streaming */
#define appA2dpIsStreaming(theInst) \
    appA2dpIsStateConnectedMediaStreaming((theInst)->a2dp.state)

/*! \brief Is A2DP instance disconnected */
#define appA2dpIsDisconnected(theInst) \
    ((theInst)->a2dp.state == A2DP_STATE_DISCONNECTED)

/*! \brief Quick check for whether a2dp is connected on the specified AV instance */
#define appA2dpIsConnected(theInst) \
    (((theInst)->a2dp.state & A2DP_STATE_MASK_CONNECTED_SIGNALLING) == A2DP_STATE_CONNECTED_SIGNALLING)

/*! \brief Check if the flag to connect media after signalling is set for an AV instance. */
#define appA2dpIsConnectMediaFlagSet(theInst) \
    (theInst->a2dp.bitfields.flags & A2DP_CONNECT_MEDIA)

extern void appA2dpInstanceInit(avInstanceTaskData *theAv, uint8 suspend_state);
extern void appA2dpSignallingConnectIndicationNew(avTaskData *theAv, const A2DP_SIGNALLING_CONNECT_IND_T *ind);
extern void appA2dpRejectA2dpSignallingConnectIndicationNew(avTaskData *theAv, const A2DP_SIGNALLING_CONNECT_IND_T *ind);
extern void appA2dpVolumeSet(avInstanceTaskData *theAv, uint16 volume);
extern avA2dpState appA2dpGetState(avInstanceTaskData *theAv);
extern void appA2dpInstanceHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message);
extern uint8 appA2dpConvertSeidFromSinkToSource(uint8 seid);
extern bool appA2dpSeidsAreCompatible(const avInstanceTaskData *inst1, const avInstanceTaskData *inst2);

/*! \brief Handle A2DP error

    Some error occurred in the A2DP state machine.

    To avoid the state machine getting stuck, if instance is connected then
    drop connection and move to 'disconnecting' state.
*/
void appA2dpError(avInstanceTaskData *theInst, MessageId id, Message message);

uint8 A2dpProfile_GetDefaultVolume(void);

/*! \brief Returns a2dpTaskData of the handset.

    \return handset a2dpTaskData.
*/
a2dpTaskData * A2dpProfile_GetHandsetData(void);

/*! \brief Returns a2dpTaskData of the peer device.

    \return peer device a2dpTaskData.
*/
a2dpTaskData * A2dpProfile_GetPeerData(void);

#ifdef INCLUDE_MIRRORING
/*! 
    \brief Handle Veto check during handover
    \param[in] the_inst     AV instance refernce \ref avInstanceTaskData
    \return bool
*/
bool A2dpProfile_Veto(avInstanceTaskData *the_inst);

/*!
    \brief Component commits to the specified role

    The component should take any actions necessary to commit to the
    new role.

    \param[in] the_inst     AV instance.
    \param[in] is_primary   TRUE if device role is primary, else secondary

*/
void A2dpProfile_Commit(avInstanceTaskData *the_inst, bool is_primary);
#endif /* INCLUDE_MIRRORING */

/*! \brief Enable/disable PTS mode.

    \param pts_mode_enabled If TRUE PTS mode will be enabled.
*/
void A2dpProfile_SetPtsMode(bool pts_mode_enabled);

/*! \brief Check if PTS mode is enabled.

    \return TRUE if PTS mode is enabled.
*/
bool A2dpProfile_IsPtsMode(void);

#endif /* A2DP_PROFILE_H_ */
