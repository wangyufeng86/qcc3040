/*****************************************************************************
Copyright (c) 2018 Qualcomm Technologies International, Ltd.

FILE NAME
    ama_send_command.h

DESCRIPTION
    Functions to send commands to the phone.
    Response functions are in a seperate header.
*/
#ifndef _AMA_SEND_COMMAND_H
#define _AMA_SEND_COMMAND_H

#include <csrtypes.h>
#include "ama_private.h"
#include "speech.pb-c.h"
#include "accessories.pb-c.h"

/***************************************************************************
DESCRIPTION
    Send COMMAND__START_SPEECH to phone
*/
void AmaSendCommand_StartSpeech(SpeechInitiator__Type speech_initiator,
                                             AudioProfile audio_profile,
                                             AudioFormat audio_format,
                                             AudioSource audio_source,
                                             uint32 start_sample,
                                             uint32 end_sample);

/***************************************************************************
DESCRIPTION
    Handler function for COMMAND__START_SPEECH response
 
PARAMS
    control_envelope_in response data
*/
void AmaSendCommand_HandleResponseStartSpeech(ControlEnvelope *control_envelope_in);

/***************************************************************************
DESCRIPTION
    Send COMMAND__STOP_SPEECH to phone
*/
void AmaSendCommand_StopSpeech(ErrorCode reason);

/***************************************************************************
DESCRIPTION
    Send COMMAND__ENDPOINT_SPEECH to phone
*/
void AmaSendCommand_EndSpeech(void);

/***************************************************************************
DESCRIPTION
    Send COMMAND__INCOMING_CALL to phone
 
PARAMS
    caller_number Pointer null terminated string containing the callers number
*/
void AmaSendCommand_IncomingCall(char* caller_number);

/***************************************************************************
DESCRIPTION
    Send COMMAND__KEEP_ALIVE to phone
*/
void AmaSendCommand_KeepAlive(void);

/***************************************************************************
DESCRIPTION
    Send COMMAND__SYNCHRONIZE_STATE to phone
 
PARAMS
    feature AMA_Feature to be synced
    value_case AmaState_ValueCase of integer
    integer value to set feature to
*/
void AmaSendCommand_SyncState(uint32 feature, ama_state_value_case_t value_case, uint16 integer);

/***************************************************************************
DESCRIPTION
    Send COMMAND__GET_STATE to phone
 
PARAMS
    feature AMA_Feature requested
*/
void AmaSendCommand_GetState(uint32 feature);

/***************************************************************************
DESCRIPTION
    Send COMMAND__RESET_CONNECTION to phone
 
PARAMS
    timeout timeout period
    force_disconnect force disconnection
*/
void AmaSendCommand_ResetConnection(uint32 timeout, bool force_disconnect);

/***************************************************************************
DESCRIPTION
    Send COMMAND__GET_CENTRAL_INFORMATION to phone
*/
void AmaSendCommand_GetCentralInformation(void);

/***************************************************************************
DESCRIPTION
    Handler function for COMMAND__GET_CENTRAL_INFORMATION response
 
PARAMS
    control_envelope_in response data
*/
void AmaSendCommand_HandleResponseGetCentralInformation(ControlEnvelope *control_envelope_in);


/***************************************************************************
DESCRIPTION
    Handler function for responses not handled by other handlers
 
PARAMS
    control_envelope_in response data
*/
void AmaSendCommand_HandleResponseNotHandled(ControlEnvelope *control_envelope_in);

/***************************************************************************
DESCRIPTION
    Function to send response of COMMAND__PROVIDE_SPEECH to phone
 
PARAMS
    accept accessory accepted/reject the request
    resp_id the new dialog id
*/
void AmaSendCommand_SendProvideSpeechResponse(bool accept, uint32 resp_id);

/***************************************************************************
DESCRIPTION
    Send COMMAND__NOTIFY_DEVICE_CONFIGURATION to phone

PARAMS
    require_va_override TRUE if assistant override is required, FALSE otherwise
*/
void AmaSendCommand_NotifyDeviceConfig(bool require_va_override);

#endif /* _AMA_SEND_COMMAND_H */

