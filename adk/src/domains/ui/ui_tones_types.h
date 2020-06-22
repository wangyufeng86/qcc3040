#ifndef UI_TONES_TYPES_H
#define UI_TONES_TYPES_H

#include <csrtypes.h>
#include <kymera.h>

/*! Audio tone configuration */
typedef struct
{
    const ringtone_note *tone;
    unsigned interruptible:1;                   /*!< Determines whether the audio tone can be interrupted during playback */
    unsigned queueable:1;                       /*!< Determines whether the audio tone can be queued prior to playback */
    unsigned button_feedback:1;                 /*!< Determines that the tone is for button press feedback to the user */

} ui_tone_data_t;

#endif // UI_TONES_TYPES_H
