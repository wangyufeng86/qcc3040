#include "speech.pb-c.h"

#include "ama_speech.h"
#include "ama_send_command.h"
#include <panic.h>
#include <source.h>
#include "ama_debug.h"
#include "message.h"

typedef enum  {
    ama_speech_initiator_none,
    ama_speech_initiator_press_and_hold,
    ama_speech_initiator_tap,
    ama_speech_initiator_wakeword,
} ama_speech_initiator_t;

typedef enum{
    ama_audio_profile_close_talk,
    ama_audio_profile_near_field,
    ama_audio_profile_far_field
}ama_audio_profile_t;

typedef enum{
    ama_audio_source_stream,
    ama_audio_source_bluetooth_sco
} ama_audio_source_t;

typedef struct{
    ama_speech_initiator_t initiator;
    uint32 pre_roll;
    /* start timestamp for trigger phrase of buffered data */
    uint32 start_timestamp;
    /* end timestamp for trigger phrase of buffered data */
    uint32 end_timestamp;
}ama_speech_msg_start_t;


typedef struct{
    uint32 dialog_id;
}ama_speech_msg_provide_speech_t;

typedef struct{
    SpeechInitiator__Type speech_initiator;
    AudioProfile audio_profile;
    AudioFormat audio_format;
    AudioSource audio_source;
    uint32 new_dialog_id;
    uint32 current_dialog_id;
}speech_settings_t;

#define MAX_START_DIALOG_ID 0x7fffffff

#define AMA_SPEECH_INITIATOR_DEFAULT        ama_speech_initiator_tap
#define AMA_SPEECH_AUDIO_PROFILE_DEFAULT    ama_audio_profile_near_field
#define AMA_SPEECH_AUDIO_FORMAT_DEFAULT     ama_audio_format_msbc
#define AMA_SPEECH_AUDIO_SOURCE_DEFAULT     ama_audio_source_stream


static speech_settings_t speech_settings = {
    AMA_SPEECH_INITIATOR_DEFAULT,
    AMA_SPEECH_AUDIO_PROFILE_DEFAULT,
    AMA_SPEECH_AUDIO_FORMAT_DEFAULT,
    AMA_SPEECH_AUDIO_SOURCE_DEFAULT,
    MAX_START_DIALOG_ID,
    0
};

/* Local function forward declarations */
static bool amaSpeech_HandleSpeechStart(ama_speech_msg_start_t *msg);

static void amaSpeech_SetInitiator(ama_speech_initiator_t initiator);
static void amaSpeech_SetAudioProfile(ama_audio_profile_t profile);

static void amaSpeech_NewDialogId(void);

/******************************************************************/
static void amaSpeech_NewDialogId(void)
{
    if(speech_settings.new_dialog_id == MAX_START_DIALOG_ID)
    {
        speech_settings.new_dialog_id = 0;
    }
    else
    {
        speech_settings.new_dialog_id++;
    }
    speech_settings.current_dialog_id = speech_settings.new_dialog_id;
}

static void amaSpeech_SetAudioSource(ama_audio_source_t source)
{
    switch(source)
    {
        case ama_audio_source_stream:
            speech_settings.audio_source = AUDIO_SOURCE__STREAM;
            break;

        case ama_audio_source_bluetooth_sco:
            speech_settings.audio_source = AUDIO_SOURCE__BLUETOOTH_SCO;
            break;

        default:
            Panic();
            break;
    }
}


/******************************************************************/
static void amaSpeech_SetAudioProfile(ama_audio_profile_t profile)
{
    switch(profile)
    {
        case ama_audio_profile_close_talk:
            speech_settings.audio_profile = AUDIO_PROFILE__CLOSE_TALK;
            break;

        case ama_audio_profile_near_field:
            speech_settings.audio_profile = AUDIO_PROFILE__NEAR_FIELD;
            break;

        case ama_audio_profile_far_field:
            speech_settings.audio_profile = AUDIO_PROFILE__FAR_FIELD;
            break;

        default:
            Panic();
            break;
    }
}

/******************************************************************/
static void amaSpeech_SetInitiator(ama_speech_initiator_t initiator)
{
    switch(initiator)
    {
        case ama_speech_initiator_none:
            speech_settings.speech_initiator = SPEECH_INITIATOR__TYPE__NONE;
            break;

        case ama_speech_initiator_press_and_hold:
            speech_settings.speech_initiator = SPEECH_INITIATOR__TYPE__PRESS_AND_HOLD;
            break;

        case ama_speech_initiator_tap:
            speech_settings.speech_initiator = SPEECH_INITIATOR__TYPE__TAP;
            break;

        case ama_speech_initiator_wakeword:
            speech_settings.speech_initiator = SPEECH_INITIATOR__TYPE__WAKEWORD;
            break;

        default:
            break;
    }
}

/******************************************************************/
void AmaSpeech_SetAudioFormat(ama_audio_format_t format)
{
    switch(format)
    {
        case ama_audio_format_pcm_l16_16khz_mono:
            speech_settings.audio_format = AUDIO_FORMAT__PCM_L16_16KHZ_MONO;
            break;

        case ama_audio_format_opus_16khz_32kbps_cbr_0_20ms:
            speech_settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_32KBPS_CBR_0_20MS;
            break;

        case ama_audio_format_opus_16khz_16kbps_cbr_0_20ms:
            speech_settings.audio_format = AUDIO_FORMAT__OPUS_16KHZ_16KBPS_CBR_0_20MS;
            break;

        case ama_audio_format_msbc:
            speech_settings.audio_format = AUDIO_FORMAT__MSBC;
            break;

        default:
            Panic();
            break;
    }
}

/******************************************************************/
AudioSource AmaSpeech_GetAudioSource(void)
{
    return speech_settings.audio_source;
}

/******************************************************************/
AudioFormat AmaSpeech_GetAudioFormat(void)
{
    return speech_settings.audio_format;
}

/******************************************************************/
AudioProfile AmaSpeech_GetAudioProfile(void)
{
    return speech_settings.audio_profile;
}

/******************************************************************/
static bool amaSpeech_HandleSpeechStart(ama_speech_msg_start_t *msg)
{
    if(msg)
    {
        uint32 start_sample = 0;
        uint32 end_sample = 0;

        amaSpeech_SetInitiator(msg->initiator);
        if(msg->initiator == ama_speech_initiator_press_and_hold)
        {
            amaSpeech_SetAudioProfile(ama_audio_profile_close_talk);
        }
        else
        {
            amaSpeech_SetAudioProfile(AMA_SPEECH_AUDIO_PROFILE_DEFAULT);
        }
        
        amaSpeech_NewDialogId();

        if(msg->initiator == ama_speech_initiator_wakeword)
        {
            #define SAMPLES_MS 16

            uint32 tp_len;

            start_sample = (msg->pre_roll/1000) * SAMPLES_MS;

            if(msg->end_timestamp >= msg->start_timestamp)
            {
                tp_len = msg->end_timestamp - msg->start_timestamp;
            }
            else
            {
                tp_len = msg->end_timestamp + (~(msg->start_timestamp)) + 1;
            }

            end_sample = ((tp_len/1000) * SAMPLES_MS) + start_sample;

        }

        AmaSendCommand_StartSpeech(speech_settings.speech_initiator,
                                  speech_settings.audio_profile,
                                  speech_settings.audio_format,
                                  speech_settings.audio_source,
                                  start_sample,
                                  end_sample);
        return TRUE;
    }
    return FALSE;
}

/******************************************************************/
uint32 AmaSpeech_GetCurrentDialogId(void)
{
    return speech_settings.current_dialog_id;
}

/******************************************************************/
bool AmaSpeech_StartTapToTalk(void)
{
    ama_speech_msg_start_t msg;

    msg.initiator = ama_speech_initiator_tap;
    return amaSpeech_HandleSpeechStart(&msg);
}

/******************************************************************/
bool AmaSpeech_StartPushToTalk(void)
{
    ama_speech_msg_start_t msg;

    msg.initiator = ama_speech_initiator_press_and_hold;
    return amaSpeech_HandleSpeechStart(&msg);
}

/******************************************************************/
void AmaSpeech_Stop(void)
{
    AmaSendCommand_StopSpeech(ERROR_CODE__USER_CANCELLED);
}
/******************************************************************/
void AmaSpeech_End(void)
{
    AmaSendCommand_EndSpeech();
}
/******************************************************************/
void AmaSpeech_UpdateProvidedDialogId(uint32 provided_id)
{
    if(provided_id > MAX_START_DIALOG_ID)
    {
        speech_settings.current_dialog_id = provided_id;
    }
}

/******************************************************************/
void AmaSpeech_SetToDefault(void)
{
    amaSpeech_SetAudioSource(AMA_SPEECH_AUDIO_SOURCE_DEFAULT);
    AmaSpeech_SetAudioFormat(AMA_SPEECH_AUDIO_FORMAT_DEFAULT);
    amaSpeech_SetAudioProfile(AMA_SPEECH_AUDIO_PROFILE_DEFAULT);
    amaSpeech_SetInitiator(AMA_SPEECH_INITIATOR_DEFAULT);
    speech_settings.new_dialog_id = MAX_START_DIALOG_ID;
    speech_settings.current_dialog_id = 0;
}

