/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file converts System Events to corresponding Audio Tone UI Events
            by table look-up, using a configuration table passed in by the Application.
            It then plays these Tones when required using the Kymera audio framework
            Aux path.
*/
#ifndef UI_TONES_H
#define UI_TONES_H

#include <ui_indicator.h>

#include <domain_message.h>
#include <csrtypes.h>
#include <message.h>
#include <kymera.h>
#include <task_list.h>
#include <rtime.h>

/*! \brief ui_tone task structure */
typedef struct
{
    /*! The task. */
    TaskData task;

    /*! The configuration table of System Event to tones to play, passed from the Application. */
    const ui_event_indicator_table_t * sys_event_to_tone_data_mappings;
    uint8 mapping_table_size;

    /*! Repeating tone reminders configuration table. */
    const ui_repeating_indication_table_t * configured_reminder_tone_mappings;
    uint8 reminder_table_size;

    /*! Used to track which repeating reminder indications are active so if multiple are ongoing,
        these can be rescheduled in the event that one is cancelled. */
    unsigned active_reminders;

    /*! Used to trigger a conditional message send when the current requested tone has completed playback. */
    uint16 tone_playback_ongoing_mask;

    /*! Used to gate audio tones and prevent them from being played. */
    unsigned tone_playback_enabled :1;

    /*! Needed to indicate back to the Power Manager UI tone completion, if a tone was configured
        to be played on Power Off. */
    unsigned indicate_when_power_shutdown_prepared :1;

    /*! Specifies whether the UI Indicator should generate UI Inidications from System Event messages
    received at the task handler. \note this is not the same as whether indication playback is enabled. */
    unsigned generate_ui_events :1;

} ui_tones_task_data_t;

/*! \brief Set/reset play_tone flag. This is flag is used to check if tones can be played or not.
  application will set and reset the flag. */
void UiTones_SetTonePlaybackEnabled(bool play_tone);

/*! \brief Get the pointer to UI Tones Task */
Task UiTones_GetUiTonesTask(void);

/*! \brief Initialise UI Tones module */
bool UiTones_Init(Task init_task);

/*! \brief De-initialise UI Tones module */
bool UiTones_DeInit(void);

/*! \brief Used by the Application to set the mapping of System Events to Audio Tones. */
void UiTones_SetToneConfiguration(const ui_event_indicator_table_t * table,
                                  uint8 size,
                                  const ui_repeating_indication_table_t * reminder_table,
                                  uint8 reminder_size);

/*! \brief Used by the UI component to notify the Audio Tones UI Indicator of a UI event. */
void UiTones_NotifyUiIndication(uint16 tone_index, rtime_t time_to_play);

/*! brief Used by the Application to control whether the UI Indicator shall generate UI events. */
void UiTones_GenerateUiEvents(bool generate);

#endif // UI_TONES_H
