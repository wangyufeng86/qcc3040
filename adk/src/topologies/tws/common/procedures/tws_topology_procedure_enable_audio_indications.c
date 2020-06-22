/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      File containing a procedure that controls whether the playback of audio
            indications is enabled.

            The procedure takes a parameter that decides whether the playback of audio
            indications is to be enabled or disabled.
*/

#include "tws_topology_procedure_enable_audio_indications.h"
#include "tws_topology_procedures.h"

#include <ui_prompts.h>
#include <ui_tones.h>

#include <logging.h>
#include <message.h>


/*! Parameter definition for audio indications enable */
const ENABLE_AUDIO_INDICATIONS_PARAMS_T proc_enable_audio_indications_enable = { .enable = TRUE };
/*! Parameter definition for audio indications disable */
const ENABLE_AUDIO_INDICATIONS_PARAMS_T proc_enable_audio_indications_disable = { .enable = FALSE };

static void TwsTopology_ProcedureEnableAudioIndicationsStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data);
static void TwsTopology_ProcedureEnableAudioIndicationsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn);

const procedure_fns_t proc_enable_audio_indications_fns = {
    TwsTopology_ProcedureEnableAudioIndicationsStart,
    TwsTopology_ProcedureEnableAudioIndicationsCancel,
};

typedef struct
{
    ENABLE_AUDIO_INDICATIONS_PARAMS_T params;
} twsTopProcEnableAudioIndicationsTaskData;

twsTopProcEnableAudioIndicationsTaskData twstop_proc_enable_audio_indications = { 0 };

#define TwsTopProcEnableConnectableHandsetGetTaskData()     (&twstop_proc_enable_audio_indications)


static void TwsTopology_ProcedureEnableAudioIndicationsStart(Task result_task,
                                        procedure_start_cfm_func_t proc_start_cfm_fn,
                                        procedure_complete_func_t proc_complete_fn,
                                        Message goal_data)
{
    twsTopProcEnableAudioIndicationsTaskData* td = TwsTopProcEnableConnectableHandsetGetTaskData();
    ENABLE_AUDIO_INDICATIONS_PARAMS_T* params = (ENABLE_AUDIO_INDICATIONS_PARAMS_T*)goal_data;

    UNUSED(result_task);

    td->params = *params;

    /* start the procedure */
    DEBUG_LOG("TwsTopology_ProcedureEnableAudioIndicationsStart Enabled=%d", params->enable);

    UiTones_SetTonePlaybackEnabled(params->enable);
    UiPrompts_SetPromptPlaybackEnabled(params->enable);

    proc_start_cfm_fn(tws_topology_procedure_enable_audio_indications, procedure_result_success);

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCompleteCfmCallback(proc_complete_fn,
                                          tws_topology_procedure_enable_audio_indications,
                                          procedure_result_success);
}

static void TwsTopology_ProcedureEnableAudioIndicationsCancel(procedure_cancel_cfm_func_t proc_cancel_cfm_fn)
{
    twsTopProcEnableAudioIndicationsTaskData* td = TwsTopProcEnableConnectableHandsetGetTaskData();

    DEBUG_LOG("TwsTopology_ProcedureEnableAudioIndicationsCancel Enabled=%d", td->params.enable);

    UiTones_SetTonePlaybackEnabled(!td->params.enable);
    UiPrompts_SetPromptPlaybackEnabled(!td->params.enable);

    /* must use delayed cfm callback to prevent script engine recursion */
    Procedures_DelayedCancelCfmCallback(proc_cancel_cfm_fn,
                                        tws_topology_procedure_enable_audio_indications,
                                        procedure_result_success);
}

