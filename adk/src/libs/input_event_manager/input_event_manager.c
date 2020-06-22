/*
Copyright (c) 2018-2019  Qualcomm Technologies International, Ltd.
*/
#include <csrtypes.h>
#include <vmtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <task_list.h>
#include <pio.h>

#include "pio_monitor.h"
#include "input_event_manager.h"
#include "input_event_manager_private.h"

static InputEventState_t input_event_manager_state = { {0} };

static void enterAction(InputEventState_t *state,
                        const InputActionMessage_t *input_action,
                        input_event_bits_t input_event_bits)
{
    /* If all the bits, for the msg, are 'on', and at least one of those bits,
     * was just turned on, then ...
     */
    if (input_action->bits == (input_event_bits & input_action->mask))
    {
        /* A new enter action cancels any existing repeat timer */
        (void) MessageCancelAll(&state->task,
                                IEM_INTERNAL_REPEAT_TIMER);
        TaskList_MessageSendId(state->client_tasks, input_action->message);

        /* if there is a repeat on this action, start the repeat timer */
        if (input_action->repeat)
        {
            state->repeat = input_action;
            MessageSendLater(&state->task, IEM_INTERNAL_REPEAT_TIMER, 0, input_action->repeat);
        }
        else
            state->repeat = 0;

    }
    /* if any of the bits are turned off and there is a repeat timer,
     * cancel it and clear the stored input_action
     */
    else if (input_action->repeat &&
             state->repeat == input_action &&
             input_action->bits == (state->input_event_bits & input_action->mask) &&
             input_action->bits != (input_event_bits & input_action->mask))
    {
        (void) MessageCancelAll(&state->task, IEM_INTERNAL_REPEAT_TIMER);
        state->repeat = 0;
    }
}

static void releaseAction(InputEventState_t *state,
                        const InputActionMessage_t *input_action,
                        input_event_bits_t input_event_bits)
{
    bool prev_bit_state_is_high = (input_action->bits == (state->input_event_bits & input_action->mask));
    bool bit_state_is_low = (input_action->bits != (input_event_bits & input_action->mask));

    if (prev_bit_state_is_high && bit_state_is_low)
    {
        TaskList_MessageSendId(state->client_tasks, input_action->message);
    }
}

static void heldActionButtonDownAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    /* Send a pointer to this input_action as part of the timer message so that it
     * can be handled when the timeout expired
     */
    const InputActionMessage_t **m = PanicUnlessNew(const InputActionMessage_t *);
    *m = input_action;

    MessageSendLater(&state->task, IEM_INTERNAL_HELD_TIMER, m, input_action->timeout);
}

static void heldActionButtonReleaseAction(InputEventState_t *state)
{
    /* Cancel any active held or repeat timers. */
    if (!MessageCancelAll(&state->task, IEM_INTERNAL_HELD_TIMER))
    {
        (void)MessageCancelAll(&state->task, IEM_INTERNAL_REPEAT_TIMER);
    }
}

/* There can be 1+ held action/messages on the same PIO */
static void heldAction(InputEventState_t *state,
                       const InputActionMessage_t *input_action,
                       input_event_bits_t input_event_bits)
{
    /* If all the PIO, for the msg, are 'on'... */
    if (input_action->bits == (input_event_bits & input_action->mask))
    {
        heldActionButtonDownAction(state, input_action);
    }
    else if (input_action->bits == (state->input_event_bits & input_action->mask) &&
             input_action->bits != (input_event_bits & input_action->mask))
    {
        heldActionButtonReleaseAction(state);
    }
}

static void singleClickFirstReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    const InputActionMessage_t **m = PanicUnlessNew(const InputActionMessage_t *);
    *m = input_action;
    /* The first release detected, start the timer and attach the single click event */
    IEM_DEBUG(("Starting single click timer, %p", input_action));
    MessageSendLater(&state->task, IEM_INTERNAL_SINGLE_CLICK_TIMER, (void*)m, SINGLE_CLICK_TIMEOUT);
    state->single_click_tracker.short_press |= input_action->bits;
}

static void singleClickSecondReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    /* This is a double click, so cancel the single click event */
    MessageCancelAll(&state->task, IEM_INTERNAL_SINGLE_CLICK_TIMER);
    state->single_click_tracker.short_press &= ~(input_action->bits);
}

static void singleClickAction(InputEventState_t *state,
                              const InputActionMessage_t *input_action,
                              input_event_bits_t input_event_bits)
{
    bool prev_bit_state_is_high = (input_action->bits == (state->input_event_bits & input_action->mask));
    bool bit_state_is_low = (input_action->bits != (input_event_bits & input_action->mask));
    bool single_press_already_detected = (input_action->bits == (state->single_click_tracker.short_press & input_action->mask ));
    bool held_release_detected = (input_action->bits == (state->single_click_tracker.long_press & input_action->mask ));

    if (prev_bit_state_is_high && bit_state_is_low)
    {
        if(held_release_detected)
        {
            /* A held release has been detected, do not process a single click.
             * Clear the single click tracker */
            state->single_click_tracker.short_press &= ~(input_action->bits);
            state->single_click_tracker.long_press &= ~(input_action->bits);
        }
        else
        {
            if(single_press_already_detected)
                singleClickSecondReleaseAction(state, input_action);
            else
                singleClickFirstReleaseAction(state, input_action);
        }
    }
}

static void heldReleaseButtonDownAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    const InputActionMessage_t **m = PanicUnlessNew(const InputActionMessage_t *);
    *m = input_action;

    MessageSendLater(&state->task, IEM_INTERNAL_HELD_RELEASE_TIMER, (void*)m, input_action->timeout);
    state->held_release = 0;
}

static void heldReleaseButtonReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    (void) MessageCancelAll(&state->task, IEM_INTERNAL_HELD_RELEASE_TIMER);

    if (state->held_release == input_action)
        /* If an action message was registered, it means that the
        * held_release timer has expired and hence send the
        * message */
    {
        state->held_release = 0;
        TaskList_MessageSendId(state->client_tasks, input_action->message);
    }
}

static void heldReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action, input_event_bits_t input_event_bits)
{
    /* If all the bits, for the msg, are 'on' then ...
     */
    if (input_action->bits == (input_event_bits & input_action->mask))
    {
        heldReleaseButtonDownAction(state, input_action);
    }
    else if (input_action->bits == (state->input_event_bits & input_action->mask) &&
             input_action->bits != (input_event_bits & input_action->mask))
    {
        heldReleaseButtonReleaseAction(state, input_action);
    }
}

static void doubleClickFirstReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    const InputActionMessage_t **m = PanicUnlessNew(const InputActionMessage_t *);
    *m = input_action;
    /* This is the first press, make a note */
    state->double_click_tracker.short_press |= (input_action->bits);
    MessageSendLater(&state->task, IEM_INTERNAL_DOUBLE_CLICK_TIMER, (void*)m, SINGLE_CLICK_TIMEOUT);
}

static void doubleClickSecondReleaseAction(InputEventState_t *state, const InputActionMessage_t *input_action)
{
    /* This a double click, send the event and remove any valid event from the single click queue */
    TaskList_MessageSendId(state->client_tasks, input_action->message);
    MessageCancelAll(&state->task, IEM_INTERNAL_SINGLE_CLICK_TIMER);
    state->double_click_tracker.short_press &= ~(input_action->bits);
}

static void doubleClickAction(InputEventState_t *state,
                         const InputActionMessage_t *input_action,
                         input_event_bits_t input_event_bits)
{
    bool prev_bit_state_is_high = (input_action->bits == (state->input_event_bits & input_action->mask));
    bool bit_state_is_low = (input_action->bits != (input_event_bits & input_action->mask));
    bool single_press_detected = (input_action->bits == (state->double_click_tracker.short_press & input_action->mask ));
    bool held_release_detected = (input_action->bits == (state->double_click_tracker.long_press & input_action->mask ));

    if (prev_bit_state_is_high && bit_state_is_low)
    {
        if(held_release_detected)
        {
            /* A held release has been detected, do not process a double click.
             * Clear the double click tracker */
            state->double_click_tracker.short_press &= ~(input_action->bits);
            state->double_click_tracker.long_press &= ~(input_action->bits);
        }
        else
        {
             if(single_press_detected)
                doubleClickSecondReleaseAction(state, input_action);
            else
                doubleClickFirstReleaseAction(state, input_action);
        }
    }
}

static void inputEventsChanged(InputEventState_t *state, input_event_bits_t input_event_bits)
{
    input_event_bits_t changed_bits = state->input_event_bits ^ input_event_bits;
    const InputActionMessage_t *input_action = state->action_table;
    const uint32 size = state->num_action_messages;

    IEM_DEBUG(("IEM: Updated input events %08x\n", input_event_bits));

    /* Go through the action table to determine what action to do and
       what message may need to be sent. */
    for (;input_action != &(state->action_table[size]); input_action++)
    {
        if (changed_bits & input_action->mask)
        {
            switch (input_action->action)
            {
                case ENTER:
                    enterAction(state, input_action, input_event_bits);
                    break;

                case RELEASE:
                    releaseAction(state, input_action, input_event_bits);
                    break;

                case SINGLE_CLICK:
                    singleClickAction(state, input_action, input_event_bits);
                    break;

                case DOUBLE_CLICK:
                    doubleClickAction(state, input_action, input_event_bits);
                    break;

                case HELD:
                    heldAction(state, input_action, input_event_bits);
                    break;

                case HELD_RELEASE:
                    heldReleaseAction(state, input_action, input_event_bits);
                    break;

                default:
                    break;
            }
        }
    }

    /* Store the bits previously reported */
    state->input_event_bits = input_event_bits;
}

static uint32 calculateInputEvents(InputEventState_t *state)
{
    int pio, bank;
    uint32 input_event_bits = 0;
    for (bank = 0; bank < IEM_NUM_BANKS; bank++)
    {
        const int pio_base = bank * 32;
        const uint32 pio_state = state->pio_state[bank];
        for (pio = 0; pio < 32; pio++)
        {
            const uint32 pio_mask = 1UL << pio;
            if (pio_state & pio_mask)
                input_event_bits |= (1UL << state->input_config->pio_to_iem_id[pio_base + pio]);
        }
    }
    return input_event_bits;
}

static void handleMessagePioChangedEvents(InputEventState_t *state, const MessagePioChanged *mpc)
{
    /* Mask out PIOs we're not interested in */
    const uint32 pio_state = (mpc->state) + ((uint32)mpc->state16to31 << 16);
    const uint32 pio_state_masked = pio_state & state->input_config->pio_input_mask[mpc->bank];

    if (state->pio_state[mpc->bank] != pio_state_masked)
    {
        /* Update our copy of the PIO state */
        state->pio_state[mpc->bank] = pio_state_masked;

        /* Calculate input events from PIO state and handle them */
        input_event_bits_t input_event_bits = calculateInputEvents(state);
        inputEventsChanged(state, input_event_bits);
    }
}

static void waitForEnableConfirmation(InputEventState_t *state)
{
    IEM_DEBUG(("IEM: Received event: PIO_MONITOR_ENABLE_CFM\n"));
    if (++(state->numActivePios) == state->maxActivePios)
    {
        /* Send initial PIO messages */
        for (uint8 bank = 0; bank < IEM_NUM_BANKS; bank++)
        {
            MessagePioChanged mpc_message;
            uint32 pio_state = PioGet32Bank(bank);
            mpc_message.state       = (pio_state >>  0) & 0xFFFF;
            mpc_message.state16to31 = (pio_state >> 16) & 0xFFFF;
            mpc_message.time = 0;
            mpc_message.bank = bank;
            handleMessagePioChangedEvents(state, &mpc_message);
        }
    }
}

static void iemHandler(Task task, MessageId id, Message message)
{
    InputEventState_t *state = (InputEventState_t *)task;

    switch (id)
    {
        case MESSAGE_PIO_CHANGED:
        {
            const MessagePioChanged *mpc = (const MessagePioChanged *)message;
            IEM_DEBUG(("IEM: MESSAGE_PIO_CHANGED: bank=%hu, mask=%04x%04x\n",
                       mpc->bank,mpc->state16to31,mpc->state));
            handleMessagePioChangedEvents(state,mpc);
        }
        break;

        case PIO_MONITOR_ENABLE_CFM:
        {
            waitForEnableConfirmation(state);
        }
        break;

        /* If a pio has been HELD for the timeout required, then send the message stored */
        case IEM_INTERNAL_HELD_TIMER:
        {
            const InputActionMessage_t **m = (const InputActionMessage_t **)message;
            const InputActionMessage_t *input_action = *m;

            /* Keep the trackers informed that this is now a held release */
            state->single_click_tracker.long_press |= (input_action->bits);
            state->double_click_tracker.long_press |= (input_action->bits);

            TaskList_MessageSendId(state->client_tasks, input_action->message);

            /* Cancel any existing repeat timer that may be running */
            (void)MessageCancelAll(&state->task, IEM_INTERNAL_REPEAT_TIMER);

            /* If there is a repeat action start the repeat on this message
               and store the input_action */
            if (input_action->repeat)
            {
                MessageSendLater(&state->task,
                                 IEM_INTERNAL_REPEAT_TIMER, 0,
                                 input_action->repeat);

                state->repeat = input_action;
            }
        }
        break;

        case IEM_INTERNAL_REPEAT_TIMER:
        {
            if (state->repeat)
            {
                TaskList_MessageSendId(state->client_tasks, (state->repeat)->message);

                /* Start the repeat timer again */
                MessageSendLater(&state->task, IEM_INTERNAL_REPEAT_TIMER,
                                 NULL, (state->repeat)->repeat);
            }
        }
        break;

        /* Store the input_action so that when the PIO for the message are released
           it can be validated and the message sent */
        case IEM_INTERNAL_HELD_RELEASE_TIMER:
        {
            const InputActionMessage_t **m = (const InputActionMessage_t **)message;
            state->held_release = *m;

            /* Keep the trackers informed that this is now a held release */
            state->single_click_tracker.long_press |= ((state->held_release)->bits);
            state->double_click_tracker.long_press |= ((state->held_release)->bits);
        }
        break;

        case IEM_INTERNAL_SINGLE_CLICK_TIMER:
        {
            const InputActionMessage_t **m = (const InputActionMessage_t **)message;
            const InputActionMessage_t *input_action = *m;

            IEM_DEBUG(("Sending single click message, %p", input_action));
            TaskList_MessageSendId(state->client_tasks, input_action->message);
            state->single_click_tracker.short_press &= ~(input_action->bits);
        }
        break;

        case IEM_INTERNAL_DOUBLE_CLICK_TIMER:
        {
            const InputActionMessage_t **m = (const InputActionMessage_t **)message;
            const InputActionMessage_t *input_action = *m;
            /* Clear the stored input_action */
            state->double_click_tracker.short_press &= ~(input_action->bits);
        }
        break;

        default:
            break;
    }
}

static void configurePioHardware(void)
{
    /* Configure PIOs:
       1.  Map as PIOs
       2.  Allow deep sleep on either level
       3.  Set as inputs */
    for ( uint16 bank = 0 ; bank < IEM_NUM_BANKS ; bank++ )
    {
        const uint32 pio_bank_mask = input_event_manager_state.input_config->pio_input_mask[bank];
        IEM_DEBUG(("IEM: Configuring bank %d, mask %08x\n", bank, pio_bank_mask));
        uint32 result;
        result = PioSetMapPins32Bank(bank, pio_bank_mask, pio_bank_mask);
        if (result != 0)
        {
            IEM_DEBUG(("IEM: PioSetMapPins32Bank error: bank %d, mask %08x, result=%08x\n", bank, pio_bank_mask, result));
            Panic();
        }
        PioSetDeepSleepEitherLevelBank(bank, pio_bank_mask, pio_bank_mask);
        result = PioSetDir32Bank(bank, pio_bank_mask, 0);
        if (result != 0)
        {
            IEM_DEBUG(("IEM: PioSetDir32Bank error: bank %d, mask %08x, result=%08x\n", bank, pio_bank_mask, result));
            Panic();
        }
    }
}

static void registerForPioEvents(void)
{
    for (uint8 pio = 0 ; pio < IEM_NUM_PIOS ; pio++)
    {
        if (input_event_manager_state.input_config->pio_to_iem_id[pio] != -1)
        {
            PioMonitorRegisterTask(&input_event_manager_state.task,pio);
        }
    }
    input_event_manager_state.maxActivePios = 1;
}

void InputEventManager_RegisterClient(Task client)
{
    TaskList_AddTask(input_event_manager_state.client_tasks, client);
}

Task InputEventManagerInit(Task client,
                           const InputActionMessage_t *action_table,
                           uint32 action_table_dim,
                           const InputEventConfig_t *input_config)
{
    IEM_DEBUG(("IEM: InputEventManagerInit\n"));
    memset(&input_event_manager_state, 0, sizeof(input_event_manager_state));

    input_event_manager_state.task.handler = iemHandler;
    input_event_manager_state.client_tasks = TaskList_Create();
    TaskList_AddTask(input_event_manager_state.client_tasks, client);
    input_event_manager_state.action_table = action_table;
    input_event_manager_state.num_action_messages = action_table_dim;
    input_event_manager_state.input_config = input_config;

    configurePioHardware();
    registerForPioEvents();
    PioMonitorSetDebounceParameters(input_config->debounce_period,input_config->debounce_reads);

    return &input_event_manager_state.task;
}
