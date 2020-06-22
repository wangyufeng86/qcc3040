/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       a2dp_profile_sync.h
\brief      Implementation of an audio_sync_t interface for an a2dp AV instance.
*/

#ifndef A2DP_PROFILE_SYNC_H_
#define A2DP_PROFILE_SYNC_H_


#include "audio_sync.h"
#include "av_typedef.h"


/*! \brief Initialise the audio_sync_t interface instance for an AV instance.

    Initialises the audio_sync_t interface returned by
    #appA2dpSyncGetAudioSyncInterface for the given AV instance.

    \param theInst  The AV instance to initialise the audio_sync_t interface for.
*/
void appA2dpSyncInitialise(avInstanceTaskData *theInst);

/*! \brief Get the audio_sync_t interface to synchronise with this AV instance.

    \param theInst  The AV instance to get the audio_sync_t interface for.

    \return         Pointer to the audio_sync_t interface for this AV instance,
                    or NULL if it is not available.
*/
struct audio_sync_t *appA2dpSyncGetAudioSyncInterface(avInstanceTaskData *theInst);

/*! \brief Register an AV to synchronise with.

    After the instance is registered the current state, based on
    #audio_sync_state_t, is sent to #theSyncInst.

    \param theInst      The AV instance to register #theSyncInst with.
    \param theSyncInst  The AV instance that should be synchronised to #theInst.
*/
void appA2dpSyncRegisterInstance(avInstanceTaskData *theInst, avInstanceTaskData *theSyncInst);

/*! \brief Un-register the #audio_sync_t registered to an AV instance.

    Will also cancel any #audio_sync_msg_t messages sent to the registered
    #audio_sync_t instance but not delivered yet.

    \param theInst      The AV instance to remove any registered #audio_sync_t from.
*/
void appA2dpSyncUnregisterInstance(avInstanceTaskData *theInst);

/*! \brief Handler function for audio_sync_msg_t messages sent to an AV instance.

    \param theInst      The AV instance to handle the audio_sync_msg_t message.
    \param id           The message Id.
    \param message      The message payload, or NULL if there is no payload.
*/
void appA2dpSyncHandleMessage(avInstanceTaskData *theInst, MessageId id, Message message);

/*! \brief Register a sync interface to synchronise.

    After the instance is registered the current state, based on
    #audio_sync_state_t, is sent to #sync_if.

    \param theInst      The AV instance to register #theSyncInst with.
    \param sync_if      The interface that should be synchronised to #theInst.
*/
void appA2dpSyncRegister(avInstanceTaskData *theInst, audio_sync_t *sync_if);

/*! \brief Un-register a sync interface from synchronisation.

    Will also cancel any #audio_sync_msg_t messages sent to the registered
    #audio_sync_t instance but not delivered yet.

    \param theInst      The AV instance to unregister from.
    \param sync_if      The interface to unregister.
*/
void appA2dpSyncUnregister(avInstanceTaskData *theInst, audio_sync_t *sync_if);

#endif /* A2DP_PROFILE_SYNC_H_ */
