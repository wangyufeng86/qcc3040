/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_receive_command.c

DESCRIPTION
    Passes incoming command packet to respective handler function
*/

#include "stdio.h"
#include "stdlib.h"
#include <vm.h>
#include "ama_receive_command.h"
#include "ama_send_command.h"
#include "accessories.pb-c.h"
#include "ama_command_handlers.h"
#include "ama_debug.h"


static void amaReceive_HandleCommand(ControlEnvelope *control_envelope_in);
static void amaReceive_HandleResponse(ControlEnvelope *control_envelope_in);



void AmaReceive_Command(char* data, uint16 length)
{
    if(data && length)
    {
        ControlEnvelope* control_envelope_in = 
                control_envelope__unpack(NULL, (size_t)length, (const uint8_t*)data);

        if(control_envelope_in)
        {
            if(control_envelope_in->payload_case == CONTROL_ENVELOPE__PAYLOAD_RESPONSE)
            {
                amaReceive_HandleResponse(control_envelope_in);
            }
            else
            {
                amaReceive_HandleCommand(control_envelope_in);
            }

            control_envelope__free_unpacked(control_envelope_in, NULL);

        }

    }
}


static void amaReceive_HandleCommand(ControlEnvelope *control_envelope_in)
{

    switch(control_envelope_in->command)
    {
        case COMMAND__GET_DEVICE_INFORMATION:
            AmaCommandHandlers_GetDeviceInformation(control_envelope_in);
            break;

        case COMMAND__GET_DEVICE_CONFIGURATION:
            AmaCommandHandlers_GetDeviceConfiguration(control_envelope_in);
            break;

        case COMMAND__START_SETUP:
            AmaCommandHandlers_StartSetup(control_envelope_in);
            break;

        case COMMAND__COMPLETE_SETUP:
            AmaCommandHandlers_CompleteSetup(control_envelope_in);
            break;

        case COMMAND__UPGRADE_TRANSPORT:
            AmaCommandHandlers_UpgradeTransport(control_envelope_in);
            break;

        case COMMAND__SWITCH_TRANSPORT:
            AmaCommandHandlers_SwitchTransport(control_envelope_in);
            break;

        case COMMAND__GET_STATE:
            AmaCommandHandlers_GetState(control_envelope_in);
            break;

        case COMMAND__SET_STATE:
            AmaCommandHandlers_SetState(control_envelope_in);
            break;

        case  COMMAND__ISSUE_MEDIA_CONTROL:
            AmaCommandHandlers_MediaControl(control_envelope_in);
            break;

        case COMMAND__OVERRIDE_ASSISTANT:
            AmaCommandHandlers_OverrideAssistant(control_envelope_in);
            break;

        case COMMAND__NOTIFY_SPEECH_STATE:
            AmaCommandHandlers_NotifySpeechState(control_envelope_in);
            break;

        case COMMAND__STOP_SPEECH:
            AmaCommandHandlers_StopSpeech(control_envelope_in);
            break;

        case COMMAND__PROVIDE_SPEECH:
            AmaCommandHandlers_ProvideSpeech(control_envelope_in);
            break;

        case COMMAND__ENDPOINT_SPEECH:
            AmaCommandHandlers_EndpointSpeech(control_envelope_in);
            break;

        case COMMAND__FORWARD_AT_COMMAND:
            AmaCommandHandlers_ForwardATCommand(control_envelope_in);
            break;

        case COMMAND__SYNCHRONIZE_SETTINGS:
            AmaCommandHandlers_SynchronizeSettings(control_envelope_in);
            break;

        case COMMAND__KEEP_ALIVE:
            AmaCommandHandlers_KeepAlive(control_envelope_in);
            break;

        case COMMAND__SYNCHRONIZE_STATE:
            AmaCommandHandlers_SynchronizeState(control_envelope_in);
            break;

        case COMMAND__NONE:
             break;

        case COMMAND__START_SPEECH:
        case COMMAND__GET_CENTRAL_INFORMATION:
        case COMMAND__NOTIFY_DEVICE_CONFIGURATION:
        case COMMAND__RESET_CONNECTION:
        default:
            AmaCommandHandlers_NotHandled(control_envelope_in);
            break;
    }
}

static void amaReceive_HandleResponse(ControlEnvelope *control_envelope_in)
{
    Command command = control_envelope_in->command;

    switch(command)
    {
        case COMMAND__START_SPEECH:
            AmaSendCommand_HandleResponseStartSpeech(control_envelope_in);
            break;

        case COMMAND__GET_CENTRAL_INFORMATION:
            AmaSendCommand_HandleResponseGetCentralInformation(control_envelope_in);
            break;

        case COMMAND__STOP_SPEECH:
        case COMMAND__ENDPOINT_SPEECH:
        case COMMAND__INCOMING_CALL:
        case COMMAND__RESET_CONNECTION:
        case COMMAND__KEEP_ALIVE:
        case COMMAND__SYNCHRONIZE_STATE:
        case COMMAND__NONE:
        default:
            AmaSendCommand_HandleResponseNotHandled(control_envelope_in);
            break;
    }
    return;
}

