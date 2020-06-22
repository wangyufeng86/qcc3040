/*!
\copyright  Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       voice_ui.c
\brief      Implementation of voice ui service.
*/

#include "logging.h"
#include <task_list.h>
#include <csrtypes.h>
#include <panic.h>
#include <power_manager.h>
#include <vmtypes.h>
#include <hfp_profile.h>

#include "ui.h"

#include "voice_ui.h"
#include "voice_audio_manager.h"
#include "voice_ui_container.h"
#include "voice_ui_gaia_plugin.h"
#include "voice_ui_peer_sig.h"
#include "telephony_messages.h"

#define VOICE_UI_REBOOT_DELAY_MILLISECONDS ((Delay) 250)

typedef struct
{
    TaskData task;

} voice_ui_service_data;

static voice_ui_service_data voice_ui_service;

static task_list_t * voice_ui_client_list;

/* Ui Inputs in which voice ui service is interested*/
static const uint16 voice_ui_inputs[] =
{
    ID_TO_MSG_GRP(UI_INPUTS_VOICE_UI_MESSAGE_BASE),
};

static Task voiceUi_GetTask(void)
{
  return (Task)&voice_ui_service.task;
}

static void voiceUi_HandleUiInput(MessageId ui_input)
{
    voice_ui_handle_t* handle = VoiceUi_GetActiveVa();

    if(handle)
    {
        VoiceUi_EventHandler(handle, ui_input);
    }
}

static unsigned voiceUi_GetUiContext(void)
{
    return (unsigned)context_voice_ui_default;
}

static void voiceUi_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);
    UNUSED(message);

    if (isMessageUiInput(id))
    {
        voiceUi_HandleUiInput(id);
        return;
    }

    /* Handle HFP profile messages */
    switch (id)
    {
        case TELEPHONY_INCOMING_CALL:
        case APP_HFP_SCO_CONNECTED_IND:
            VoiceUi_Suspend(VoiceUi_GetActiveVa());
            break;


        case APP_HFP_SCO_DISCONNECTED_IND:
        default:
            DEBUG_LOG("voiceUi_HandleMessage: Unhandled id 0x%x", id);
            break;

        case VOICE_UI_INTERNAL_REBOOT:
            appPowerReboot();
            break;
    }
}

bool VoiceUi_Init(Task init_task)
{
    DEBUG_LOG("VoiceUi_Init()");

    UNUSED(init_task);

    voice_ui_client_list = TaskList_Create();
    VoiceUiGaiaPlugin_Init();
    VoiceUi_PeerSignallingInit();

    voice_ui_service.task.handler = voiceUi_HandleMessage;
	
	/* register with HFP for changes in sco state */
    appHfpStatusClientRegister(voiceUi_GetTask());

    /* Register av task call back as ui provider*/
    Ui_RegisterUiProvider(ui_provider_voice_ui, voiceUi_GetUiContext);

    Ui_RegisterUiInputConsumer(voiceUi_GetTask(), (uint16*)voice_ui_inputs, sizeof(voice_ui_inputs)/sizeof(uint16));

    return TRUE;
}

void VoiceUi_Notify(voice_ui_msg_id_t msg)
{
    TaskList_MessageSendId(voice_ui_client_list, msg);
}

static void voiceAssistant_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse(group == VOICE_UI_SERVICE_MESSAGE_GROUP);
    TaskList_AddTask(voice_ui_client_list, task);
}

void VoiceUi_RebootLater(void)
{
    MessageSendLater(voiceUi_GetTask(), VOICE_UI_INTERNAL_REBOOT, NULL, VOICE_UI_REBOOT_DELAY_MILLISECONDS);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(VOICE_UI_SERVICE, voiceAssistant_RegisterMessageGroup, NULL);
