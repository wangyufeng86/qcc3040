/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       ama_actions.c
\brief      Implementation for ama user events
*/
#ifdef INCLUDE_AMA
#include "ama_actions.h"
#include "ama.h"
#include "ama_audio.h"
#include "ama_data.h"
#include "ama_protocol.h"
#include "ama_speech.h"
#include "ama_rfcomm.h"
#include "voice_ui_container.h"
#include "bt_device.h"
#include <stdlib.h>
#define ama_GetActionMapping() (0)  /* "Dedicated assistant physical button (one button)" */
#define NUM_VA_TRANSLATION_TABLES ARRAY_DIM(va_translation_tables)
/* Ama Events */

typedef enum
{
    AMA_BUTTON_TAPPED,
    AMA_BUTTON_PUSHED ,
    AMA_BUTTON_RELEASED,
}ama_button_events_t;

typedef struct
{
    ui_input_t voice_assistant_user_event;
    ama_button_events_t action_event;
} va_event_translation_t;

typedef struct
{
    const va_event_translation_t* event_translations;
    unsigned num_translations;
} va_translation_table_t;

static void amaActions_SetVaActionsTranslationTable(uint8 translation_id);
static void amaActions_ActionOnEvent(ama_button_events_t event_id);
static va_translation_table_t va_translation_table;

static const va_event_translation_t one_button_va_event_translations[] =
{
    { ui_input_va_3, AMA_BUTTON_TAPPED},
    { ui_input_va_5, AMA_BUTTON_PUSHED},
    { ui_input_va_6, AMA_BUTTON_RELEASED}
};

static va_translation_table_t va_translation_tables[] =
{
    {one_button_va_event_translations,   ARRAY_DIM(one_button_va_event_translations)}
};
/************************************************************************/
void AmaActions_Init(void)
{
    amaActions_SetVaActionsTranslationTable(ama_GetActionMapping());
}
/************************************************************************/
static void amaActions_SetVaActionsTranslationTable(uint8 translation_id)
{
     if(translation_id < NUM_VA_TRANSLATION_TABLES)
        va_translation_table = va_translation_tables[translation_id];
     else
         Panic();
}
/************************************************************************/
bool AmaActions_HandleVaEvent(ui_input_t voice_assistant_user_event)
{
    ama_button_events_t action_event;
    uint16 index;
    bool handled = FALSE;
    for (index = 0; (index < va_translation_table.num_translations); index++)
    {
        if (voice_assistant_user_event == va_translation_table.event_translations[index].voice_assistant_user_event)
        {
            action_event = va_translation_table.event_translations[index].action_event;
            DEBUG_LOG("AmaActions_HandleVaEvent sending action %d", action_event);
            amaActions_ActionOnEvent(action_event);
            handled = TRUE;
        }   
    }
    return handled;
}
/************************************************************************/

static void amaActions_ActionOnEvent(ama_button_events_t event_id)
{
    DEBUG_LOG("amaActions_ActionOnEvent,Button Event receveid %d",event_id);
    
    switch(event_id)
    {
        case AMA_BUTTON_TAPPED:
            if(AmaAudio_Start(ama_audio_trigger_tap))
            {
                DEBUG_LOG("ama_audio_trigger_tap");
                AmaData_SetState(ama_state_sending);
            }
            break;
        case AMA_BUTTON_PUSHED:
            if(AmaAudio_Start(ama_audio_trigger_press))
            {
                DEBUG_LOG("ama_button_push_to_talk_start");
                AmaData_SetState(ama_state_sending);
            }
            break;
        case AMA_BUTTON_RELEASED:
            {
                DEBUG_LOG("ama_button_push_to_talk_stop");
                if(AmaData_IsSendingVoiceData())
                {
                    DEBUG_LOG("inside ama_button_push_to_talk_stop");
                    AmaAudio_End();
                    /* The following satisfies AVS requirments in case when earbud is already
                     * streaming data and it receives another speech control with a diﬀerent Dialog Id.
                     * In this case, USER_CANCELLED error must be sent. */
                    if(AmaData_GetState() == ama_state_sending)
                    {
                        DEBUG_LOG("Interrupting a previous TTT or PTT, sending USER_CANCELLED");
                        AmaSpeech_Stop();
                    }

                    AmaData_SetState(ama_state_idle);
                }
            }
            break;
        default:
            DEBUG_LOG("Unhandled button");
        break;

    }
}
#endif /* INCLUDE_AMA */
