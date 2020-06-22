/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   voice_assistant container
\ingroup    Service
\brief      A component responsible for managing voice assistants.

Responsibilities:
- Service component which creates and manages handle.
*/

#ifndef VOICE_UI_CONTAINER_H_
#define VOICE_UI_CONTAINER_H_

#include "ui.h"
#include "panic.h"
#include "voice_ui.h"


/*============================================================================*
  Definitions
 *============================================================================*/

#if defined INCLUDE_GAA
#define VOICE_UI_PROVIDER_DEFAULT   voice_ui_provider_gaa
#elif defined INCLUDE_AMA
#define VOICE_UI_PROVIDER_DEFAULT   voice_ui_provider_ama
#else
#define VOICE_UI_PROVIDER_DEFAULT   voice_ui_provider_none
#endif

/*! \brief Macro that defines the maximum no of voice assistants supported in the system */
#ifdef ENABLE_AUDIO_TUNING_MODE
    #ifdef INCLUDE_VA_COEXIST
        #define MAX_NO_VA_SUPPORTED     (3)
    #else
        #define MAX_NO_VA_SUPPORTED     (2)
    #endif
#else
    #ifdef INCLUDE_VA_COEXIST
        #define MAX_NO_VA_SUPPORTED     (2)
    #else
        #define MAX_NO_VA_SUPPORTED     (1)
    #endif
#endif
/*============================================================================*
    Types & Defines for Voice Assistant
 *============================================================================*/

typedef enum
{
    voice_ui_a2dp_state_suspended,
    voice_ui_a2dp_state_streaming
}voice_ui_a2dp_state_t;

/*! \brief Voice Assistant provider names
 *         Note: This is a list of voice assistants in priority order */
typedef enum
{
    voice_ui_provider_none = 0,
    voice_ui_provider_audio_tuning = 1,
    voice_ui_provider_gaa = 2,
    voice_ui_provider_ama = 3
} voice_ui_provider_t;

/*! \brief Voice Assistant Callback Routines */
typedef struct
{
    voice_ui_provider_t va_provider;
    void (*EventHandler)(ui_input_t event_id);
    void (*Suspend)(void);
    void (*DeselectVoiceAssitant)(void);
    void (*SelectVoiceAssitant)(void);    
}const voice_ui_if_t;

typedef struct
{
    voice_ui_provider_t (*GetActiveVaProvider)(void);
    void (*SetVoiceAssistantA2dpStreamState)(voice_ui_a2dp_state_t a2dp_stream_state);
}const voice_ui_protected_if_t;

/*! \brief Voice assistant handle descriptor */
typedef struct
{
    voice_ui_a2dp_state_t    va_a2dp_state;
    voice_ui_if_t*           voice_assistant;
    voice_ui_protected_if_t* va_protected_if;
}voice_ui_handle_t;

/*\{*/

/*! \brief Get the active voice assistant
 */
voice_ui_provider_t VoiceUi_GetSelectedAssistant(void);

/*! \brief Get the available voice assistant
    \param assistants Array of uint8 to be populated with identifiers of the
           available assistants.
    \return Number of supported assistants.
 */
uint16 VoiceUi_GetSupportedAssistants(uint8 *assistants);

/*! \brief Set the active voice assistant
 *   \param va_provider Voice assistant provider to be selected
 *   \return TRUE if the provider was successfully selected, FALSE otherwise
 */
bool VoiceUi_SelectVoiceAssistant(voice_ui_provider_t va_provider);

/*! \brief Stores the active voice assistant into the Device database
 */
void VoiceUi_SetSelectedAssistant(uint8 voice_ui_provider);

/* AMA_TODO - these are functions private to voice ui. Prototype needs to be moved to private heaser file */
/*! \brief Registration method to be called by Voice Assistant.

    \param va_table  voice assistant registered call back
 */
voice_ui_protected_if_t *VoiceUi_Register(voice_ui_if_t* va_table);

/*! \brief DeInitialise the Voice Assistants Component and free up allocated resources.

    \param none
 */
void VoiceUi_UnRegister(voice_ui_provider_t va_provider);

/*! \brief Get the active Voice Assistant
*/
voice_ui_handle_t* VoiceUi_GetActiveVa(void);

/*! \brief Function called by voice assistant to handle ui events.
    \param
 */
void VoiceUi_EventHandler(voice_ui_handle_t* va_handle, ui_input_t event_id);

/*! \brief Function called by voice assistant to suspend current voice assitant.
    \param  
 */
void VoiceUi_Suspend(voice_ui_handle_t* va_handle);


/*! \brief Reboot the local device after a delay
 */
void VoiceUi_RebootLater(void);

#endif


