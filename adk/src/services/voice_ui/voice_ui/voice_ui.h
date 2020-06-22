/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       voice_ui.h
\defgroup   voice_ui 
\ingroup    services
\brief      A component responsible for controlling voice assistants.

Responsibilities:
- Voice Ui control (like A way to start/stop voice capture as a result of some user action) 
  Notifications to be sent to the Application raised by the VA.

The Voice Ui uses  \ref audio_domain Audio domain and \ref bt_domain BT domain.

*/

#ifndef VOICE_UI_H_
#define VOICE_UI_H_

#include "domain_message.h"
#include <message.h>

/*! \brief Voice UI Provider contexts */
typedef enum
{
    context_voice_ui_default = 0
} voice_ui_context_t;


/*! \brief Messages sent by the voice ui service to interested clients. */
typedef enum
{
    VOICE_UI_IDLE   = VOICE_UI_SERVICE_MESSAGE_BASE,
    VOICE_UI_CONNECTED,
    VOICE_UI_ACTIVE,
    VOICE_UI_MIC_OPEN,
    VOICE_UI_CAPTURE_START,
    VOICE_UI_CAPTURE_END,
    VOICE_UI_MIC_CLOSE,
    VOICE_UI_DISCONNECTED
} voice_ui_msg_id_t;

/*! \brief voice ui service message. */
typedef struct
{
    void *vui_handle;
} voice_ui_msg_t;

/*\{*/

/*! \brief Initialise the voice ui service

    \param init_task Unused
 */
bool VoiceUi_Init(Task init_task);

/*! \brief Notify clients of the Voice UI Service

    \param msg The message to notify the clients of the Voice UI with
 */
void VoiceUi_Notify(voice_ui_msg_id_t msg);

/*! \brief Gets the A2DP status of the currently active voice assistant

    \return TRUE if VA A2DP is in streaming state, FALSE otherwise
 */
bool VoiceUi_IsVoiceAssistantA2dpStreamActive(void);

/*\}*/

#endif /* VOICE_UI_H_ */
