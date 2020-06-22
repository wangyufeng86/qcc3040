/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file converts System Events to corresponding Audio Prompt UI Events
            by table look-up, using a configuration table passed in by the Application.
            It then plays these Prompts when required using the Kymera audio framework
            Aux path.
*/
#ifndef UI_PROMPTS_H
#define UI_PROMPTS_H

#include <ui_indicator.h>

#include <domain_message.h>
#include <csrtypes.h>
#include <message.h>
#include <kymera.h>
#include <task_list.h>
#include <rtime.h>

/*! \brief ui_prompt task structure */
typedef struct
{
    /*! The task. */
    TaskData task;

    /*! The configuration table of System Event to prompts to play, passed from the Application. */
    const ui_event_indicator_table_t * sys_event_to_prompt_data_mappings;
    uint8 mapping_table_size;

    /*! Cache of the file index of each prompt. */
    FILE_INDEX *prompt_file_indexes;

    /*! This is a hold off time for after a voice prompt is played, in milliseconds. When a voice prompt
    is played, for the period of time specified, any repeat of this prompt will not be played. If the no
    repeat period set to zero, then all prompts will be played, regardless of whether the prompt was
    recently played.*/
    Delay no_repeat_period_in_ms;

    /*! The last prompt played, used to avoid repeating prompts. */
    uint16 last_prompt_played_index;

    /*! Used to trigger a conditional message send when the current requested prompt has completed playback. */
    uint16 prompt_playback_ongoing_mask;

    /*! Used to gate audio prompts and prevent them from being played. */
    unsigned prompt_playback_enabled :1;

    /*! Needed to indicate back to the Power Manager UI Prompt completion, if a prompt was configured
    to be played on Power Off. */
    unsigned indicate_when_power_shutdown_prepared :1;

    /*! Specifies whether the UI Indicator should generate UI Inidications from System Event messages
    received at the task handler. \note this is not the same as whether indication playback is enabled. */
    unsigned generate_ui_events :1;

} ui_prompts_task_data_t;

/*! brief Set/reset play_prompt flag. This is flag is used to check if prompts can be played or not.
  application will set and reset the flag. */
void UiPrompts_SetPromptPlaybackEnabled(bool play_prompt);

/*! Get the pointer to UI Prompts Task */
Task UiPrompts_GetUiPromptsTask(void);

/*! brief Initialise UI Prompts module */
bool UiPrompts_Init(Task init_task);

/*! brief De-initialise UI Prompts module */
bool UiPrompts_DeInit(void);

/*! brief Used by the Application to set the mapping of System Events to Audio Prompts. */
void UiPrompts_SetPromptConfiguration(const ui_event_indicator_table_t * table, uint8 size);

/*! brief Used by the Application to set the back off timer for which a play of the same audio prompt will not be repeated. */
void UiPrompts_SetNoRepeatPeriod(Delay no_repeat_period_in_ms);

/*! brief Used by the UI component to notify the Audio Prompts UI Indicator of a UI event. */
void UiPrompts_NotifyUiIndication(uint16 prompt_index, rtime_t time_to_play);

/*! brief Used by the Application to control whether the UI Indicator shall generate UI events. */
void UiPrompts_GenerateUiEvents(bool generate);

#endif // UI_PROMPTS_H
