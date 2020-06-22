/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for the  gaia anc framework plugin
*/

#include "anc_gaia_plugin.h"
#include "anc_gaia_plugin_private.h"
#include "anc_state_manager.h"
#include "ui.h"
#include "phy_state.h"

#include <gaia.h>
#include <logging.h>
#include <panic.h>

/*! \brief Function pointer definition for the command handler

    \param pdu_id      PDU specific ID for the message

    \param length      Length of the payload

    \param payload     Payload data
*/
static void ancGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload);

/*! \brief Function that sends all available notifications
*/
static void ancGaiaPlugin_SendAllNotifications(void);

/*! GAIA ANC Plugin Message Handler. */
static void ancGaiaPlugin_HandleMessage(Task task, MessageId id, Message message);

static void ancGaiaPlugin_GetAncState(void);
static void ancGaiaPlugin_SetAncState(uint8 payload_length, const uint8 *payload);
static void ancGaiaPlugin_GetNumOfModes(void);
static void ancGaiaPlugin_GetCurrentMode(void);
static void ancGaiaPlugin_SetAncMode(uint8 payload_length, const uint8 *payload);
static void ancGaiaPlugin_GetAncLeakthroughGain(void);
static void ancGaiaPlugin_SetAncLeakthroughGain(uint8 payload_length, const uint8 *payload);

static void ancGaiaPlugin_SetReceivedCommand(uint8 received_command)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    anc_gaia_data -> is_command_received = TRUE;
    anc_gaia_data -> received_command = received_command;
}

static uint8 ancGaiaPlugin_GetReceivedCommand(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    return anc_gaia_data -> received_command;
}

static void ancGaiaPlugin_ResetReceivedCommand(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    anc_gaia_data -> is_command_received = FALSE;
}

static bool ancGaiaPlugin_IsCommandReceived(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    return anc_gaia_data -> is_command_received;
}

static void ancGaiaplugin_UpdateAncState(bool anc_state)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    anc_gaia_data -> anc_state = anc_state;
}

static bool ancGaiaplugin_GetAncState(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    return anc_gaia_data -> anc_state;
}

static void ancGaiaplugin_UpdateAncMode(uint8 anc_mode)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    anc_gaia_data -> anc_mode = anc_mode;
}

static uint8 ancGaiaplugin_GetAncMode(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    return anc_gaia_data -> anc_mode;
}

static void ancGaiaplugin_UpdateAncGain(uint8 anc_gain)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    anc_gaia_data -> anc_gain = anc_gain;
}

static uint8 ancGaiaplugin_GetAncGain(void)
{
    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    return anc_gaia_data -> anc_gain;
}

static void ancGaiaPlugin_SetAncEnable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("ancGaiaPlugin_SetAncEnable");
        Ui_InjectUiInput(ui_input_anc_on);
    }
}

static void ancGaiaPlugin_SetAncDisable(void)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("ancGaiaPlugin_SetAncDisable");
        Ui_InjectUiInput(ui_input_anc_off);
    }
}

static void ancGaiaPlugin_SetMode(anc_mode_t mode)
{
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("ancGaiaPlugin_SetMode");
        switch(mode)
        {
            case anc_mode_1:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
            case anc_mode_2:
                Ui_InjectUiInput(ui_input_anc_set_mode_2);
                break;
            case anc_mode_3:
                Ui_InjectUiInput(ui_input_anc_set_mode_3);
                break;
            case anc_mode_4:
                Ui_InjectUiInput(ui_input_anc_set_mode_4);
                break;
            case anc_mode_5:
                Ui_InjectUiInput(ui_input_anc_set_mode_5);
                break;
            case anc_mode_6:
                Ui_InjectUiInput(ui_input_anc_set_mode_6);
                break;
            case anc_mode_7:
                Ui_InjectUiInput(ui_input_anc_set_mode_7);
                break;
            case anc_mode_8:
                Ui_InjectUiInput(ui_input_anc_set_mode_8);
                break;
            case anc_mode_9:
                Ui_InjectUiInput(ui_input_anc_set_mode_9);
                break;
            case anc_mode_10:
                Ui_InjectUiInput(ui_input_anc_set_mode_10);
                break;
            default:
                Ui_InjectUiInput(ui_input_anc_set_mode_1);
                break;
        }
    }
}

static void ancGaiaPlugin_SetGain(uint8 gain)
{
    UNUSED(gain);
    if (appPhyStateIsOutOfCase())
    {
        DEBUG_LOG_ALWAYS("ancGaiaPlugin_SetGain");
        AncStateManager_StoreAncLeakthroughGain(gain);
        Ui_InjectUiInput(ui_input_anc_set_leakthrough_gain);
    }
}

void AncGaiaPlugin_Init(void)
{
    DEBUG_LOG("AncGaiaPlugin_Init");

    anc_gaia_plugin_task_data_t *anc_gaia_data = ancGaiaPlugin_GetTaskData();

    /* Initialise plugin framework task data */
    memset(anc_gaia_data, 0, sizeof(*anc_gaia_data));
    anc_gaia_data->task.handler = ancGaiaPlugin_HandleMessage;

    AncStateManager_ClientRegister(ancGaiaPlugin_GetTask());

    ancGaiaplugin_UpdateAncState(AncStateManager_IsEnabled());
    ancGaiaplugin_UpdateAncMode(AncStateManager_GetCurrentMode());
    ancGaiaplugin_UpdateAncGain(AncStateManager_GetAncLeakthroughGain());

    GaiaFramework_RegisterFeature(GAIA_AUDIO_CURATION_FEATURE_ID, ANC_GAIA_PLUGIN_VERSION, ancGaiaPlugin_MainHandler, ancGaiaPlugin_SendAllNotifications);
}

static void ancGaiaPlugin_MainHandler(uint8 pdu_id, uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("ancGaiaPlugin_MainHandler called for %d", pdu_id);

    switch (pdu_id)
    {
        case anc_gaia_get_anc_state:
            ancGaiaPlugin_GetAncState();
            break;

        case anc_gaia_set_anc_state:
            ancGaiaPlugin_SetAncState(payload_length, payload);
            break;

        case anc_gaia_get_num_anc_modes:
            ancGaiaPlugin_GetNumOfModes();
            break;

        case anc_gaia_get_current_anc_mode:
            ancGaiaPlugin_GetCurrentMode();
            break;

        case anc_gaia_set_anc_mode:
            ancGaiaPlugin_SetAncMode(payload_length, payload);
            break;

        case anc_gaia_get_configured_leakthrough_gain:
            ancGaiaPlugin_GetAncLeakthroughGain();
            break;

        case anc_gaia_set_configured_leakthrough_gain:
            ancGaiaPlugin_SetAncLeakthroughGain(payload_length, payload);
            break;

        default:
            DEBUG_LOG("ancGaiaPlugin_MainHandler UNHANDLED call for %d", pdu_id);
            break;
    }
}

static void ancGaiaPlugin_SendAllNotifications(void)
{
    DEBUG_LOG("ancGaiaPlugin_SendAllNotifications");

    uint8 anc_state_payload = AncStateManager_IsEnabled() ? ANC_GAIA_STATE_ENABLE : ANC_GAIA_STATE_DISABLE;
    GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_state_notification, ANC_GAIA_ANC_STATE_NOTIFICATION_PAYLOAD_LENGTH, &anc_state_payload);

    uint8 anc_mode_payload = AncStateManager_GetCurrentMode();
    GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_mode_change_notification, ANC_GAIA_ANC_MODE_CHANGE_NOTIFICATION_PAYLOAD_LENGTH, &anc_mode_payload);

    if(AncStateManager_GetCurrentMode() != anc_mode_1)
	{
        uint8 anc_gain_payload = AncStateManager_GetAncLeakthroughGain();
        GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_gain_change_notification, ANC_GAIA_ANC_GAIN_CHANGE_NOTIFICATION_PAYLOAD_LENGTH, &anc_gain_payload);
	}

}

static void ancGaiaPlugin_GetAncState(void)
{
    DEBUG_LOG("ancGaiaPlugin_GetAncState");

    uint8 payload = AncStateManager_IsEnabled() ? ANC_GAIA_STATE_ENABLE : ANC_GAIA_STATE_DISABLE;

    DEBUG_LOG("ancGaiaPlugin_GetAncState, ANC State is %d", payload);
    GaiaFramework_SendResponse(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_get_anc_state, ANC_GAIA_GET_ANC_STATE_RESPONSE_PAYLOAD_LENGTH, &payload);
}

static void ancGaiaPlugin_SetAncState(uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("ancGaiaPlugin_SetAncState");

    if(payload_length == ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH)
    {
        if(*payload == ANC_GAIA_SET_ANC_STATE_ENABLE)
        {
            ancGaiaPlugin_SetAncEnable();
        }
        else if(*payload == ANC_GAIA_SET_ANC_STATE_DISABLE)
        {
            ancGaiaPlugin_SetAncDisable();
        }
    }
    ancGaiaPlugin_SetReceivedCommand(anc_gaia_set_anc_state);
}

static void ancGaiaPlugin_GetNumOfModes(void)
{
    DEBUG_LOG("ancGaiaPlugin_GetNumOfModes");

    uint8 payload= AncStateManager_GetNumberOfModes();

    DEBUG_LOG("ancGaiaPlugin_GetNumOfModes, Number of modes = %d", payload);
    GaiaFramework_SendResponse(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_get_num_anc_modes, ANC_GAIA_GET_ANC_NUM_OF_MODES_RESPONSE_PAYLOAD_LENGTH, &payload);
}

static void ancGaiaPlugin_GetCurrentMode(void)
{
    DEBUG_LOG("ancGaiaPlugin_GetCurrentMode");

    uint8 payload= AncStateManager_GetCurrentMode();

    DEBUG_LOG("ancGaiaPlugin_GetCurrentMode, Current mode = %d", payload);
    GaiaFramework_SendResponse(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_get_current_anc_mode, ANC_GAIA_GET_ANC_CURRENT_MODE_RESPONSE_PAYLOAD_LENGTH, &payload);
}


static void ancGaiaPlugin_SetAncMode(uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("ancGaiaPlugin_SetAncMode");

    if(payload_length == ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH)
    {
        anc_mode_t mode = (anc_mode_t)*payload;
        ancGaiaPlugin_SetMode(mode);
    }
    ancGaiaPlugin_SetReceivedCommand(anc_gaia_set_anc_mode);
}

static void ancGaiaPlugin_GetAncLeakthroughGain(void)
{
    DEBUG_LOG("ancGaiaPlugin_GetAncLeakthroughGain");

    if(AncStateManager_GetCurrentMode() != anc_mode_1)
    {
        uint8 payload = AncStateManager_GetAncLeakthroughGain();

        DEBUG_LOG("ancGaiaPlugin_GetAncLeakthroughGain, ANC Leakthrough gain = %d", payload);
        GaiaFramework_SendResponse(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_get_configured_leakthrough_gain, ANC_GAIA_GET_ANC_LEAKTHROUGH_GAIN_RESPONSE_PAYLOAD_LENGTH, &payload);
    }
}

static void ancGaiaPlugin_SetAncLeakthroughGain(uint8 payload_length, const uint8 *payload)
{
    DEBUG_LOG("ancGaiaPlugin_SetAncLeakthroughGain");

    if(AncStateManager_GetCurrentMode() != anc_mode_1 && payload_length == ANC_GAIA_SET_ANC_STATE_PAYLOAD_LENGTH)
    {
        ancGaiaPlugin_SetGain(*payload);
        ancGaiaPlugin_SetReceivedCommand(anc_gaia_set_configured_leakthrough_gain);
    }
}

/*! \brief Handle local events for ANC data update.and Send response */
static void ancGaiaPlugin_SendResponse(void)
{
    GaiaFramework_SendResponse(GAIA_AUDIO_CURATION_FEATURE_ID, ancGaiaPlugin_GetReceivedCommand(), 0, NULL);

    ancGaiaPlugin_ResetReceivedCommand();
}

/*! \brief Handle local events for ANC data update.and Sends Notification */
static void ancGaiaPlugin_SendNotification(const ANC_UPDATE_IND_T* anc_data)
{
    if(ancGaiaplugin_GetAncState() != anc_data -> state)
    {
        uint8 payload = anc_data -> state ? ANC_GAIA_STATE_ENABLE : ANC_GAIA_STATE_DISABLE;

        GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_state_notification, ANC_GAIA_ANC_STATE_NOTIFICATION_PAYLOAD_LENGTH, &payload);
    }
    if(ancGaiaplugin_GetAncMode() != anc_data -> mode)
    {
        uint8 payload = anc_data -> mode;

        GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_mode_change_notification, ANC_GAIA_ANC_MODE_CHANGE_NOTIFICATION_PAYLOAD_LENGTH, &payload);
    }
    if(AncStateManager_GetCurrentMode() != anc_mode_1 && ancGaiaplugin_GetAncGain() != anc_data -> anc_leakthrough_gain)
    {
        uint8 payload = anc_data -> anc_leakthrough_gain;

        GaiaFramework_SendNotification(GAIA_AUDIO_CURATION_FEATURE_ID, anc_gaia_anc_gain_change_notification, ANC_GAIA_ANC_GAIN_CHANGE_NOTIFICATION_PAYLOAD_LENGTH, &payload);
    }
}

static void ancGaiaPlugin_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        /* ANC update indication */
        case ANC_UPDATE_IND:
            {
                const ANC_UPDATE_IND_T* anc_data = (ANC_UPDATE_IND_T*)message;

                if(ancGaiaPlugin_IsCommandReceived())
                {
                    ancGaiaPlugin_SendResponse();
                }
				ancGaiaPlugin_SendNotification(anc_data);

                ancGaiaplugin_UpdateAncState(anc_data -> state);
                ancGaiaplugin_UpdateAncMode(anc_data -> mode);
                ancGaiaplugin_UpdateAncGain(anc_data -> anc_leakthrough_gain);
            }

            break;

        default:
            DEBUG_LOG("ancGaiaPlugin_HandleMessage unhandled message id %u", id);
            break;
    }
}

