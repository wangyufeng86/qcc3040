/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice_sources_telephony_control composite.
*/

#include "voice_sources.h"

#include <panic.h>

static const voice_source_telephony_control_interface_t * telephony_control_interface[max_voice_sources];

static void voiceSources_ValidateSource(voice_source_t source)
{
    if((source <= voice_source_none) || (source >= max_voice_sources))
    {
        Panic();
    }
}

static bool voiceSources_IsSourceRegistered(voice_source_t source)
{
    return ((telephony_control_interface[source] == NULL) ? FALSE : TRUE);
}

void VoiceSources_TelephonyControlRegistryInit(void)
{
    memset(telephony_control_interface, 0, sizeof(telephony_control_interface));
}

void VoiceSources_RegisterTelephonyControlInterface(voice_source_t source, const voice_source_telephony_control_interface_t * interface)
{
    voiceSources_ValidateSource(source);
    PanicNull((void *)interface);
    telephony_control_interface[source] = interface;
}

void VoiceSources_AcceptIncomingCall(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->IncomingCallAccept))
    {
        telephony_control_interface[source]->IncomingCallAccept(source);
    }
}

void VoiceSources_RejectIncomingCall(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->IncomingCallReject))
    {
        telephony_control_interface[source]->IncomingCallReject(source);
    }
}

void VoiceSources_TerminateOngoingCall(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->OngoingCallTerminate))
    {
        telephony_control_interface[source]->OngoingCallTerminate(source);
    }
}

void VoiceSources_TransferOngoingCallAudioToAg(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->OngoingCallTransferAudioToAg))
    {
        telephony_control_interface[source]->OngoingCallTransferAudioToAg(source);
    }
}

void VoiceSources_TransferOngoingCallAudioToSelf(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->OngoingCallTransferAudioToSelf))
    {
        telephony_control_interface[source]->OngoingCallTransferAudioToSelf(source);
    }
}

void VoiceSources_InitiateCallUsingNumber(voice_source_t source, phone_number_t number)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->InitiateCallUsingNumber))
    {
        telephony_control_interface[source]->InitiateCallUsingNumber(source, number);
    }
}

void VoiceSources_InitiateVoiceDial(voice_source_t source)
{
    voiceSources_ValidateSource(source);
    if(voiceSources_IsSourceRegistered(source) && (telephony_control_interface[source]->InitiateVoiceDial))
    {
        telephony_control_interface[source]->InitiateVoiceDial(source);
    }
}
