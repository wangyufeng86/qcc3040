/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Kymera module to handle Voice Assistant related internal APIs

*/

#include "kymera_va.h"
#include "kymera_va_handlers.h"
#include "kymera_va_encode_chain.h"
#include "kymera_va_common.h"
#include <logging.h>

#define MAX_NUMBER_OF_ACTIONS 8

#define NO_ACTIONS {NULL}
#define ADD_ACTIONS(...) {__VA_ARGS__}

#define ADD_TRANSITIONS(state_transitions_array) ARRAY_DIM(state_transitions_array), state_transitions_array

typedef enum
{
    va_idle,
    va_live_capturing,
    va_wuw_detecting,
    va_wuw_detected,
    va_wuw_capturing,
    va_wuw_capturing_detect_pending,
    va_live_capturing_detect_pending
} va_states_t;

typedef enum
{
    live_capture_start,
    wuw_capture_start,
    capture_stop,
    wuw_detect_start,
    wuw_detect_stop,
    wuw_detected,
    wuw_ignore_detected
} va_events_t;

typedef void (* enterStateAction)(const void *event_params);

typedef struct
{
    va_events_t      event;
    va_states_t      new_state;
    enterStateAction actions[MAX_NUMBER_OF_ACTIONS];
} state_transition_t;

typedef struct
{
    va_states_t               state_id;
    unsigned                  is_capture_active;
    unsigned                  is_wuw_active;
    unsigned                  number_of_state_transitions;
    const state_transition_t *state_transitions;
} state_t;

static const state_transition_t idle_state_transitions[] =
{
    {live_capture_start, va_live_capturing, ADD_ACTIONS(Kymera_CreateMicChainForLiveCapture, Kymera_UpdateAudioFrameworkConfig, Kymera_CreateEncodeChainForLiveCapture,
                                                        Kymera_StartEncodeChain, Kymera_StartMicChain)},
    {wuw_detect_start,   va_wuw_detecting,  ADD_ACTIONS(Kymera_CreateMicChainForWuw, Kymera_UpdateAudioFrameworkConfig, Kymera_CreateWuwChain, Kymera_BufferMicChainEncodeOutput,
                                                        Kymera_StartWuwChain, Kymera_StartMicChain, Kymera_ActivateMicChainWuwOutput, Kymera_StartGraphManagerDelegation)},
};

static const state_transition_t live_capturing_state_transitions[] =
{
    {capture_stop, va_idle, ADD_ACTIONS(Kymera_StopMicChain, Kymera_StopEncodeChain, Kymera_DestroyEncodeChain, Kymera_UpdateAudioFrameworkConfig, Kymera_DestroyMicChain)},
};

static const state_transition_t wuw_detecting_state_transitions[] =
{
    {live_capture_start, va_live_capturing_detect_pending, ADD_ACTIONS(Kymera_StopGraphManagerDelegation, Kymera_DeactivateMicChainWuwOutput, Kymera_StopWuwChain,
                                                                       Kymera_DeactivateMicChainEncodeOutput, Kymera_UpdateAudioFrameworkConfig, Kymera_CreateEncodeChainForLiveCapture,
                                                                       Kymera_StartEncodeChain, Kymera_ActivateMicChainEncodeOutputForLiveCapture)},
    {wuw_detect_stop,    va_idle,                          ADD_ACTIONS(Kymera_StopGraphManagerDelegation, Kymera_StopMicChain, Kymera_StopWuwChain, Kymera_DestroyWuwChain,
                                                                       Kymera_UpdateAudioFrameworkConfig, Kymera_DestroyMicChain)},
    {wuw_detected,       va_wuw_detected,                  ADD_ACTIONS(Kymera_StopGraphManagerDelegation, Kymera_DeactivateMicChainWuwOutput)},
};

static const state_transition_t wuw_detected_state_transitions[] =
{
    {wuw_capture_start,   va_wuw_capturing_detect_pending, ADD_ACTIONS(Kymera_StopWuwChain, Kymera_UpdateAudioFrameworkConfig, Kymera_CreateEncodeChainForWuwCapture,
                                                                       Kymera_StartEncodeChain, Kymera_ActivateMicChainEncodeOutputForWuwCapture)},
    {wuw_ignore_detected, va_wuw_detecting,                ADD_ACTIONS(Kymera_ActivateMicChainWuwOutput, Kymera_StartGraphManagerDelegation)},
};

static const state_transition_t wuw_capturing_state_transitions[] =
{
    {capture_stop, va_idle, ADD_ACTIONS(Kymera_StopMicChain, Kymera_StopEncodeChain, Kymera_DestroyEncodeChain, Kymera_UpdateAudioFrameworkConfig, Kymera_DestroyMicChain)},
};

static const state_transition_t wuw_capturing_detect_pending_state_transitions[] =
{
    {capture_stop,    va_wuw_detecting, ADD_ACTIONS(Kymera_DeactivateMicChainEncodeOutput, Kymera_StopEncodeChain, Kymera_DestroyEncodeChain, Kymera_UpdateAudioFrameworkConfig,
                                                    Kymera_BufferMicChainEncodeOutput, Kymera_StartWuwChain, Kymera_ActivateMicChainWuwOutput, Kymera_StartGraphManagerDelegation)},
    {wuw_detect_stop, va_wuw_capturing, ADD_ACTIONS(Kymera_UpdateAudioFrameworkConfig, Kymera_DestroyWuwChain)},
};

static const state_transition_t live_capturing_detect_pending_state_transitions[] =
{
    {capture_stop,    va_wuw_detecting, ADD_ACTIONS(Kymera_DeactivateMicChainEncodeOutput, Kymera_StopEncodeChain, Kymera_DestroyEncodeChain, Kymera_UpdateAudioFrameworkConfig,
                                                    Kymera_BufferMicChainEncodeOutput, Kymera_StartWuwChain, Kymera_ActivateMicChainWuwOutput, Kymera_StartGraphManagerDelegation)},
    {wuw_detect_stop, va_wuw_capturing, ADD_ACTIONS(Kymera_UpdateAudioFrameworkConfig, Kymera_DestroyWuwChain)},
};

static const state_t states[] =
{
/*  {state,                            is_capture_active, is_wuw_active, state_transitions */
    {va_idle,                          FALSE,             FALSE,         ADD_TRANSITIONS(idle_state_transitions)},
    {va_live_capturing,                TRUE,              FALSE,         ADD_TRANSITIONS(live_capturing_state_transitions)},
    {va_wuw_detecting,                 FALSE,             TRUE,          ADD_TRANSITIONS(wuw_detecting_state_transitions)},
    {va_wuw_detected,                  FALSE,             TRUE,          ADD_TRANSITIONS(wuw_detected_state_transitions)},
    {va_wuw_capturing,                 TRUE,              FALSE,         ADD_TRANSITIONS(wuw_capturing_state_transitions)},
    {va_wuw_capturing_detect_pending,  TRUE,              FALSE,         ADD_TRANSITIONS(wuw_capturing_detect_pending_state_transitions)},
    {va_live_capturing_detect_pending, TRUE,              FALSE,         ADD_TRANSITIONS(live_capturing_detect_pending_state_transitions)},
};

static va_states_t current_state = va_idle;

static const state_t * kymera_GetStateInfo(va_states_t state)
{
    unsigned i;
    for(i = 0; i < ARRAY_DIM(states); i++)
    {
        if (states[i].state_id == state)
            return &states[i];
    }
    return NULL;
}

static const state_transition_t * kymera_GetStateTransition(va_states_t state, va_events_t event)
{
    unsigned i;
    const state_t *state_info = kymera_GetStateInfo(state);

    if (state_info)
    {
        for(i = 0; i < state_info->number_of_state_transitions; i++)
        {
            if (state_info->state_transitions[i].event == event)
            {
                return &state_info->state_transitions[i];
            }
        }
    }

    return NULL;
}

static void kymera_ExecuteActions(const void *event_params, const enterStateAction *actions, unsigned number_of_actions)
{
    unsigned i;
    for(i = 0; i < number_of_actions; i++)
    {
        if (actions[i])
        {
            actions[i](event_params);
        }
    }
}

static void kymera_UpdateState(va_events_t event, const void *event_params)
{
    const state_transition_t *transition = kymera_GetStateTransition(current_state, event);

    if (transition)
    {
        /* Updating the state first since the new state must be used for any DSP clock adjustments */
        current_state = transition->new_state;
        kymera_ExecuteActions(event_params, transition->actions, ARRAY_DIM(transition->actions));
    }
    else
    {
        PANIC("kymera_UpdateState: No transition for current_state = %d - event = %d", current_state, event);
    }
}

Source Kymera_StartVaLiveCapture(const va_audio_voice_capture_params_t *params)
{
    DEBUG_LOG("Kymera_StartVaLiveCapture");
    kymera_UpdateState(live_capture_start, params);
    return PanicNull(Kymera_GetVaEncodeChainOutput());
}

void Kymera_StopVaCapture(void)
{
    DEBUG_LOG("Kymera_StopVaCapture");
    kymera_UpdateState(capture_stop, NULL);
}

void Kymera_StartVaWuwDetection(Task wuw_detection_handler, const va_audio_wuw_detection_params_t *params)
{
    wuw_detection_start_t wuw_params = {.handler = wuw_detection_handler, .params = params};
    DEBUG_LOG("Kymera_StartVaWuwDetection");
    kymera_UpdateState(wuw_detect_start, &wuw_params);
}

void Kymera_StopVaWuwDetection(void)
{
    DEBUG_LOG("Kymera_StopVaWuwDetection");
    kymera_UpdateState(wuw_detect_stop, NULL);
}

void Kymera_VaWuwDetected(void)
{
    DEBUG_LOG("Kymera_VaWuwDetected");
    kymera_UpdateState(wuw_detected, NULL);
}

Source Kymera_StartVaWuwCapture(const va_audio_wuw_capture_params_t *params)
{
    DEBUG_LOG("Kymera_StartVaWuwCapture");
    kymera_UpdateState(wuw_capture_start, params);
    return PanicNull(Kymera_GetVaEncodeChainOutput());
}

void Kymera_IgnoreDetectedVaWuw(void)
{
    DEBUG_LOG("Kymera_IgnoreDetectedVaWuw");
    kymera_UpdateState(wuw_ignore_detected, NULL);
}

bool Kymera_IsVaCaptureActive(void)
{
    return kymera_GetStateInfo(current_state)->is_capture_active;
}

bool Kymera_IsVaWuwDetectionActive(void)
{
    return kymera_GetStateInfo(current_state)->is_wuw_active;
}
