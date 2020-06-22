/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    audio_mixer_power.c

DESCRIPTION
    Handle delayed power off of audio subsystem

*/

#include "audio_mixer.h"
#include "audio_mixer_power.h"

#include <operators.h>
#include <print.h>

#define DELAY_DISABLE_OPERATOR_FRAMEWORK_MS D_SEC(30)

#define MIXER_INTERNAL_MSG_BASE (0x0)

/* Mixer internal messages */
typedef enum
{
    AUDIO_MIXER_POWER_OFF_REQ = MIXER_INTERNAL_MSG_BASE,
    AUDIO_MIXER_MSG_TOP
} audio_mixer_msg_t;

static void AudioMixerMessageHandler ( Task task, MessageId id, Message message );

const TaskData mixer_plugin = { AudioMixerMessageHandler };

/****************************************************************************/
void audioMixerPowerOn(void)
{
    if(!MessageCancelFirst((Task)&mixer_plugin, AUDIO_MIXER_POWER_OFF_REQ))
    {
        PRINT(("AM: Power On Audio\n"));
        OperatorsFrameworkEnable();
    }
}

/****************************************************************************/
void audioMixerPowerOff(void)
{
    PRINT(("AM: Power Off Scheduled\n"));
    MessageSendLater((Task)&mixer_plugin, AUDIO_MIXER_POWER_OFF_REQ, NULL, DELAY_DISABLE_OPERATOR_FRAMEWORK_MS);
}


/****************************************************************************/
static void AudioMixerMessageHandler ( Task task, MessageId id, Message message )
{
    UNUSED(task);
    UNUSED(message);
    switch(id)
    {
        case AUDIO_MIXER_POWER_OFF_REQ:
            PRINT(("AM: Power Off Audio\n"));
            OperatorsFrameworkDisable();
        break;
        
        default:
        break;
    }
}