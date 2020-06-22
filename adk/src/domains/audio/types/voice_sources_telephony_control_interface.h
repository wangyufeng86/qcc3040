/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Interface to voice_sources_telephony_control - provides an interface that can be used to
            access the telephony features of a voice source.
*/

#ifndef VOICE_SOURCES_TELEPHONY_CONTROL_INTERFACE_H_
#define VOICE_SOURCES_TELEPHONY_CONTROL_INTERFACE_H_

#include "voice_sources_list.h"

typedef struct
{
    uint8 * digits;
    unsigned number_of_digits;
} phone_number_t;

typedef struct
{
    void (*IncomingCallAccept)(voice_source_t source);
    void (*IncomingCallReject)(voice_source_t source);
    void (*OngoingCallTerminate)(voice_source_t source);
    void (*OngoingCallTransferAudioToAg)(voice_source_t source);
    void (*OngoingCallTransferAudioToSelf)(voice_source_t source);
    void (*InitiateCallUsingNumber)(voice_source_t source, phone_number_t number);
    void (*InitiateVoiceDial)(voice_source_t source);
} voice_source_telephony_control_interface_t;

#endif /* VOICE_SOURCES_TELEPHONY_CONTROL_INTERFACE_H_ */
