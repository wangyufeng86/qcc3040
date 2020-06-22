/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the voice audio manager state machine
*/

#include "voice_audio_manager_sm.h"

typedef struct
{
    states_t state;
    events_t event;
    states_t new_state;
} state_transition_t;

static const state_transition_t state_transitions[] =
{
    {idle, live_capture_start, live_capturing},
    {idle, wuw_detect_start,   wuw_detecting},

    {live_capturing, capture_stop, idle},

    {wuw_detecting, live_capture_start, live_capturing_detect_pending},
    {wuw_detecting, wuw_capture_start, wuw_capturing_detect_pending},
    {wuw_detecting, wuw_detect_stop, idle},

    {live_capturing_detect_pending, capture_stop, wuw_detecting},
    {live_capturing_detect_pending, wuw_detect_stop, live_capturing},

    {wuw_capturing_detect_pending, capture_stop, wuw_detecting},
    {wuw_capturing_detect_pending, wuw_detect_stop, wuw_capturing},

    {wuw_capturing, capture_stop, idle},
};

static states_t current_state = idle;

static const state_transition_t * voiceAudioManager_GetStateTransition(states_t state, events_t event)
{
    unsigned i;
    for(i = 0; i < ARRAY_DIM(state_transitions); i++)
    {
        if ((state_transitions[i].state == state) && (state_transitions[i].event == event))
        {
            return &state_transitions[i];
        }
    }

    return NULL;
}

bool VoiceAudioManager_UpdateState(events_t event)
{
    bool status = FALSE;
    const state_transition_t *transition = voiceAudioManager_GetStateTransition(current_state, event);

    if (transition)
    {
        current_state = transition->new_state;
        status = TRUE;
    }

    return status;
}

bool VoiceAudioManager_IsMicDataExpected(void)
{
    return (current_state == live_capturing) || (current_state == live_capturing_detect_pending) ||
           (current_state == wuw_capturing_detect_pending) || (current_state == wuw_capturing);
}

bool VoiceAudioManager_IsWuwDetectionExpected(void)
{
    return (current_state == wuw_detecting);
}

void VoiceAudioManager_SmTestReset(void)
{
    current_state = idle;
}
