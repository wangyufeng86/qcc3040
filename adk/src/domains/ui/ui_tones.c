/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Source file converts System Events to corresponding Audio Tone UI Events
            by table look-up, using a configuration table passed in by the Application.
            It then plays these tones when required using the Kymera audio framework
            Aux path.
*/

#include "ui_tones.h"
#include "ui_tones_private.h"

#include "ui_inputs.h"
#include "pairing.h"
#include "ui.h"
#include "av.h"
#include <power_manager.h>
#include <phy_state.h>

#include <domain_message.h>
#include <logging.h>
#include <message.h>
#include <panic.h>

#include <stdlib.h>

#include "system_clock.h"

ui_tones_task_data_t the_tones;

static bool uiTones_IsRepeatingIndication(uint16 tone_index)
{
    return !!(tone_index & REPEATING_INDICATION_MASK);
}

static bool uiTones_GetToneIndexFromMappingTable(MessageId id, uint16 * tone_index)
{
    return UiIndicator_GetIndexFromMappingTable(
                the_tones.sys_event_to_tone_data_mappings,
                the_tones.mapping_table_size,
                id,
                tone_index);
}

static bool uiTones_GetStartToneIndexFromReminderMappingTable(MessageId id, uint16 * tone_index)
{
    bool event_found = FALSE;
    event_found = UiIndicator_GetStartIndexFromReminderMappingTable(
                    the_tones.configured_reminder_tone_mappings,
                    the_tones.reminder_table_size,
                    id,
                    tone_index);
    if (event_found)
    {
        *tone_index |= REPEATING_INDICATION_MASK;
    }
    return event_found;
}

static bool uiTones_GetCancelToneIndexFromReminderMappingTable(MessageId id, uint16 * tone_index)
{
    return UiIndicator_GetCancelIndexFromReminderMappingTable(
                the_tones.configured_reminder_tone_mappings,
                the_tones.reminder_table_size,
                id,
                tone_index);
}

static const ui_tone_data_t* uiTones_GetToneData(uint16 tone_index)
{
    const ui_tone_data_t *config = NULL;
    if (uiTones_IsRepeatingIndication(tone_index))
    {
        tone_index &= ~REPEATING_INDICATION_MASK;
        PanicFalse(tone_index < the_tones.reminder_table_size);
        config = &UiIndicator_GetDataForReminderIndex(
                    the_tones.configured_reminder_tone_mappings,
                    the_tones.reminder_table_size,
                    tone_index)->tone;
    }
    else
    {
        PanicFalse(tone_index < the_tones.mapping_table_size);
        config = &UiIndicator_GetDataForIndex(
                    the_tones.sys_event_to_tone_data_mappings,
                    the_tones.mapping_table_size,
                    tone_index)->tone;
    }
    return config;
}

/*! \brief Play tone.

    \param tone_index The tone to play from the mappings table.
    \param time_to_play The microsecond at which to begin mixing of this audio tone.
*/
static void uiTones_PlayTone(uint16 tone_index, rtime_t time_to_play, const ui_tone_data_t *config)
{
    DEBUG_LOG("uiTones_PlayTone index=%04x ttp=%d enabled=%d",
              tone_index, time_to_play, the_tones.tone_playback_enabled );

    if (the_tones.tone_playback_enabled)
    {
        uint16 *client_lock = NULL;
        uint16 client_lock_mask = 0;

        if (!uiTones_IsRepeatingIndication(tone_index))
        {
            UiIndicator_ScheduleIndicationCompletedMessage(
                    the_tones.sys_event_to_tone_data_mappings,
                    the_tones.mapping_table_size,
                    tone_index,
                    UI_INTERNAL_TONE_PLAYBACK_COMPLETED,
                    &the_tones.task,
                    &the_tones.tone_playback_ongoing_mask,
                    &client_lock,
                    &client_lock_mask);
        }

        appKymeraTonePlay(config->tone, time_to_play, config->interruptible, client_lock, client_lock_mask);
    }
}

static inline void uiTones_SetActiveReminder(uint16 tone_index)
{
    the_tones.active_reminders |= 0x1 << tone_index;
}

static inline void uiTones_ClearActiveReminder(uint16 tone_index)
{
    the_tones.active_reminders &= ~(0x1 << tone_index);
}

static void uiTones_SchedulePlay(uint16 tone_index)
{
    /* Factor in the propagation latency through the various buffers for the aux channel and the time
       to start the file source */
    rtime_t time_now = SystemClockGetTimerTime();
    const ui_tone_data_t *config = uiTones_GetToneData(tone_index);

    if (config->queueable || (!appKymeraIsTonePlaying() && (the_tones.tone_playback_ongoing_mask == 0)))
    {
        rtime_t time_to_play = time_now;

        if (!config->button_feedback)
        {
            time_to_play = rtime_add(time_now, UI_INDICATOR_DELAY_FOR_SYNCHRONISED_TTP_IN_MICROSECONDS);
            time_to_play = Ui_RaiseUiEvent(ui_indication_type_audio_tone, tone_index, time_to_play);
        }

        uiTones_PlayTone(tone_index, time_to_play, config);
    }
}

static void uiTones_ScheduleReminder(uint16 tone_index)
{
    uint16 table_index = tone_index & ~REPEATING_INDICATION_MASK;
    MESSAGE_MAKE(msg, UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY_T);
    msg->reminder_index = tone_index;
    MessageSendLater(&the_tones.task,
                     UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY,
                     msg,
                     D_SEC(the_tones.configured_reminder_tone_mappings[table_index].reminder_period));
    uiTones_SetActiveReminder(table_index);
}

static void uiTones_RescheduleActiveReminders(void)
{
    unsigned to_restart = the_tones.active_reminders;
    uint8 tone_index = 0;
    while (to_restart != 0)
    {
        if (to_restart & 0x1)
        {
            uiTones_ScheduleReminder(tone_index);
        }
        to_restart >>= 1;
        tone_index++;
    }
}

static void uiTones_HandleMessage(Task task, MessageId id, Message message)
{
    uint16 tone_index = 0;

    UNUSED(task);

    DEBUG_LOG("uiTones_HandleMessage Id=%04x", id);

    bool tone_found = uiTones_GetToneIndexFromMappingTable(id, &tone_index);
    if (tone_found)
    {
        if (the_tones.generate_ui_events)
        {
            uiTones_SchedulePlay(tone_index);
        }
        else
        {
            /* Tones that need to be indicated before shutdown should always be played,
               regardless of whether we are rendering indications based on the current
               device topology role and any other gating factors. */
            ui_event_indicator_table_t tone_to_play = the_tones.sys_event_to_tone_data_mappings[tone_index];
            bool indicate_on_shutdown = tone_to_play.await_indication_completion;
            bool button_press_feedback = tone_to_play.data.tone.button_feedback;
            if (indicate_on_shutdown || button_press_feedback)
            {
                uiTones_SchedulePlay(tone_index);
            }
        }
    }
    if (uiTones_GetStartToneIndexFromReminderMappingTable(id, &tone_index))
    {
        DEBUG_LOG("schedule UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY index=%x", tone_index);
        uiTones_ScheduleReminder(tone_index);
    }
    if (uiTones_GetCancelToneIndexFromReminderMappingTable(id, &tone_index))
    {
        DEBUG_LOG("cancel UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY index=%x", tone_index);
        MessageCancelAll(&the_tones.task,
                           UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY);
        uiTones_ClearActiveReminder(tone_index);
        uiTones_RescheduleActiveReminders();
    }

    if (id == UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY)
    {
        tone_index = ((UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY_T *)message)->reminder_index;

        DEBUG_LOG("UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY reminder_index=%x", tone_index);
        uiTones_SchedulePlay(tone_index);
        uiTones_ScheduleReminder(tone_index);
    }
    else if (id == UI_INTERNAL_TONE_PLAYBACK_COMPLETED)
    {
        DEBUG_LOG("UI_INTERNAL_TONE_PLAYBACK_COMPLETED indicate=%d", the_tones.indicate_when_power_shutdown_prepared);
        if (the_tones.indicate_when_power_shutdown_prepared)
        {
            appPowerShutdownPrepareResponse(&the_tones.task);
        }
    }
    else if (id == APP_POWER_SHUTDOWN_PREPARE_IND)
    {
        DEBUG_LOG("uiTones_HandleMessage APP_POWER_SHUTDOWN_PREPARE_IND");
        the_tones.indicate_when_power_shutdown_prepared = TRUE;
    }
    else if (id == APP_POWER_SLEEP_PREPARE_IND)
    {
        appPowerSleepPrepareResponse(&the_tones.task);
    }
    else
    {
        // Ignore message
    }
}

/*! \brief brief Set/reset play_tone flag. This is flag is used to check if tones
  can be played or not. Application will set and reset the flag. Scenarios like earbud
  is in ear or not and etc.

    \param play_tone If TRUE, tone can be played, if FALSE, the tone can not be
    played.
*/
void UiTones_SetTonePlaybackEnabled(bool play_tone)
{
    the_tones.tone_playback_enabled = play_tone;
}

Task UiTones_GetUiTonesTask(void)
{
    return &the_tones.task;
}

void UiTones_SetToneConfiguration(const ui_event_indicator_table_t * table,
                                  uint8 size,
                                  const ui_repeating_indication_table_t * reminder_table,
                                  uint8 reminder_size)
{
    the_tones.sys_event_to_tone_data_mappings = table;
    the_tones.mapping_table_size = size;
    the_tones.configured_reminder_tone_mappings = reminder_table;
    the_tones.reminder_table_size = reminder_size;

    UiIndicator_RegisterInterestInConfiguredSystemEvents(
                the_tones.sys_event_to_tone_data_mappings,
                the_tones.mapping_table_size,
                &the_tones.task);

    UiIndicator_RegisterInterestInConfiguredReminderSystemEvents(
                the_tones.configured_reminder_tone_mappings,
                the_tones.reminder_table_size,
                &the_tones.task);
}

void UiTones_NotifyUiIndication(uint16 tone_index, rtime_t time_to_play)
{
    const ui_tone_data_t *config = uiTones_GetToneData(tone_index);
    uiTones_PlayTone(tone_index, time_to_play, config);
}

/*! brief Initialise UI Tones module */
bool UiTones_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("UiTones_Init");

    memset(&the_tones, 0, sizeof(ui_tones_task_data_t));

    the_tones.task.handler = uiTones_HandleMessage;
    the_tones.generate_ui_events = TRUE;
    the_tones.tone_playback_enabled = TRUE;

    return TRUE;
}

/*! brief de-initialise UI Tones module */
bool UiTones_DeInit(void)
{
    DEBUG_LOG("UiTones_DeInit");

    the_tones.sys_event_to_tone_data_mappings = NULL;
    the_tones.mapping_table_size = 0;

    return TRUE;
}

void UiTones_GenerateUiEvents(bool generate)
{
    the_tones.generate_ui_events = generate;
}
