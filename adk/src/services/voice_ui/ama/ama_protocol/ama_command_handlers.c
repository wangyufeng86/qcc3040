/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_command_handlers.c

DESCRIPTION
    Handlers for ama commands
*/

#include "ama.h"
#include "ama_state.h"
#include "ama_speech.h"
#include "ama_command_handlers.h"
#include "speech.pb-c.h"
#include "logging.h"
#include "ama_send_envelope.h"
#include "ama_send_command.h"
#include "ama_private.h"
#include "voice_ui_container.h"
#include <string.h>

#define AMA_SEND_NOTIFY_DEVICE_CFG_DELAY        D_SEC(1)
#define AMA_INTERNAL_MSG_ASSISTANT_OVERRIDEN    1
#define MAKE_DEFAULT_RESPONSE_ENVELOPE(envelope_name, command_id) \
Response response = RESPONSE__INIT;\
response.error_code = ERROR_CODE__SUCCESS;\
ControlEnvelope  envelope_name = CONTROL_ENVELOPE__INIT;\
amaCommandHandlers_ElementPaster(&envelope_name, command_id , &response);

static void amaCommandHandlers_SendDefaultResponse(Command command);
static void amaCommandHandlers_InternalMsgHandler(Task task, MessageId id, Message message);
static TaskData internalMsgTask = {amaCommandHandlers_InternalMsgHandler};

static void amaCommandHandlers_InternalMsgHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    switch(id)
    {
        case AMA_INTERNAL_MSG_ASSISTANT_OVERRIDEN:
            AmaSendCommand_NotifyDeviceConfig(ASSISTANT_OVERRIDEN);
        break;

        default:
            break;
    }
}

static void amaCommandHandlers_ElementPaster(ControlEnvelope* envelope_name,Command command_id, Response* response)
{
    envelope_name->command = command_id;
    envelope_name->u.response = response;
    envelope_name->payload_case = CONTROL_ENVELOPE__PAYLOAD_RESPONSE;
}

static void amaCommandHandlers_NotifyStateMsg(SpeechState state)
{
    ama_speech_state_t speech_state = ama_speech_state_err;

    switch(state)
    {
        case SPEECH_STATE__IDLE:
            speech_state = ama_speech_state_idle;
            break;

        case SPEECH_STATE__LISTENING:
            speech_state = ama_speech_state_listening;
            break;

        case SPEECH_STATE__PROCESSING:
            speech_state = ama_speech_state_processing;
            break;

        case SPEECH_STATE__SPEAKING:
            speech_state = ama_speech_state_speaking;
            break;

        default:
            DEBUG_LOG("AMA Unknown speech state indicated%d", state);
            break;
    }

    if(speech_state != ama_speech_state_err)
    {
        /* AMA A2DP stream set to streaming for AMA query prompt tone and suspended after response */
        voice_ui_a2dp_state_t va_a2dp_stream_state = (speech_state == ama_speech_state_idle) ? voice_ui_a2dp_state_suspended : voice_ui_a2dp_state_streaming;
        Ama_GetVoiceUiProtectedInterface()->SetVoiceAssistantA2dpStreamState(va_a2dp_stream_state);

        AmaNotifyAppMsg_StateMsg(speech_state);
    }
}

void AmaCommandHandlers_NotifySpeechState(ControlEnvelope *control_envelope_in)
{
    NotifySpeechState *notify_speech_state = control_envelope_in->u.notify_speech_state;

    DEBUG_LOG("AMA COMMAND__NOTIFY_SPEECH_STATE received State=%d", notify_speech_state->state);

    amaCommandHandlers_NotifyStateMsg(notify_speech_state->state);
}


void AmaCommandHandlers_StopSpeech(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__STOP_SPEECH received Error=%d",
                    control_envelope_in->u.stop_speech->error_code);

    Dialog *dialog = control_envelope_in->u.stop_speech->dialog;

    if(dialog->id == AmaSpeech_GetCurrentDialogId())
    {
        AmaNotifyAppMsg_StopSpeechMsg();
    }

    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}


void AmaCommandHandlers_GetDeviceInformation(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    Transport supported_transports[NUMBER_OF_SUPPORTED_TRANSPORTS];
    DeviceInformation device_information = DEVICE_INFORMATION__INIT;

    /* Get the AMA device configuration. */
    ama_device_config_t *device_config = AmaProtocol_GetDeviceConfiguration();

    DEBUG_LOG("AMA COMMAND__GET_DEVICE_INFORMATION received");

    device_information.n_supported_transports = AmaProtocol_GetNumTransportSupported();

    device_information.name = device_config->name;
    device_information.device_type = device_config->device_type;
    device_information.serial_number = device_config->serial_number;

    supported_transports[TRANSPORT__BLUETOOTH_LOW_ENERGY] = TRANSPORT__BLUETOOTH_LOW_ENERGY;
    supported_transports[TRANSPORT__BLUETOOTH_RFCOMM] = TRANSPORT__BLUETOOTH_RFCOMM;
    supported_transports[TRANSPORT__BLUETOOTH_IAP] = TRANSPORT__BLUETOOTH_IAP;

    device_information.supported_transports = &supported_transports[0];

    /* assign resposne union type */
    response.u.device_information = &device_information;

    response.payload_case = RESPONSE__PAYLOAD_DEVICE_INFORMATION;

    AmaSendEnvelope_Send(&control_envelope_out);

}


void AmaCommandHandlers_GetDeviceConfiguration(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    DEBUG_LOG("AMA COMMAND__GET_DEVICE_CONFIGURATION received");

    DeviceConfiguration device_config = DEVICE_CONFIGURATION__INIT;

    bool require_assistant_override = Ama_GetVoiceUiProtectedInterface()->GetActiveVaProvider() != voice_ui_provider_ama;
    device_config.needs_assistant_override = require_assistant_override;
    device_config.needs_setup = TRUE;

    /* assign response union type */
    response.u.device_configuration = &device_config;

    response.payload_case = RESPONSE__PAYLOAD_DEVICE_CONFIGURATION;
    AmaSendEnvelope_Send(&control_envelope_out);

}


void AmaCommandHandlers_StartSetup(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__START_SETUP received");
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}

void AmaCommandHandlers_CompleteSetup(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__COMPLETE_SETUP received");
    VoiceUi_SelectVoiceAssistant(voice_ui_provider_ama);
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}

static void amaCommandHandlers_BdaddrToArray(uint8 *array, bdaddr *bdaddr_in)
{
    array[1] = (uint8)(bdaddr_in->nap & 0xff);
    array[0] = (uint8)((bdaddr_in->nap>>8) & 0xff);
    array[2] = bdaddr_in->uap;
    array[5] = (uint8)(bdaddr_in->lap) & 0xff;
    array[4] = (uint8)(bdaddr_in->lap>>8) & 0xff ;
    array[3] = (uint8)(bdaddr_in->lap>>16) & 0xff ;
}

void AmaCommandHandlers_UpgradeTransport(ControlEnvelope *control_envelope_in)
{
    uint8 bdaddr_array[6];
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);
    ConnectionDetails connection_details = CONNECTION_DETAILS__INIT;
    UpgradeTransport *upgrade_transport = control_envelope_in->u.upgrade_transport;

    DEBUG_LOG("AMA COMMAND__UPGRADE_TRANSPORT received. Transport=%d", upgrade_transport->transport);

    amaCommandHandlers_BdaddrToArray(bdaddr_array, AmaProtocol_GetLocalAddress());

    AmaProtocol_SendAppMsg(AMA_ENABLE_CLASSIC_PAIRING_IND, NULL);

    connection_details.identifier.len = 6;
    connection_details.identifier.data = bdaddr_array;

    response.payload_case = RESPONSE__PAYLOAD_CONNECTION_DETAILS;
    response.u.connection_details = &connection_details;

    AmaProtocol_SendAppMsg(AMA_UPGRADE_TRANSPORT_IND, NULL);

    AmaSendEnvelope_Send(&control_envelope_out);
}

void AmaCommandHandlers_SwitchTransport(ControlEnvelope *control_envelope_in)
{

    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    SwitchTransport* switch_transport = control_envelope_in->u.switch_transport;

    DEBUG_LOG("AMA COMMAND__SWITCH_TRANSPORT received Transport=%d", switch_transport->new_transport);

    AmaSendEnvelope_Send(&control_envelope_out);

    /* AMA_TODO ... Revisit for future improvement */
    AmaSendCommand_GetCentralInformation();

    AmaNotifyAppMsg_TransportSwitch((ama_transport_t)switch_transport->new_transport);
}

void AmaCommandHandlers_SynchronizeSettings(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    DEBUG_LOG("AMA COMMAND__SYNCHRONIZE_SETTINGS received");

    AmaSendEnvelope_Send(&control_envelope_out);

    AmaSendCommand_GetCentralInformation();

    AmaNotifyAppMsg_SynchronizeSettingMsg();
}

void AmaCommandHandlers_GetState(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    GetState *get_state = control_envelope_in->u.get_state;
    uint32_t feature = get_state->feature;
    uint32 state_value;
    State__ValueCase value_case;

    DEBUG_LOG("AMA COMMAND__GET_STATE feature %x", feature);

    State state = STATE__INIT;

    response.error_code = (ErrorCode) AmaState_GetState(feature, &state_value, (ama_state_value_case_t*)&value_case);

    state.value_case = value_case;
    state.feature = feature;

    if(state.value_case == STATE__VALUE_BOOLEAN)
    {
        state.u.boolean = (protobuf_c_boolean)state_value;
    }
    else if(state.value_case == STATE__VALUE_INTEGER)
    {
        state.u.integer = (uint32_t)state_value;
    }

    response.payload_case = RESPONSE__PAYLOAD_STATE;
    response.u.state = &state;

    AmaSendEnvelope_Send(&control_envelope_out);
}


void AmaCommandHandlers_SetState(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    SetState *set_state = control_envelope_in->u.set_state;
    uint32 feature = (uint32)set_state->state->feature;
    State__ValueCase value_case = set_state->state->value_case;
    uint32 state_value = 0xFFFF;

    if(value_case == STATE__VALUE_BOOLEAN)
    {
        state_value = (uint32)set_state->state->u.boolean;
    }
    else if (value_case == STATE__VALUE_INTEGER)
    {
        state_value = (uint32)set_state->state->u.integer;
    }

    DEBUG_LOG("AMA COMMAND__SET_STATE received feature %x valcase %d val %d", feature, value_case, state_value);

    response.error_code = (ErrorCode) AmaState_SetState(feature, state_value, (ama_state_value_case_t)value_case);

    AmaSendEnvelope_Send(&control_envelope_out);

}

void AmaCommandHandlers_MediaControl(ControlEnvelope *control_envelope_in)
{

    IssueMediaControl *issue_media_control = control_envelope_in->u.issue_media_control;
    MediaControl control =  issue_media_control->control;

    DEBUG_LOG("AMA COMMAND__ISSUE_MEDIA_CONTROL received control=%d", control);

    AmaProtocol_MediaControl((AMA_MEDIA_CONTROL) control);
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}

void AmaCommandHandlers_OverrideAssistant(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__OVERRIDE_ASSISTANT received");
    VoiceUi_SelectVoiceAssistant(voice_ui_provider_ama);
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);

    /* Notify Alexa that the assistant has been overridden */
    MessageSendLater(&internalMsgTask, AMA_INTERNAL_MSG_ASSISTANT_OVERRIDEN,
                     NULL, AMA_SEND_NOTIFY_DEVICE_CFG_DELAY);
}

void AmaCommandHandlers_SynchronizeState(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__SYNCHRONIZE_SETTINGS received");
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}

void AmaCommandHandlers_ProvideSpeech(ControlEnvelope *control_envelope_in)
{
    ProvideSpeech *provide_speech = control_envelope_in->u.provide_speech;
    Dialog* dialog = provide_speech->dialog;

    DEBUG_LOG("AMA COMMAND__PROVIDE_SPEECH - dialog id =%d", dialog->id);

    AmaNotifyAppMsg_ProvideSpeechMsg(dialog->id);
}

void AmaCommandHandlers_EndpointSpeech(ControlEnvelope *control_envelope_in)
{

    DEBUG_LOG("AMA COMMAND__ENDPOINT_SPEECH ");

    if(control_envelope_in->payload_case == CONTROL_ENVELOPE__PAYLOAD_ENDPOINT_SPEECH)
    {
        EndpointSpeech* endpoint_speech = control_envelope_in->u.endpoint_speech;
        Dialog *dialog = endpoint_speech->dialog;

        DEBUG_LOG("Rx Dialog ID %d", dialog->id);
        if(dialog->id == AmaSpeech_GetCurrentDialogId())
        {
            AmaNotifyAppMsg_StopSpeechMsg();
        }
        else
        {
            DEBUG_LOG("AMA Dialog Id incorrect. Received %d, should be %d",
                        dialog->id,
                        AmaSpeech_GetCurrentDialogId());
        }
    }
    else if(control_envelope_in->payload_case == CONTROL_ENVELOPE__PAYLOAD_NOTIFY_SPEECH_STATE)
    {
        /* probably we get this case if send end speech when there is no speech going on */
        NotifySpeechState * notify_speech_state = control_envelope_in->u.notify_speech_state;
        SpeechState state = notify_speech_state->state;
        DEBUG_LOG("NOTIFY_SPEECH_STATE %d", state);
        amaCommandHandlers_NotifyStateMsg(state);
    }
    else
    {
        DEBUG_LOG("unexpected payload case %d", control_envelope_in->payload_case);
    }

    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);

}


static ErrorCode amaCommandHandlers_ProcessForwardAtCommand(char* command)
{
    typedef struct{
        char* at_string;
        ama_at_cmd_t command;
    }at_lookup_t;

    static const at_lookup_t  at_lookup[] = {
        {"ATA",           ama_at_cmd_ata_ind},
        {"AT+CHUP",       ama_at_cmd_at_plus_chup_ind},
        {"AT+BLDN",       ama_at_cmd_at_plus_bldn_ind},
        {"AT+CHLD=0",     ama_at_cmd_at_plus_chld_eq_0_ind},
        {"AT+CHLD=1",     ama_at_cmd_at_plus_chld_eq_1_ind},
        {"AT+CHLD=2",     ama_at_cmd_at_plus_chld_eq_2_ind},
        {"AT+CHLD=3",     ama_at_cmd_at_plus_chld_eq_3_ind},
        {"ATD",           ama_at_cmd_atd_ind}
    };

    uint8 num_of_commands = sizeof(at_lookup) / sizeof(at_lookup[0]);
    uint8 index;

    for(index = 0; index < num_of_commands; index++)
    {
        if(strcmp(at_lookup[index].at_string, command) == 0)
        {
            MAKE_AMA_MESSAGE(AMA_SEND_AT_COMMAND_IND);

            message->at_command = at_lookup[index].command;

            AmaProtocol_SendAppMsg(AMA_SEND_AT_COMMAND_IND, message);

            return ERROR_CODE__SUCCESS;
        }
    }

    return ERROR_CODE__UNKNOWN;
}


void AmaCommandHandlers_ForwardATCommand(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);

    ForwardATCommand* forward_at_command = control_envelope_in->u.forward_at_command;

    char* forward_command = forward_at_command->command;

    DEBUG_LOG("AMA COMMAND__FORWARD_AT_COMMAND received Command %s", forward_command);

    response.error_code = amaCommandHandlers_ProcessForwardAtCommand(forward_command);

    AmaSendEnvelope_Send(&control_envelope_out);
}

void AmaCommandHandlers_NotHandled(ControlEnvelope *control_envelope_in)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, control_envelope_in->command);
    DEBUG_LOG("AMA unhandled command!! %d", control_envelope_in->command);
    response.error_code = ERROR_CODE__UNSUPPORTED;
    AmaSendEnvelope_Send(&control_envelope_out);
}


void AmaCommandHandlers_KeepAlive(ControlEnvelope *control_envelope_in)
{
    DEBUG_LOG("AMA COMMAND__KEEP_ALIVE received");
    amaCommandHandlers_SendDefaultResponse(control_envelope_in->command);
}


static void amaCommandHandlers_SendDefaultResponse(Command command)
{
    MAKE_DEFAULT_RESPONSE_ENVELOPE(control_envelope_out, command);
    AmaSendEnvelope_Send(&control_envelope_out);
}

