/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_send_command.c

DESCRIPTION
    Functions to send commands to the phone with functions to handle responses to
    the commands.
*/

#include "logging.h"
#include "accessories.pb-c.h"
#include "ama_send_envelope.h"
#include "ama_send_command.h"
#include "ama_speech.h"

void AmaSendCommand_StartSpeech(SpeechInitiator__Type speech_initiator,
                                             AudioProfile audio_profile,
                                             AudioFormat audio_format,
                                             AudioSource audio_source,
                                             uint32 start_sample,
                                             uint32 end_sample
)
{
    ControlEnvelope  control_envelope_out = CONTROL_ENVELOPE__INIT;
    control_envelope_out.command = COMMAND__START_SPEECH;
    control_envelope_out.payload_case = CONTROL_ENVELOPE__PAYLOAD_START_SPEECH;
    StartSpeech start_speech = START_SPEECH__INIT;
    SpeechSettings settings = SPEECH_SETTINGS__INIT;

    settings.audio_profile = audio_profile;
    settings.audio_format = audio_format;
    settings.audio_source = audio_source;

    start_speech.settings = &settings;

    SpeechInitiator  initiator = SPEECH_INITIATOR__INIT;

    initiator.type = speech_initiator;

    SpeechInitiator__WakeWord wakeWord = SPEECH_INITIATOR__WAKE_WORD__INIT;

    wakeWord.start_index_in_samples = start_sample;
    wakeWord.end_index_in_samples = end_sample;

    initiator.wake_word = &wakeWord;

    start_speech.initiator = &initiator;

    Dialog dialog = DIALOG__INIT;
    dialog.id = AmaSpeech_GetCurrentDialogId();

    start_speech.dialog = &dialog;

    control_envelope_out.u.start_speech = &start_speech;

    DEBUG_LOG("AMA Send COMMAND__START_SPEECH Dialog Id=%d", dialog.id);

    AmaSendEnvelope_Send(&control_envelope_out);
}


void AmaSendCommand_HandleResponseStartSpeech(ControlEnvelope *control_envelope_in)
{
    Response* response = control_envelope_in->u.response;
    Response__PayloadCase response_case = response->payload_case;

    DEBUG_LOG("AMA COMMAND__START_SPEECH Response ");

    if( response->error_code == ERROR_CODE__SUCCESS)
    {
        switch(response_case)
        {
            case RESPONSE__PAYLOAD__NOT_SET:
                DEBUG_LOG("No payload");
                break;

            case RESPONSE__PAYLOAD_DIALOG:
                {   /* AMA TO-DO cg11 should we check this against the one we sent ? */
                    DEBUG_LOG("Dialog ID %d", response->u.dialog->id);
                }
                break;

            default:
                DEBUG_LOG("Unhandled response_case %d, Dialog ID %d", response_case, response->u.dialog->id);
                break;
        }
    }
    else
    {
        DEBUG_LOG(" Error!! Code=%d", response->error_code);
        AmaNotifyAppMsg_StopSpeechMsg();
    }
}

void AmaSendCommand_StopSpeech(ErrorCode reason)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;
    controlEnvelopeOut.command = COMMAND__STOP_SPEECH;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_STOP_SPEECH;

    Dialog dialog = DIALOG__INIT;

    dialog.id = AmaSpeech_GetCurrentDialogId();

    DEBUG_LOG("AMA Send COMMAND__STOP_SPEECH Dialog Id=%d", dialog.id);

    StopSpeech stop_speech = STOP_SPEECH__INIT;

    stop_speech.dialog = &dialog;
    stop_speech.error_code = reason;
    controlEnvelopeOut.u.stop_speech = &stop_speech;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_EndSpeech(void)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;
    controlEnvelopeOut.command = COMMAND__ENDPOINT_SPEECH;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_ENDPOINT_SPEECH;

    Dialog dialog = DIALOG__INIT;

    dialog.id = AmaSpeech_GetCurrentDialogId();

    DEBUG_LOG("AMA Send COMMAND__ENDPOINT_SPEECH Dialog Id=%d", dialog.id);

    EndpointSpeech endpoint_speech = ENDPOINT_SPEECH__INIT;

    endpoint_speech.dialog = &dialog;

    controlEnvelopeOut.u.endpoint_speech = &endpoint_speech;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_IncomingCall(char* caller_number)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;

    DEBUG_LOG("AMA Send COMMAND__INCOMING_CALL");

    controlEnvelopeOut.command = COMMAND__INCOMING_CALL;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_INCOMING_CALL;

    IncomingCall incoming_call = INCOMING_CALL__INIT;

    incoming_call.caller_number = caller_number;  /* AMA_TODO Revisit for future improvement */

    controlEnvelopeOut.u.incoming_call = &incoming_call;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}


void AmaSendCommand_KeepAlive(void)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;

    DEBUG_LOG("AMA Send COMMAND__KEEP_ALIVE");

    controlEnvelopeOut.command = COMMAND__KEEP_ALIVE;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_KEEP_ALIVE;

    KeepAlive  keep_alive = KEEP_ALIVE__INIT;

    controlEnvelopeOut.u.keep_alive = &keep_alive;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_NotifyDeviceConfig(bool require_va_override)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;

    DEBUG_LOG("AMA Send COMMAND__NOTIFY_DEVICE_CONFIGURATION");

    controlEnvelopeOut.command = COMMAND__NOTIFY_DEVICE_CONFIGURATION;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_NOTIFY_DEVICE_CONFIGURATION;

    NotifyDeviceConfiguration  notify_device_config = NOTIFY_DEVICE_CONFIGURATION__INIT;

    /* AMA_TODO - memory needs to be malloc'd & freed */
    static DeviceConfiguration device_config = DEVICE_CONFIGURATION__INIT;
    notify_device_config.device_configuration = &device_config;
    notify_device_config.device_configuration->needs_assistant_override = require_va_override;

    controlEnvelopeOut.u.notify_device_configuration = &notify_device_config;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_SyncState(uint32 feature, ama_state_value_case_t value_case, uint16 integer)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;
    controlEnvelopeOut.command = COMMAND__SYNCHRONIZE_STATE;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_SYNCHRONIZE_STATE;

    State state = STATE__INIT;

    DEBUG_LOG("AMA Send COMMAND__SYNCHRONIZE_STATE");

    state.feature = feature;
    state.value_case = (State__ValueCase) value_case;

    if(value_case == STATE__VALUE_BOOLEAN)
    {
        state.u.boolean = (protobuf_c_boolean)integer;
    }
    else  if(value_case == STATE__VALUE_INTEGER)
    {
        state.u.boolean = (uint32_t)integer;
    }
    else
    {
        return;
    }

    SynchronizeState synchronize_state = SYNCHRONIZE_STATE__INIT;
    synchronize_state.state = &state;

    controlEnvelopeOut.u.synchronize_state = &synchronize_state;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}


void AmaSendCommand_GetState(uint32 feature)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;
    controlEnvelopeOut.command = COMMAND__GET_STATE;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_GET_STATE;

    GetState get_state = GET_STATE__INIT;

    DEBUG_LOG("AMA Send COMMAND__GET_STATE");

    get_state.feature = feature;

    controlEnvelopeOut.u.get_state = &get_state;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_ResetConnection(uint32 timeout, bool force_disconnect)
{
    ControlEnvelope  controlEnvelopeOut = CONTROL_ENVELOPE__INIT;
    controlEnvelopeOut.command = COMMAND__RESET_CONNECTION;
    controlEnvelopeOut.payload_case =  CONTROL_ENVELOPE__PAYLOAD_RESET_CONNECTION;

    ResetConnection reset_connection = RESET_CONNECTION__INIT;

    reset_connection.force_disconnect = force_disconnect;
    reset_connection.timeout = timeout;

    DEBUG_LOG("AMA Send COMMAND__RESET_CONNECTION");

    controlEnvelopeOut.u.reset_connection = &reset_connection;

    AmaSendEnvelope_Send(&controlEnvelopeOut);
}

void AmaSendCommand_GetCentralInformation(void)
{
    ControlEnvelope  control_envelope_out = CONTROL_ENVELOPE__INIT;
    control_envelope_out.command = COMMAND__GET_CENTRAL_INFORMATION;
    control_envelope_out.payload_case =  CONTROL_ENVELOPE__PAYLOAD_GET_CENTRAL_INFORMATION;

    GetCentralInformation  get_central_information = GET_CENTRAL_INFORMATION__INIT;

    DEBUG_LOG("AMA Send COMMAND__GET_CENTRAL_INFORMATION");

    control_envelope_out.u.get_central_information = &get_central_information;

    AmaSendEnvelope_Send(&control_envelope_out);
}

void AmaSendCommand_HandleResponseGetCentralInformation(ControlEnvelope *control_envelope_in)
{   /* Check this. constants.ph-c.h says only the phone can send COMMAND__GET_CENTRAL_INFORMATION */
    /* and we have nothing to send the command??? */
#ifdef DEBUG_AMA_LIB
    Response* response = control_envelope_in->u.response;

    DEBUG_LOG(("AMA COMMAND__GET_CENTRAL_INFORMATION response "));
    if(NULL != response)
    {
        Response__PayloadCase respCase = response->payload_case;
        CentralInformation * centralInformation = response->u.central_information;
        DEBUG_LOG(("Reponse Case %d, code %d, ", respCase, response->error_code));
        if(NULL != centralInformation)
            DEBUG_LOG(("central name %s, platform == %d\n", centralInformation->name, centralInformation->platform));
    }
    
#else
    UNUSED(control_envelope_in);
#endif /* DEBUG_AMA_LIB */

}

void AmaSendCommand_HandleResponseNotHandled(ControlEnvelope *control_envelope_in)
{
#ifdef DEBUG_AMA_LIB
    Command command = control_envelope_in->command;
    Response* response = control_envelope_in->u.response;
    Response__PayloadCase response_case = response->payload_case;
    DEBUG_LOG(("AMA Reponse Not handled command = %d, case %d, error code %d == ", command, response_case, response->error_code));
#else
    UNUSED(control_envelope_in);
#endif /* DEBUG_AMA_LIB */
}

void AmaSendCommand_ProvideSpeechRsp(bool accept, uint32 resp_id)
{
    ControlEnvelope  control_envelope_out = CONTROL_ENVELOPE__INIT;
    Response response = RESPONSE__INIT;
    SpeechProvider speech_provider = SPEECH_PROVIDER__INIT;
    SpeechSettings settings = SPEECH_SETTINGS__INIT;
    Dialog  dialog_response = DIALOG__INIT;

    /* First fill the control envelope */
    control_envelope_out.command = COMMAND__PROVIDE_SPEECH;
    control_envelope_out.u.response = &response;
    control_envelope_out.payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;

    /* Next the response part */
    response.error_code = ERROR_CODE__SUCCESS;
    response.payload_case = RESPONSE__PAYLOAD_SPEECH_PROVIDER;
    response.u.speech_provider = &speech_provider;

    /* Next the speech provider response part */
    speech_provider.speech_settings = &settings;
    speech_provider.dialog = &dialog_response;
    
    /* Next the setting of speech which forms the part of response */
    settings.audio_profile = AmaSpeech_GetAudioProfile();
    settings.audio_format  = AmaSpeech_GetAudioFormat();
    settings.audio_source  = AmaSpeech_GetAudioSource();
    /* Update the dailog id */
    dialog_response.id = resp_id;
    if(!accept)
    {
        response.error_code = ERROR_CODE__BUSY;
        DEBUG_LOG("AMA Responding busy - dialog id =%d", dialog_response.id);
    }
    AmaSendEnvelope_Send(&control_envelope_out);

}


