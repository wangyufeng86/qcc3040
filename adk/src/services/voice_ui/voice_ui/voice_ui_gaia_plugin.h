/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      GAIA interface with Voice Assistants.
*/

#ifndef _VOICE_UI_GAIA_
#define _VOICE_UI_GAIA_

#include "voice_ui_container.h"

/*! \brief Voice UI GAIA component version
*/
#define GAIA_VOICE_UI_VERSION (1)

/*! \brief Commands handled by the Voice UI GAIA Component
*/
typedef enum
{
    /*! Get the currently selected Voice Assistant ID */
    voice_ui_gaia_get_selected_assistant = 0,
    /*! Select a Voice Assistant to use */
    voice_ui_gaia_set_selected_assistant = 1,
    /*! Get vector of supported assistant IDs */
    voice_ui_gaia_get_supported_assistants = 2
} voice_ui_gaia_command_t;

typedef enum
{
    voice_ui_gaia_assistant_changed = 0
} voice_ui_gaia_notification_t;

/*! \brief Initialise the Voice UI GAIA component.
*/
void VoiceUiGaiaPlugin_Init(void);

/*! \brief Notify the GAIA host that the VA provider has changed.
 *  \param va_provider Identifies the new VA provider.
*/
void VoiceUiGaiaPlugin_NotifyAssistantChanged(voice_ui_provider_t va_provider);

#endif /* _VOICE_UI_GAIA_ */
