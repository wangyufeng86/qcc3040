/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_ui.c
\brief      Configure the Earbud Application User Interface Indicators
*/

#include "earbud_ui.h"

#include "earbud_prompts_config_table.h"
#include "earbud_tones_config_table.h"
#include "earbud_leds_config_table.h"

#include <ui_prompts.h>
#include <ui_tones.h>
#include <ui_leds.h>

/*! brief Initialise indicator module */
bool EarbudUi_Init(Task init_task)
{
    UNUSED(init_task);

#if INCLUDE_PROMPTS
    UiPrompts_SetPromptConfiguration(
                earbud_ui_prompts_table,
                EarbudPromptsConfigTable_GetSize());
#endif

#if INCLUDE_TONES
    UiTones_SetToneConfiguration(
                earbud_ui_tones_table,
                EarbudTonesConfigTable_SingleGetSize(),
                earbud_ui_repeating_tones_table,
                EarbudTonesConfigTable_RepeatingGetSize());
#endif

    UiLeds_SetLedConfiguration(
                earbud_ui_leds_table,
                EarbudLedsConfigTable_EventsTableGetSize(),
                earbud_ui_leds_context_indications_table,
                EarbudLedsConfigTable_ContextsTableGetSize());

    return TRUE;
}

