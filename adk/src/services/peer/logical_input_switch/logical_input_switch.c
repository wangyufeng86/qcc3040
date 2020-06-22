/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Logical input switch implementation.
*/

#include "logical_input_switch.h"
#include "logical_input_switch_marshal_defs.h"

#include <bt_device.h>
#include <peer_signalling.h>
#include <state_proxy.h>
#include <ui.h>
#include <domain_message.h>

#include <bdaddr.h>
#include <input_event_manager.h>
#include <logging.h>
#include <panic.h>
#include <stdlib.h>
#include <vmtypes.h>

/* The full range of logical inputs are split across two different message
 * groups, one of which is used to identify device specific messages. */
#define MIN_LOGICAL_INPUT_MSG DEVICE_SPECIFIC_LOGICAL_INPUT_MESSAGE_BASE
#define MAX_LOGICAL_INPUT_MSG LAST_ID_IN_MSG_GRP(LOGICAL_INPUT_MESSAGE_GROUP)

/* Message ids reserved for passthrough logical inputs */
#define PASSTHROUGH_DEVICE_SPECIFIC_LOGICAL_INPUT_MSG INTERNAL_MESSAGE_BASE
#define PASSTHROUGH_LOGICAL_INPUT_MSG INTERNAL_MESSAGE_BASE + 1
typedef struct
{
    ui_input_t ui_input;
} PASSTHROUGH_LOGICAL_INPUT_MSG_T;

static void logicalInputSwitch_HandleMessage(Task task, MessageId id, Message message);

static TaskData lis_task = { .handler=logicalInputSwitch_HandleMessage };

static bool reroute_logical_inputs_to_peer = FALSE;

static uint16 lowest_valid_logical_input = MIN_LOGICAL_INPUT_MSG;
static uint16 highest_valid_logical_input = MAX_LOGICAL_INPUT_MSG;

static inline bool logicalInputSwitch_IsPassthroughLogicalInput(uint16 logical_input)
{
    return logical_input == PASSTHROUGH_LOGICAL_INPUT_MSG ||
           logical_input == PASSTHROUGH_DEVICE_SPECIFIC_LOGICAL_INPUT_MSG;
}

static inline bool logicalInputSwitch_IsDeviceSpecificLogicalInput(uint16 logical_input)
{
    return ID_TO_MSG_GRP(logical_input) == DEVICE_SPECIFIC_LOGICAL_INPUT_MESSAGE_GROUP ||
           logical_input == PASSTHROUGH_DEVICE_SPECIFIC_LOGICAL_INPUT_MSG;
}

static ui_input_t logicalInputSwitch_PassthroughUiInput(uint16 logical_input, PASSTHROUGH_LOGICAL_INPUT_MSG_T* message)
{
    if (logicalInputSwitch_IsPassthroughLogicalInput(logical_input))
    {
        PanicNull(message);
        return message->ui_input;
    }
    else
    {
        return ui_input_invalid;
    }
}

static bool logicalInputSwitch_IsValidLogicalInput(uint16 logical_input)
{
    bool logical_input_in_valid_range = logical_input >= lowest_valid_logical_input && logical_input <= highest_valid_logical_input;

    return logical_input_in_valid_range || logicalInputSwitch_IsPassthroughLogicalInput(logical_input);
}

static void logicalInputSwitch_PassLogicalInputToLocalUi(uint16 logical_input, ui_input_t ui_input)
{
    if (logicalInputSwitch_IsPassthroughLogicalInput(logical_input))
    {
        DEBUG_LOG("logicalInputSwitch_PassLogicalInputToLocalUi Injecting UI Input %04X", ui_input);

        Ui_InjectUiInput(ui_input);
    }
    else
    {
        MessageSend(Ui_GetUiTask(), logical_input, NULL);
    }
}

static void logicalInputSwitch_SendLogicalInputToPeer(uint16 logical_input, ui_input_t ui_input)
{
    logical_input_ind_t* msg = PanicUnlessMalloc(sizeof(logical_input_ind_t));
    msg->logical_input = logical_input;
    msg->passthrough_ui_input = ui_input;

    DEBUG_LOG("logicalInputSwitch_SendLogicalInputToPeer %04X %04X", msg->logical_input, msg->passthrough_ui_input);

    appPeerSigMarshalledMsgChannelTx(LogicalInputSwitch_GetTask(),
                                     PEER_SIG_MSG_CHANNEL_LOGICAL_INPUT_SWITCH,
                                     msg,
                                     MARSHAL_TYPE_logical_input_ind_t);
}

static inline void logicalInputSwitch_HandleMarshalledMsgChannelTxCfm(PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T* cfm)
{
    DEBUG_LOG("logicalInputSwitch_HandleMarshalledMsgChannelTxCfm %d", cfm->status);
}

static void logicalInputSwitch_HandleMarshalledMsgChannelRxInd(PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T* ind)
{
    logical_input_ind_t *msg;
    PanicNull(ind);
    msg = (logical_input_ind_t *)ind->msg;
    PanicNull(msg);

    logicalInputSwitch_PassLogicalInputToLocalUi(msg->logical_input, msg->passthrough_ui_input);
    free(msg);
}

static void logicalInputSwitch_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch(id)
    {
    case PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND:
        logicalInputSwitch_HandleMarshalledMsgChannelRxInd((PEER_SIG_MARSHALLED_MSG_CHANNEL_RX_IND_T*)message);
        break;

    case PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM:
        logicalInputSwitch_HandleMarshalledMsgChannelTxCfm((PEER_SIG_MARSHALLED_MSG_CHANNEL_TX_CFM_T*)message);
        break;

    default:
        if (logicalInputSwitch_IsValidLogicalInput(id))
        {
            ui_input_t passthrough_ui_input = logicalInputSwitch_PassthroughUiInput(id, (PASSTHROUGH_LOGICAL_INPUT_MSG_T*)message);

            if (reroute_logical_inputs_to_peer &&
                !logicalInputSwitch_IsDeviceSpecificLogicalInput(id))
            {
                logicalInputSwitch_SendLogicalInputToPeer(id, passthrough_ui_input);
            }
            else
            {
                logicalInputSwitch_PassLogicalInputToLocalUi(id, passthrough_ui_input);
            }
        }
        break;
    }
}

static void logicalInputSwitch_SendPassthroughLogicalInput(ui_input_t ui_input, MessageId passthrough_msg)
{
    MESSAGE_MAKE(msg, PASSTHROUGH_LOGICAL_INPUT_MSG_T);
    msg->ui_input = ui_input;

    MessageSend(LogicalInputSwitch_GetTask(), passthrough_msg, msg);
}

Task LogicalInputSwitch_GetTask(void)
{
    return &lis_task;
}

void LogicalInputSwitch_SetRerouteToPeer(bool reroute_enabled)
{
    reroute_logical_inputs_to_peer = reroute_enabled;
}

void LogicalInputSwitch_SetLogicalInputIdRange(uint16 lowest, uint16 highest)
{
    if ((lowest < MIN_LOGICAL_INPUT_MSG) || (highest > MAX_LOGICAL_INPUT_MSG) || (lowest > highest))
    {
        DEBUG_LOG("LogicalInputSwitch_SetLogicalInputIdRange Invalid range %04X %04X", lowest, highest);
        Panic();
    }

    lowest_valid_logical_input = lowest;
    highest_valid_logical_input = highest;
}

void LogicalInputSwitch_SendPassthroughLogicalInput(ui_input_t ui_input)
{
    logicalInputSwitch_SendPassthroughLogicalInput(ui_input, PASSTHROUGH_LOGICAL_INPUT_MSG);
}

void LogicalInputSwitch_SendPassthroughDeviceSpecificLogicalInput(ui_input_t ui_input)
{
    logicalInputSwitch_SendPassthroughLogicalInput(ui_input, PASSTHROUGH_DEVICE_SPECIFIC_LOGICAL_INPUT_MSG);
}

bool LogicalInputSwitch_Init(Task init_task)
{
    /* Verify these two message bases are consecutive */
    PanicFalse(LOGICAL_INPUT_MESSAGE_BASE==MSG_GRP_TO_ID(ID_TO_MSG_GRP(DEVICE_SPECIFIC_LOGICAL_INPUT_MESSAGE_BASE)+1));

    LogicalInputSwitch_SetRerouteToPeer(FALSE);

    LogicalInputSwitch_SetLogicalInputIdRange(MIN_LOGICAL_INPUT_MSG, MAX_LOGICAL_INPUT_MSG);

    appPeerSigMarshalledMsgChannelTaskRegister(LogicalInputSwitch_GetTask(),
                                               PEER_SIG_MSG_CHANNEL_LOGICAL_INPUT_SWITCH,
                                               logical_input_switch_marshal_type_descriptors,
                                               NUMBER_OF_MARSHAL_OBJECT_TYPES);

    UNUSED(init_task);
    return TRUE;
}

static void logicalInputSwitch_RegisterMessageGroup(Task task, message_group_t group)
{
    PanicFalse((group == LOGICAL_INPUT_MESSAGE_GROUP)||
               (group == DEVICE_SPECIFIC_LOGICAL_INPUT_MESSAGE_GROUP));
    InputEventManager_RegisterClient(task);
}

MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(LOGICAL_INPUT, logicalInputSwitch_RegisterMessageGroup, NULL);
MESSAGE_BROKER_GROUP_REGISTRATION_MAKE(DEVICE_SPECIFIC_LOGICAL_INPUT, logicalInputSwitch_RegisterMessageGroup, NULL);
