/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       audio_sync.h
\brief      API to synchronise an audio sink with and audio source.

Example of the API in use:
@startuml

box "earbuds application" #Orange
    participant earbud
end box

box "services" #MediumPurple
    participant peer
end box

box "domains" #LightBlue
    participant a2dp_sink
    participant a2dp_source
    participant audio_sync
end box

box "libraries" #LightGreen
    participant a2dp
end box

note over peer
    peer service is responsible for registering the audio_sink
    with the audio_source, but it does not handle the
    synchronisation logic.
end note

note over peer
    In the current earbud app the av module takes the role of
    the peer service.
end note

a2dp -->> a2dp_sink : A2DP_SIGNALLING_CONNECT_CFM
activate a2dp_sink
note over a2dp_sink : Handset connection; peer is already connected.
a2dp_sink -> peer : appAvInstanceA2dpConnected
peer -> a2dp_sink : appA2dpSyncEnable(handset_inst)
a2dp_sink -> a2dp_sink : appA2dpRegisterSyncTask(handset_inst, peer_inst)
a2dp_sink -->> a2dp_sink : AV_INTERNAL_A2DP_SYNC_STATE_REQ
a2dp_sink -> audio_sync : audioSync_StateIndication(DISCONNECTED)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(DISCONNECTED)
note over a2dp_source: Nothing else to do
deactivate a2dp_sink

note over a2dp_sink
    Peer instance will now receive synchronisation
    messages from the handset instance
end note

a2dp -->> a2dp_sink : A2DP_MEDIA_OPEN_IND
activate a2dp_sink
a2dp_sink -> audio_sync : audioSync_ConnectIndication
audio_sync -->> a2dp_source : AUDIO_SYNC_CONNECT_IND
deactivate a2dp_sink

activate a2dp_source
a2dp_source -->> a2dp_source : AV_INTERNAL_A2DP_CONNECT_MEDIA_REQ
a2dp_source -> a2dp : A2dpMediaOpenRequest
deactivate a2dp_source

a2dp -->> a2dp_source : A2DP_MEDIA_OPEN_CFM
activate a2dp_source
a2dp_source -->> a2dp_sink : AUDIO_SYNC_CONNECT_RES
deactivate a2dp_source

activate a2dp_sink
a2dp_sink -> a2dp : A2dpMediaOpenResponse
a2dp_sink -> audio_sync : audioSync_StateIndication(CONNECTED)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(CONNECTED)
note over a2dp_source: Nothing else to do
deactivate a2dp_sink

note over a2dp
    handset starts streaming
end note

a2dp -->> a2dp_sink : A2DP_MEDIA_START_IND
activate a2dp_sink
a2dp_sink -> audio_sync : audioSync_ActivateIndication
audio_sync -->> a2dp_source : AUDIO_SYNC_ACTIVATE_IND
deactivate a2dp_sink

activate a2dp_source
a2dp_source -->> a2dp_source : AV_INTERNAL_A2DP_RESUME_MEDIA_REQ
a2dp_source -> a2dp : A2dpMediaStartRequest
deactivate a2dp_source

a2dp -->> a2dp_source : A2DP_MEDIA_START_CFM
activate a2dp_source
a2dp_source -->> a2dp_sink : AUDIO_SYNC_ACTIVATE_RES
deactivate a2dp_source

activate a2dp_sink
a2dp_sink -> a2dp : A2dpMediaStartResponse
a2dp_sink -> audio_sync : audioSync_StateIndication(ACTIVE)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(ACTIVE)
note over a2dp_source: Nothing else to do
deactivate a2dp_sink

note over a2dp
    handset stops streaming
end note

a2dp -->> a2dp_sink : A2DP_MEDIA_SUSPEND_IND
activate a2dp_sink
a2dp_sink -> audio_sync : audioSync_StateIndication(CONNECTED)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(CONNECTED)
note over a2dp_source
    Does not need a reply but need
    to suspend peer media channel
end note
deactivate a2dp_sink

activate a2dp_source
a2dp_source -->> a2dp_source : AV_INTERNAL_A2DP_SUSPEND_MEDIA_REQ
a2dp_source -> a2dp : A2dpMediaSuspendRequest
deactivate a2dp_source

a2dp -->> a2dp_source : A2DP_MEDIA_SUSPEND_CFM

note over a2dp
    handset re-negotiates the codec
end note

a2dp -->> a2dp_sink : A2DP_MEDIA_RECONFIGURE_IND
activate a2dp_sink
a2dp_sink -> audio_sync : audioSync_CodecReconfiguredIndication(codec)
audio_sync -->> a2dp_source : AUDIO_SYNC_CODEC_RECONFIGURED_IND(codec)
note over a2dp_source
    Does not need a reply but need to
    re-configure the peer media channel
end note
deactivate a2dp_sink

activate a2dp_source
a2dp_source -->> a2dp_source : AV_INTERNAL_A2DP_CODEC_RECONFIG_IND
a2dp_source -> a2dp : A2dpMediaReconfigureRequest
deactivate a2dp_source

a2dp -->> a2dp_source : A2DP_MEDIA_RECONFIGURE_CFM

note over a2dp
    handset closes media channel
end note

a2dp -->> a2dp_sink : A2DP_MEDIA_CLOSE_IND
activate a2dp_sink
a2dp_sink -> audio_sync : audioSync_StateIndication(DISCONNECTED)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(DISCONNECTED)
note over a2dp_source
    Does not need a reply but need to
    close the peer media channel
end note
deactivate a2dp_sink

activate a2dp_source
a2dp_source -->> a2dp_source : AV_INTERNAL_A2DP_DISCONNECT_MEDIA_REQ
a2dp_source -> a2dp : A2dpMediaCloseRequest
deactivate a2dp_source

a2dp -->> a2dp_source : A2DP_MEDIA_CLOSE_IND

a2dp -->> a2dp_sink : A2DP_SIGNALLING_DISCONNECT_IND
activate a2dp_sink
note over a2dp_sink : Handset disconnects; peer stays connected.
a2dp_sink -> audio_sync : audioSync_StateIndication(DISCONNECTED)
audio_sync -->> a2dp_source : AUDIO_SYNC_STATE_IND(DISCONNECTED)
note over a2dp_source : Nothing more to do in this case
a2dp_sink -> peer : appAvInstanceA2dpDisconnected
peer -> a2dp_sink : appA2dpSyncDisable(handset_inst)
a2dp_sink -> a2dp_sink : appA2dpRegisterSyncTask(handset_inst, NULL)
deactivate a2dp_sink

note over a2dp_sink
    Peer instance will not receive any
    more synchronisation messages
end note

@enduml
*/

#ifndef AUDIO_SYNC_H_
#define AUDIO_SYNC_H_

#include <hydra_types.h>

#include "domain_message.h"
#include "audio_sources_list.h"


/*! \brief Simplified audio states used for synchronisation.

    The states here are intended to show how ready an audio instance is.
    How each state maps to the underlying implementation depends on the type
    of the audio instance - e.g. a2dp, usb or analogue in.

    For a2dp the states correspond to the state of the a2dp media channel:

    AUDIO_SYNC_STATE_DISCONNECTED:
        a2dp media channel not open (but signalling channel may be connected).

    AUDIO_SYNC_STATE_CONNECTED:
        a2dp media channel open; codec has been negotiated.

    AUDIO_SYNC_STATE_ACTIVE:
        a2dp media channel is in the streaming state.
*/
typedef enum
{
    AUDIO_SYNC_STATE_DISCONNECTED = 0,
    AUDIO_SYNC_STATE_CONNECTED,
    AUDIO_SYNC_STATE_ACTIVE
} audio_sync_state_t;

/*! \brief Messages used to synchronise between two AV instances. */
typedef enum
{
    AUDIO_SYNC_BASE = AUDIO_SYNC_MESSAGE_BASE,

    AUDIO_SYNC_CONNECT_IND = AUDIO_SYNC_BASE,
    AUDIO_SYNC_CONNECT_RES,
    AUDIO_SYNC_ACTIVATE_IND,
    AUDIO_SYNC_ACTIVATE_RES,

    AUDIO_SYNC_STATE_IND,

    AUDIO_SYNC_CODEC_RECONFIGURED_IND,

    AUDIO_SYNC_TOP
} audio_sync_msg_t;

/*! \brief Request a change to "Connected" state.

    Sent by the audio source to request that the audio sink does what is
    needed to get to the AUDIO_SYNC_STATE_CONNECTED state.

    The audio sink must send a AUDIO_SYNC_CONNECT_RES in response, even if
    it cannot reach the requested state.
*/
typedef struct
{
    Task task; /*!< The task to send the response, or NULL for no response. */
    audio_source_t source_id; /*!< The audio source. */
    uint8 seid; /*!< Currently active SEID */
    uint8 sync_id; /*!< The sequence ID for this sync indication. */
} AUDIO_SYNC_CONNECT_IND_T;

/*! \brief Response to a AUDIO_SYNC_CONNECT_IND */
typedef struct
{
    uint8 sync_id; /*!< The sequence ID for this sync indication. */
} AUDIO_SYNC_CONNECT_RES_T;

/*! \brief Request a change to "Active" state.

    Sent by the audio source to request that the audio sink does what is
    needed to get to the AUDIO_SYNC_STATE_ACTIVE state.

    The audio sink must send a AUDIO_SYNC_ACTIVE_RES in response, even if
    it cannot reach the requested state.
*/
typedef struct
{
    Task task; /*!< The task to send the response, or NULL for no response. */
    audio_source_t source_id; /*!< The audio source. */
    uint8 seid; /*!< Currently active SEID */
    uint8 sync_id; /*!< The sequence ID for this sync indication. */
} AUDIO_SYNC_ACTIVATE_IND_T;

/*! \brief Response to a AUDIO_SYNC_ACTIVE_IND */
typedef struct
{
    uint8 sync_id; /*!< The sequence ID for this sync indication. */
} AUDIO_SYNC_ACTIVATE_RES_T;

/*! \brief Indicate that the audio source state has changed.

    The receiving audio sink shall make a best effort to match the state
    of the audio source.

    There is no requirement to send a response to this message.
*/
typedef struct
{
    audio_sync_state_t state; /*!< The current state of the linked AV instance */
    audio_source_t source_id; /*!< The audio source. */
    uint8 seid; /*!< Currently active SEID */
} AUDIO_SYNC_STATE_IND_T;

/*! \brief Indicate that the audio source codec has changed.

    The receiving audio sink shall make a best effort to re-configure itself
    to be compatible with the new codec.

    There is no requirement to send a response to this message.
*/
typedef struct
{
    audio_source_t source_id; /*!< The audio source. */
    uint8 seid; /*!< Currently active SEID */
    uint8 device_id; /*!< The A2DP device id */
    uint8 stream_id; /*!< The A2DP stream id */
} AUDIO_SYNC_CODEC_RECONFIGURED_IND_T;


struct audio_sync_t;

/*! \brief Defines the function that sends a audio_sync_msg_t to an audio_sink.

    The receiving instance must implement this function. It allows the receiver
    to implement its own method of sending the message. For example it could
    send it conditionally, using MessageSendConditionally, instead of using
    MessageSend.

    \param sync_inst Contains the Task to send the message to.
    \param id Message Id of the message to send.
    \param message Message payload; may be NULL.
*/
typedef void (*audio_sync_send_sync_message)(struct audio_sync_t *sync_inst, MessageId id, Message message);

/*! \brief The function interface to an audio_sync object. */
typedef struct
{
    audio_sync_send_sync_message SendSyncMessage;
} audio_sync_interface_t;


/*! \brief Represents the peer instance to synchronise with. */
typedef struct audio_sync_t
{
    /*! The Task of the audio sink to send sync requests to. */
    Task task;

    /*! The audio sink specific implementation of audio_sync_interface_t */
    audio_sync_interface_t sync_if;
} audio_sync_t;



/*! \brief Indicate that an audio sink must become connected and expect a reply when complete.

    The indication is sent as a AUDIO_SYNC_CONNECT_IND to an audio sink instance.

    The audio sink instance represented by #sync_inst must do what is necessary
    to get into a connected state. A connected state means that the media codec
    has been negotiated successfully but the media stream is not currently
    active.

    When the connected state has been reached the audio sink must reply with a
    AUDIO_SYNC_CONNECT_RES to #reply_task.

    This function will also send a response to #reply_task to cover the
    following cases:

    1) If #sync_inst is NULL (e.g. no audio_sync_t instance has been registered
         with the caller). A reponse message will be sent 'now'
    2) If the audio sink does not send the response in time. A response
         message will be sent with a time delay of #AudioSync_MaxSyncDelayMs().

    It is assumed that the caller will wait for the response message
    before continuing its normal operation.

    When the caller receives the first repsonse message it must also cancel any
    other AUDIO_SYNC_CONNECT_RES that have been sent to it. This is to cover
    the case where the time-delayed message from 2) above may still be in the
    caller's queue.

    \param sync_inst The audio_sync_t instance to send the message to.
    \param reply_task The Task to send the reply message to.
    \param source_id The audio source id.
    \param seid The SEID of the audio source. The audio sink must select a
                compatible SEID.
    \param sync_id The sequence id of this specific connect indication message.
*/
void AudioSync_ConnectIndication(struct audio_sync_t *sync_inst, Task reply_task, audio_source_t source_id, uint8 seid, uint8 sync_id);

/*! \brief Indicate that an audio sink must become active and expect a reply when complete.

    The indication is sent as a AUDIO_SYNC_ACTIVE_IND to an audio sink instance.

    The audio sink instance represented by #sync_inst must do what is necessary
    to get into an active state. An active state means that the media codec
    has been negotiated successfully and the the media stream is currently
    active.

    When the active state has been reached the audio sink must reply with a
    AUDIO_SYNC_ACTIVE_RES to #reply_task.

    This function will also send a response to #reply_task to cover the
    following cases:

    1) If #sync_inst is NULL (e.g. no audio_sync_t instance has been registered
         with the caller). A reponse message will be sent 'now'
    2) If the audio sink does not send the response in time. A response
         message will be sent with a time delay of #AudioSync_MaxSyncDelayMs().

    It is assumed that the caller will wait for the response message
    before continuing its normal operation.

    When the caller receives the first repsonse message it must also cancel any
    other AUDIO_SYNC_ACTIVE_RES that have been sent to it. This is to cover the
    case where the time-delayed message from 2) above may still be in the
    caller's queue.

    \param sync_inst The audio_sync_t instance to send the message to.
    \param reply_task The Task to send the reply message to.
    \param source_id The audio source id.
    \param seid The SEID of the audio source. The audio sink must select a
                compatible SEID.
    \param sync_id The sequence id of this specific connect indication message.
*/
void AudioSync_ActivateIndication(struct audio_sync_t *sync_inst, Task reply_task, audio_source_t source_id, uint8 seid, uint8 sync_id);

/*! \brief Indicate the current state of the audio source to an audio sink.

    The indication is sent as a AUDIO_SYNC_STATE_IND to an audio sink instance.

    The audio sink shall attempt to match the state of the audio source, but
    may fail without affecting the audio source.

    There is no reply message expected to be sent back to the audio source.

    \param sync_inst The audio_sync instance to send the message to.
    \param source_id The audio source id.
    \param state The state of the audio source that the audio sink should try to match.
    \param seid The SEID of the audio source. The audio sink must select a
                compatible SEID.
*/
void AudioSync_StateIndication(struct audio_sync_t *sync_inst, audio_source_t source_id, audio_sync_state_t state, uint8 seid);

/*! \brief Indicate that the codec used by the audio source has changed.

    The indication is sent as a AUDIO_SYNC_CODEC_RECONFIGURED_IND to an audio
    sink instance.

    There is no reply message expected to be sent back to the audio source.

    Note: Currently the codec parameters are very much A2DP specific but are
          likely to change in order to work with different types of audio
          source, e.g. USB or analogue input.

    \param sync_inst The audio_sync instance to send the message to.
    \param source_id The audio source id.
    \param seid The SEID of the audio source. The audio sink must select a
                compatible SEID.
    \param device_id The A2DP device id of the audio source.
    \param stream_id The A2DP stream id of the audio source.
*/
void AudioSync_CodecReconfiguredIndication(struct audio_sync_t *sync_inst, audio_source_t source_id, uint8 seid, uint8 device_id, uint8 stream_id);

/*! \brief Cancel any pending AUDIO_SYNC_* messages on a Task.

    This will cancel all audio_sync_msg_t messages that are queued up on the
    Task.

    It may be used, for example, when destroying an object that receives
    audio_sync_msg_t messages, or if it uses the AudioSink_* API to send
    audio_sync_msg_t messages, to clear the Task of audio_sync_t instance
    of any sent but not yet delivered messages.

*/
void AudioSync_CancelQueuedMessages(struct audio_sync_t *sync_inst);

/*! \brief Maximum time to wait for audio sync to complete after which initiating
           task will continue.
*/
#define AudioSync_MaxSyncDelayMs() 500

#endif /* AUDIO_SYNC_H_ */
