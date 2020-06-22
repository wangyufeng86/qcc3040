/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Private messages and symbols used by the UI Tones module.
*/
#ifndef UI_TONES_PRIVATE_H
#define UI_TONES_PRIVATE_H

#define TONE_NONE                     0xFFFF
#define REPEATING_INDICATION_MASK     (0x1 << 15)

#define UI_TONES_WAIT_FOR_TONE_COMPLETION 0x1

/*! User interface internal messasges */
enum ui_internal_messages
{
    /*! Message sent later when a tone is played. Until this message is delivered
        repeat tones will not be played */
    UI_INTERNAL_TONE_PLAYBACK_COMPLETED,
    /*! Message sent later when a UI Reminder Indication is scheduled. */
    UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY
};

typedef struct
{
    unsigned reminder_index;
} UI_INTERNAL_REMINDER_INDICATION_TIMER_EXPIRY_T;

#endif // UI_TONES_PRIVATE_H
