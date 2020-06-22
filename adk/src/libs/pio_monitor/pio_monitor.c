/*
Copyright (c) 2019  Qualcomm Technologies International, Ltd.
*/
#include <csrtypes.h>
#include <vmtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <pio.h>

#include "pio_monitor.h"
#include "pio_monitor_private.h"

static PioMonitorState_t pio_monitor_state = { 0 };

/*! Check if any task registered is interested in this PIO change message */
static void handleMessagePioChangedClients(PioMonitorState_t *state, const MessagePioChanged *mpc)
{
    PioMonitorClient_t *client = NULL;
    const uint32 pio_state = (mpc->state) + ((uint32)mpc->state16to31 << 16);

    /* For each client ... */
    for (client = state->cl_head;
         client != NULL;
         client = client->next)
    {
        /* For each PIO of the client ... */
        for (uint8 p = 0 ; p < client->numPios ; p++)
        {
            const int client_bank = client->pios[p] / 32;
            if (client_bank == mpc->bank)
            {
                const int pio_in_bank = client->pios[p] - (32 * client_bank);
                const uint32 pio_mask = 1UL << pio_in_bank;

                /* Check if latest PIO state doesn't match the stored state */
                if (((pio_state & pio_mask) >> pio_in_bank) != ((client->states >> p) & 1))
                {
                    /* Update stored state */
                    client->states ^= 1 << p;

                    PIOM_DEBUG(("PIOM: Sending MESSAGE_PIO_CHANGED to task %p for PIO %u\n", client->task, client->pios[p]));

                    /* Send MESSAGE_PIO_CHANGED to task */
                    void *mpc_copy = PanicUnlessNew(MessagePioChanged);
                    memcpy(mpc_copy, mpc, sizeof(MessagePioChanged));
                    MessageSend(client->task, MESSAGE_PIO_CHANGED, mpc_copy);
                }
            }
        }
    }
}

/*! Dispatch messages to the appropriate handling routine */
static void pioHandler(Task task, MessageId id, Message message)
{
    PioMonitorState_t *state = (PioMonitorState_t *)task;
    if (id == MESSAGE_PIO_CHANGED)
    {
        const MessagePioChanged *mpc = (const MessagePioChanged *)message;
        PIOM_DEBUG(("PIOM: pioHandler state %04x%04x for bank %u\n",
                    mpc->state16to31, mpc->state, mpc->bank));
        handleMessagePioChangedClients(state,mpc);
    }
}

/*!
 * Determine if there already exists a client task, and if there
 * is, that there is room to append another PIO.
 */
static PioMonitorClient_t *findClient(Task task)
{
    PioMonitorClient_t *result = NULL;
    PioMonitorClient_t *client;
    for (client = pio_monitor_state.cl_head;
         client != NULL;
         client = client->next)
    {
        if (client->task == task && client->numPios < MAX_PIOS_PER_TASK)
        {
            result = client;
            break;
        }
    }
    return result;
}

/*!
 * Create and initialise a new client object.
 */
static PioMonitorClient_t *newClient(Task task)
{
    PioMonitorClient_t *client = PanicUnlessNew(PioMonitorClient_t);
    memset(client, 0, sizeof(*client));
    memset(client->pios, EMPTY_PIO, MAX_PIOS_PER_TASK);
    client->task = task;
    return client;
}

/*!
 * Add a PIO to a client task.
 */
static void addPioToClient(PioMonitorClient_t *client, uint8 pio)
{
    const uint32 bank = pio / 32;
    const uint32 bank_pio = pio % 32;
    const uint32 bank_pio_mask = 1UL << bank_pio;
    uint32 pin_state = (PioGet32Bank(bank) & bank_pio_mask) ? 1 : 0;
    PanicFalse(client->numPios < MAX_PIOS_PER_TASK);
    pin_state <<= client->numPios;
    client->pios[client->numPios++] = pio;
    client->states |= pin_state;
}

/*!
 * Determine if a PIO is registered for this client.
 */
static int findPioInClient(PioMonitorClient_t *client, uint8 pio)
{
    for (uint8 i = 0 ; i < client->numPios ; i++)
    {
        if (client->pios[i] == pio) return i;
    }
    return -1;
}

/*!
 * From a PIO states value of 'aaaabccc', the objective
 * is to remove bit b, and result in '0aaaaccc', where 'b'
 * is the PIO state bit of the pio being removed.
 */
static void removePioFromClient(PioMonitorClient_t *client, uint8 pio)
{
    /* mask_a matches the 'aaaa' bits to be preserved */
    /* mask_c matches the 'cccc' bits to be preserved */
    uint16 mask_a = (uint16)0xfffe, mask_c = 0x00;
    for (uint8 i = 0 ; i < client->numPios ; i++)
    {
        if (client->pios[i] == pio)
        {
            /* Remove state bit of this pio from the states */
            uint16 left_a = ( client->states & mask_a ) >> 1;
            uint16 right_c= ( client->states & mask_c );
            client->states = left_a | right_c;

            /* Remove the pio from the array */
            for (uint8 j = i++ ; i < client->numPios ; i++, j++)
            {
                client->pios[j] = client->pios[i];
            }
            client->numPios--;
            client->pios[client->numPios] = EMPTY_PIO;

            /* All done */
            break;
        }
        mask_a = ( mask_a << 1 );
        mask_c = ( mask_c << 1 ) | 0x01;
    }
}

/*! Add a client to the list of clients */
static void addClient(Task task, uint8 pio)
{
    PioMonitorClient_t *client = findClient(task);
    if ( !client )
    {
        /* Either it's a new task, or the pios table for an existing task is full. */
        client = newClient(task);
        if ( !pio_monitor_state.cl_head )
        {
            /* First client, head and tail point to it */
            pio_monitor_state.cl_head = client;
            pio_monitor_state.cl_tail = client;
        }
        else
        {
            /* Append to the end of the list */
            pio_monitor_state.cl_tail->next = client;
            pio_monitor_state.cl_tail = client;
        }
    }
    addPioToClient(client,pio);
}

/*! Remove a client from the list of clients */
static void removeClient(Task task, uint8 pio)
{
    PioMonitorClient_t *client, *prev = NULL;
    for (client = pio_monitor_state.cl_head ; client != NULL ; prev = client, client = client->next)
    {
        if (client->task == task && findPioInClient(client,pio) != -1)
        {
            removePioFromClient(client,pio);
            if (client->numPios == 0)
            {
                /* Last PIO removed from the task, also remove from list */
                if (prev == NULL)
                {
                    /* Deleted the first node in the list */
                    pio_monitor_state.cl_head = client->next;
                }
                else
                {
                    /* Remove the node from the interior of the list */
                    prev->next = client->next;
                }
                if (client->next == NULL)
                {
                    /* Deleted the last node in the list */
                    pio_monitor_state.cl_tail = prev;
                }
                free(client);
                break;
            }
        }
    }
}

/*! Set the debounce paramters for the selected pins in the selected bank */
static void setPioBounce(uint32 bank, uint32 mask)
{
    if(PioDebounce32Bank(bank, mask, pio_monitor_state.debounce_reads, pio_monitor_state.debounce_period))
    {
        PIOM_DEBUG(("PIOM: updateDebounced: Monitoring not set for bank %d, mask %08x\n", bank, mask));
    }
    else
    {
        PIOM_DEBUG(("PIOM: updateDebounced bank %d, mask %08x\n", bank, mask));
    }
}

/*! Given a PIO (which has been added/removed from interest) update the debounced PIOs */
static void updateDebounced(uint8 pio)
{
    PioMonitorClient_t *client;
    const uint32 pio_bank = pio / 32;
    uint32 pio_bank_mask = 0;

    /* For each client ... */
    for (client = pio_monitor_state.cl_head;
         client != NULL;
         client = client->next)
    {
        /* For each PIO of the client ... */
        for (uint8 p = 0 ; p < client->numPios ; p++)
        {
            const uint32 client_bank = client->pios[p] / 32;
            if (client_bank == pio_bank)
            {
                const uint32 client_bank_pio = client->pios[p] - (32 * client_bank);
                const uint32 client_bank_pio_mask = 1UL << client_bank_pio;
                pio_bank_mask |= client_bank_pio_mask;
            }
        }
    }
    setPioBounce(pio_bank,pio_bank_mask);
}

/*! Update the debounced PIOs for all the registered tasks */
static void updateAllDebounced(void)
{
    uint32 masks[PIOM_NUM_BANKS] = { 0 };
    PioMonitorClient_t *client;

    /* For each client ... */
    for (client = pio_monitor_state.cl_head;
         client != NULL;
         client = client->next)
    {
        /* For each PIO of the client ... */
        for (uint8 p = 0 ; p < client->numPios ; p++)
        {
            const uint32 client_bank = client->pios[p] / 32;
            const uint32 client_bank_pio = client->pios[p] - (32 * client_bank);
            const uint32 client_bank_pio_mask = 1UL << client_bank_pio;
            masks[client_bank] |= client_bank_pio_mask;
        }
    }
    for (uint32 bank = 0 ; bank < PIOM_NUM_BANKS ; bank++)
    {
        setPioBounce(bank,masks[bank]);
    }
}

/*! Initialise the PIO monitor */
bool PioMonitorInit(Task task)
{
    UNUSED(task);
    PIOM_DEBUG(("PIOM: PioMonitorInit\n"));
    memset(&pio_monitor_state, 0, sizeof(pio_monitor_state));

    pio_monitor_state.task.handler = pioHandler;

    return TRUE;
}

Task PioMonitorGetTask(void)
{
    return &pio_monitor_state.task;
}

/*! Set the debounce parameters */
void PioMonitorSetDebounceParameters(uint16 debounce_reads, uint16 debounce_period )
{
    pio_monitor_state.debounce_reads = debounce_reads;
    pio_monitor_state.debounce_period = debounce_period;
}

/*! Send message to the registered clients that Input Event Manager is enabled */
static void enableClients(void)
{
    PioMonitorClient_t *client = NULL;

    /* We have the ownership of all the PIOs now. Update the debounce for them */
    updateAllDebounced();

    /* For each client ... */
    for (client = pio_monitor_state.cl_head;
         client != NULL;
         client = client->next)
    {
        /* Send PIO_MONITOR_ENABLE_CFM to client task */
        /* TODO: How many times should each task get this message? */
        /* Especially if a task registers more than MAX_PIOS_PER_TASK PIO's */
        PIOM_DEBUG(("PIOM: Sending PIO_MONITOR_ENABLE_CFM to task %p\n", client->task));
        MessageSend(client->task, PIO_MONITOR_ENABLE_CFM, NULL);
    }
}

/*! All clients have registered and configured their PIOs appropriately */
void PioMonitorEnable(void)
{
    PIOM_DEBUG(("PIOM: PioMonitorEnable\n"));

    /* Register with the platform to receive PIO events */
    MessagePioTask(&pio_monitor_state.task);

    /* Notify the clients of IEM that the module is enabled */
    enableClients();
}

/*! Register a client task's interest in a pio */
void PioMonitorRegisterTask(Task client, uint8 pio)
{
    PIOM_DEBUG(("PIOM: PioMonitorRegisterTask %d\n",pio));
    addClient(client, pio);
}

/*! De-Register a client task's interest in a pio */
void PioMonitorUnregisterTask(Task client, uint8 pio)
{
    PIOM_DEBUG(("PIOM: PioMonitorUnregisterTask %d\n",pio));
    removeClient(client, pio);
    updateDebounced(pio);
}
