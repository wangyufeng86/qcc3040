/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   telephony_messages Telephony Messages
\ingroup    domains
\brief      Functions for generating Telephony notification messages
*/

#ifndef TELEPHONY_MESSAGES_H_
#define TELEPHONY_MESSAGES_H_

#include <message.h>

#include "domain_message.h"
#include "voice_sources_list.h"

/*!\{*/

/*! Messages sent by the voice call domain to interested clients.
*/
enum telephony_domain_messages
{
    TELEPHONY_AUDIO_CONNECTED = TELEPHONY_MESSAGE_BASE,
    TELEPHONY_AUDIO_DISCONNECTED,

    TELEPHONY_CONNECTED,
    TELEPHONY_INCOMING_CALL,
    TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE,
    TELEPHONY_INCOMING_CALL_ENDED,

    TELEPHONY_CALL_ANSWERED,
    TELEPHONY_CALL_REJECTED,
    TELEPHONY_CALL_ONGOING,
    TELEPHONY_CALL_ENDED,
    TELEPHONY_CALL_HUNG_UP,
    TELEPHONY_UNENCRYPTED_CALL_STARTED,
    TELEPHONY_CALL_CONNECTION_FAILURE,
    TELEPHONY_LINK_LOSS_OCCURRED,
    TELEPHONY_DISCONNECTED,
    TELEPHONY_CALL_AUDIO_RENDERED_LOCAL,
    TELEPHONY_CALL_AUDIO_RENDERED_REMOTE,
    TELEPHONY_ERROR,
    TELEPHONY_MUTE_ACTIVE,
    TELEPHONY_MUTE_INACTIVE,
    TELEPHONY_TRANSFERED,
    TELEPHONY_NUMBER_DIAL_INITIATED
};

typedef struct
{
    voice_source_t voice_source;
} telephony_message_t;

typedef telephony_message_t TELEPHONY_AUDIO_CONNECTED_T,
    TELEPHONY_AUDIO_DISCONNECTED_T,
    TELEPHONY_CONNECTED_T,
    TELEPHONY_DISCONNECTED_T,
    TELEPHONY_LINK_LOSS_OCCURRED_T,
    TELEPHONY_INCOMING_CALL_T,
    TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE_T,
    TELEPHONY_INCOMING_CALL_ENDED_T,
    TELEPHONY_CALL_ONGOING_T,
    TELEPHONY_CALL_ENDED_T,
    TELEPHONY_UNENCRYPTED_CALL_STARTED_T,
    TELEPHONY_CALL_CONNECTION_FAILURE_T,
    TELEPHONY_CALL_AUDIO_RENDERED_LOCAL_T,
    TELEPHONY_CALL_AUDIO_RENDERED_REMOTE_T,
    TELEPHONY_CALL_ANSWERED_T,
    TELEPHONY_CALL_REJECTED_T,
    TELEPHONY_CALL_HUNG_UP_T,
    TELEPHONY_NUMBER_DIAL_INITIATED_T,
    TELEPHONY_TRANSFERED_T,
    TELEPHONY_MUTE_ACTIVE_T,
    TELEPHONY_MUTE_INACTIVE_T,
    TELEPHONY_ERROR_T;


/*! \brief Setup the telephony message delivery framework.

    \param init_task Not used.

    \return TRUE
 */
bool Telephony_InitMessages(Task init_task);

/*! \brief Register task to receive voice call messages.

    \param task_to_register
 */
void Telephony_RegisterForMessages(Task task_to_register);

/*! \brief Send a telephony notification message.

    \param the notification message id
    \param source The voice source of which call audio has become connected
 */
void Telephony_NotifyMessage(MessageId id, voice_source_t source);

/*! \brief Send a call audio connected notification.

    \param source The voice source of which call audio has become connected
 */
#define Telephony_NotifyCallAudioConnected(source) \
    Telephony_NotifyMessage(TELEPHONY_AUDIO_CONNECTED, source)

/*! \brief Send a call audio disconnected notification.

    \param source The voice source of which call audio has become disconnected
 */
#define Telephony_NotifyCallAudioDisconnected(source) \
    Telephony_NotifyMessage(TELEPHONY_AUDIO_DISCONNECTED, source)

/*! \brief Send a connected notification.

    \param source The voice source connected
 */
#define Telephony_NotifyConnected(source) \
    Telephony_NotifyMessage(TELEPHONY_CONNECTED, source)

/*! \brief Send a disconnected notification.

    \param source The voice source disconnected
 */
#define Telephony_NotifyDisconnected(source) \
    Telephony_NotifyMessage(TELEPHONY_DISCONNECTED, source)

/*! \brief Send a disconnected due to linkloss notification.

    \param source The voice source disconnected due to linkloss
 */
#define Telephony_NotifyDisconnectedDueToLinkloss(source) \
    Telephony_NotifyMessage(TELEPHONY_LINK_LOSS_OCCURRED, source)

/*! \brief Notify of an incoming call.

    \param source The voice source associated with the incoming call
 */
#define Telephony_NotifyCallIncoming(source) \
    Telephony_NotifyMessage(TELEPHONY_INCOMING_CALL, source)

/*! \brief Notify of an incoming call with out of band ringtone.

    \param source The voice source associated with the incoming call
 */
#define Telephony_NotifyCallIncomingOutOfBandRingtone(source) \
    Telephony_NotifyMessage(TELEPHONY_INCOMING_CALL_OUT_OF_BAND_RINGTONE, source)

/*! \brief Notify of an incoming call ending.

    \param source The voice source associated with the ended call
 */
#define Telephony_NotifyCallIncomingEnded(source) \
    Telephony_NotifyMessage(TELEPHONY_INCOMING_CALL_ENDED, source)

/*! \brief Notify of an active call.

    \param source The voice source associated with the active call
 */
#define Telephony_NotifyCallActive(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_ONGOING, source)

/*! \brief Notify of an active call ending.

    \param source The voice source associated with the ended call
 */
#define Telephony_NotifyCallEnded(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_ENDED, source)

/*! \brief Notify of an active call becoming unencrypted.

    \param source The voice source associated with the active call
 */
#define Telephony_NotifyCallBecameUnencrypted(source) \
    Telephony_NotifyMessage(TELEPHONY_UNENCRYPTED_CALL_STARTED, source)

/*! \brief Notify of an incoming call failing to connect.

    \param source The voice source associated with the incoming call
 */
#define Telephony_NotifyCallConnectFailure(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_CONNECTION_FAILURE, source)

/*! \brief Notify of call audio being rendered on the local device.

    \param source The voice source associated with the ongoing call
 */
#define Telephony_NotifyCallAudioRenderedLocal(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_AUDIO_RENDERED_LOCAL, source)

/*! \brief Notify of call audio being rendered on the remote device.

    \param source The voice source associated with the ongoing call
 */
#define Telephony_NotifyCallAudioRenderedRemote(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_AUDIO_RENDERED_REMOTE, source)

/*! \brief Notify of call being answered.

    \param source The voice source associated with the answered call
 */
#define Telephony_NotifyCallAnswered(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_ANSWERED, source)

/*! \brief Notify of a call being rejected.

    \param source The voice source associated with the rejected call
 */
#define Telephony_NotifyCallRejected(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_REJECTED, source)

/*! \brief Notify of a call being terminated.

    \param source The voice source associated with the terminated call
 */
#define Telephony_NotifyCallTerminated(source) \
    Telephony_NotifyMessage(TELEPHONY_CALL_HUNG_UP, source)

/*! \brief Notify of a call being initiated with a given number.

    \param source The voice source associated with the initiated call
 */
#define Telephony_NotifyCallInitiatedUsingNumber(source) \
    Telephony_NotifyMessage(TELEPHONY_NUMBER_DIAL_INITIATED, source)

/*! \brief Notify of call audio being transferred to/from remote device.

    \param source The voice source associated with the incoming call
 */
#define Telephony_NotifyCallAudioTransferred(source) \
    Telephony_NotifyMessage(TELEPHONY_TRANSFERED, source)

/*! \brief Notify of microphone being muted.

    \param source The voice source associated with the muted microphone
 */
#define Telephony_NotifyMicrophoneMuted(source) \
    Telephony_NotifyMessage(TELEPHONY_MUTE_ACTIVE, source)

/*! \brief Notify of microphone being unmuted.

    \param source The voice source associated with the unmuted microphone
 */
#define Telephony_NotifyMicrophoneUnmuted(source) \
    Telephony_NotifyMessage(TELEPHONY_MUTE_INACTIVE, source)

/*! \brief Notify of a telephony error.

    \param source The voice source associated with the error
 */
#define Telephony_NotifyError(source) \
    Telephony_NotifyMessage(TELEPHONY_ERROR, source)

/*!\}*/

#endif /* TELEPHONY_MESSAGES_H_ */
