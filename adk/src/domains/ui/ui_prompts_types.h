#ifndef UI_PROMPTS_TYPES_H
#define UI_PROMPTS_TYPES_H

#include <csrtypes.h>
#include <kymera.h>

/*! Audio prompt configuration */
typedef struct
{
    const char *filename;                       /*!< Prompt filename */
    uint32 rate;                                /*!< Prompt sample rate */
    promptFormat format;                        /*!< Prompt format */
    unsigned interruptible:1;                   /*!< Determines whether the audio prompt can be interrupted during playback */
    unsigned queueable:1;                       /*!< Determines whether the audio prompt can be queued prior to playback */

} ui_prompt_data_t;

#endif // UI_PROMPTS_TYPES_H
