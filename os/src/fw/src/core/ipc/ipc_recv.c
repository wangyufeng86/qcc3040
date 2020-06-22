/* Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "ipc/ipc_private.h"


/**
 * Leaves the IPC receive buffer pages mapped in if set to TRUE.
 */
static bool leave_pages_mapped = FALSE;

void ipc_leave_recv_buffer_pages_mapped(void)
{
    leave_pages_mapped = TRUE;
}

/**
 * @brief Update the IPC receive buffer behind pointer.
 *
 * Either frees the now unused buffer pages or leaves them mapped in depending
 * on the value of @c leave_pages_mapped.
 */
static void ipc_recv_update_behind(void)
{
    if( leave_pages_mapped )
    {
        buf_update_behind(ipc_data.recv);
    }
    else
    {
        buf_update_behind_free(ipc_data.recv);
    }
}

/**
 * @brief Determines whether ipc_recv() should try to sleep.
 * @return TRUE  If there's no pending work and it's safe to sleep.
 * @return FALSE If ipc_recv() shouldn't sleep.
 */
static bool ipc_recv_should_sleep(void)
{
    bool should_sleep = FALSE;

    if (!ipc_data.pending && !background_work_pending)
    {
        /* The send queue must be clear before sleeping or we may block waiting
           for a response to a message that hasn't been sent yet. */
        block_interrupts();
        should_sleep = ipc_clear_queue();
        unblock_interrupts();
    }

    return should_sleep;
}

/**
 * @brief Put the processor into shallow sleep if possible.
 */
static void ipc_recv_try_to_sleep(void)
{
    while (ipc_recv_should_sleep())
    {
        /* Purely nominal timeout - it is ignored by dorm_shallow_sleep. */
        TIME timeout = time_add(hal_get_time(), SECOND);
        dorm_shallow_sleep(timeout);
    }
}

void ipc_recv(IPC_SIGNAL_ID recv_id, void *blocking_msg)
{
    bool changed_background_work_pending = FALSE;

    /* Memory must be provided for the response. */
    assert(blocking_msg);

    /* Sleep until we see the IPC interrupt fire, and then process the
     * entries.  Keep doing this until we see the recv_id message */
    /*lint -e(716) Loop will terminate when the recv_id message is found */
    while (TRUE)
    {
        ipc_recv_try_to_sleep();

        if (ipc_recv_handler(recv_id, blocking_msg))
        {
            /* Restore indicator of pending background work.
             * Note: this is safe because code running from interrupt handlers
             * only increases TotalNumMessages, so once background_work_pending is set,
             * it doesn't get cleared until the scheduler has a chance to run
             * background work. */
            if (changed_background_work_pending)
            {
                background_work_pending = TRUE;
            }
            break;
        }

        if (background_work_pending)
        {
            /* We can't service background work anyway until
             * an expected IPC response is received, so no need to prevent
             * processor from shallow sleeping. */
            background_work_pending = FALSE;
            /* Remember that we tampered with it */
            changed_background_work_pending = TRUE;
        }
    }
}

#ifdef DESKTOP_TEST_BUILD
static bool ipc_recv_process_cpu_static_callback_message(IPC_SIGNAL_ID id,
                                                         const void *msg,
                                                         uint16 msg_length)
{
    UNUSED(msg_length);
    switch(id)
    {
#ifdef BLUESTACK_IF_MODULE_PRESENT
    case IPC_SIGNAL_ID_BLUESTACK_PRIM:
    case IPC_SIGNAL_ID_BLUESTACK_PRIM_RECEIVED:
    case IPC_SIGNAL_ID_APP_MESSAGE_RECEIVED:
    case IPC_SIGNAL_ID_STREAM_L2CAP_SINK:
        ipc_bluestack_handler(id, msg);
        break;
#endif /* BLUESTACK_IF_MODULE_PRESENT */
    case IPC_SIGNAL_ID_MMU_HANDLE_ALLOC_REQ:
    case IPC_SIGNAL_ID_MMU_HANDLE_FREE:
    case IPC_SIGNAL_ID_BUF_TAIL_FREE:
    case IPC_SIGNAL_ID_BUF_UPDATE_BEHIND_FREE:
        ipc_mmu_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_PIO_SET_OWNER:
    case IPC_SIGNAL_ID_PIO_GET_OWNER:
    case IPC_SIGNAL_ID_PIO_SET_PULL_EN:
    case IPC_SIGNAL_ID_PIO_GET_PULL_EN:
    case IPC_SIGNAL_ID_PIO_SET_PULL_DIR:
    case IPC_SIGNAL_ID_PIO_GET_PULL_DIR:
    case IPC_SIGNAL_ID_PIO_SET_PULL_STR:
    case IPC_SIGNAL_ID_PIO_GET_PULL_STR:
    case IPC_SIGNAL_ID_PIO_GET_UNUSED:
    case IPC_SIGNAL_ID_PIO_SET_PIO_MUX:
    case IPC_SIGNAL_ID_PIO_SET_PAD_MUX:
    case IPC_SIGNAL_ID_PIO_GET_PAD_MUX:
    case IPC_SIGNAL_ID_PIO_GET_PIO_MUX:
    case IPC_SIGNAL_ID_PIO_SET_DRIVE_STRENGTH:
    case IPC_SIGNAL_ID_PIO_GET_DRIVE_STRENGTH:
    case IPC_SIGNAL_ID_PIO_SET_STICKY:
    case IPC_SIGNAL_ID_PIO_GET_STICKY:
    case IPC_SIGNAL_ID_PIO_SET_SLEW:
    case IPC_SIGNAL_ID_PIO_GET_SLEW:
    case IPC_SIGNAL_ID_PIO_ACQUIRE:
    case IPC_SIGNAL_ID_PIO_RELEASE:
    case IPC_SIGNAL_ID_PIO_SET_XIO_MODE:
    case IPC_SIGNAL_ID_PIO_GET_XIO_MODE:
    case IPC_SIGNAL_ID_PIO_SET_DRIVE_ENABLE:
    case IPC_SIGNAL_ID_PIO_GET_DRIVE_ENABLE:
    case IPC_SIGNAL_ID_PIO_SET_DRIVE:
    case IPC_SIGNAL_ID_PIO_GET_DRIVE:
    case IPC_SIGNAL_ID_PIO_SET_FUNC_BITSERIAL:
    case IPC_SIGNAL_ID_PIO_SET_FUNC_UART:
        ipc_pio_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_FAULT:
        ipc_fault_panic_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_SMALLOC_REQ:
    case IPC_SIGNAL_ID_SFREE:
        ipc_malloc_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_P1_DEEP_SLEEP_MSG:
    case IPC_SIGNAL_ID_DEEP_SLEEP_WAKEUP_SOURCE:
        ipc_deep_sleep_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_VM_GET_FW_VERSION:
        vm_trap_VmGetFwVersion();
        break;
    case IPC_SIGNAL_ID_VM_READ_SECURITY:
    case IPC_SIGNAL_ID_VM_ENABLE_SECURITY:
        ipc_vm_handler(id, msg);
        break;
#ifdef TRAPSET_UART
    case IPC_SIGNAL_ID_STREAM_UART_SINK:
        ipc_uart_handler(id);
        break;
#endif
#ifdef TRAPSET_TEST
    case IPC_SIGNAL_ID_TESTTRAP_BT_REQ:
        ipc_test_trap_handler(id, msg);
        break;
#endif
#if TRAPSET_SD_MMC
    case IPC_SIGNAL_ID_SD_MMC_SLOT_INIT_REQ:
    case IPC_SIGNAL_ID_SD_MMC_READ_DATA_REQ:
        ipc_sd_mmc_handler(id, msg);
        break;
#endif
    default:
        /* This is not a P0 specific static callback message. */
        return FALSE;
    }

    /* Message has been handled. */
    return TRUE;
}
#endif /* defined(PROCESSOR_P0) || defined(DESKTOP_TEST_BUILD) */


static bool ipc_recv_process_cpu_static_callback_message(IPC_SIGNAL_ID id,
                                                         const void *msg,
                                                         uint16 msg_length)
{
    switch(id)
    {
    case IPC_SIGNAL_ID_BLUESTACK_PRIM:
        ipc_bluestack_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_APP_MSG:
    case IPC_SIGNAL_ID_APP_SINK_SOURCE_MSG:
    case IPC_SIGNAL_ID_APP_MSG_TO_HANDLER:
        ipc_trap_api_handler(id, msg, msg_length);
        break;
    case IPC_SIGNAL_ID_IPC_LEAVE_RECV_BUFFER_PAGES_MAPPED:
        ipc_leave_recv_buffer_pages_mapped();
        break;
    case IPC_SIGNAL_ID_STREAM_DESTROYED:
    case IPC_SIGNAL_ID_OPERATORS_DESTROYED:
        ipc_stream_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_MEMORY_ACCESS_FAULT_INFO:
        ipc_memory_access_fault_handler(id, msg);
        break;
    default:
        /* This is not a P1 specific static callback message. */
        return FALSE;
    }

    /* Message has been handled. */
    return TRUE;
}

static void ipc_recv_process_cpu_autogen_message(IPC_SIGNAL_ID id,
                                                 const void *msg,
                                                 uint16 msg_length)
{
    ipc_trap_api_handler(id, msg, msg_length);
}

static bool ipc_recv_process_static_callback_message(IPC_SIGNAL_ID id,
                                                     const void *msg,
                                                     uint16 msg_length)
{
    switch(id)
    {
        /* The cases here are for static callback messages that are handled
           similarly on both processors. */
    case IPC_SIGNAL_ID_TEST_TUNNEL_PRIM:
        ipc_test_tunnel_handler(id, msg, msg_length);
        break;
    case IPC_SIGNAL_ID_SCHED_MSG_PRIM:
        ipc_sched_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_PFREE:
        ipc_malloc_msg_handler(id, msg);
        break;
    case IPC_SIGNAL_ID_SIGNAL_INTERPROC_EVENT:
        hal_set_reg_interproc_event_1(1);
        break;
    case IPC_SIGNAL_ID_TRAP_API_VERSION:
        ipc_trap_api_version_prim_handler(id, msg);
        break;
    default:
        /* Defer to the processor specific handler. */
        return ipc_recv_process_cpu_static_callback_message(id, msg,
                                                            msg_length);
    }
    return TRUE;
}

static bool ipc_recv_process_autogen_message(IPC_SIGNAL_ID id, const void *msg,
                                             uint16 msg_length)
{
    /* The autogenerated signals are numbered by the corresponding
     * trap ID, which contains the trapset index in the upper word,
     * and trapsets are numbered from 1.  Hence this is the lowest
     * possible number for an autogenerated signal */
    if (id >= 0x10000)
    {
        ipc_recv_process_cpu_autogen_message(id, msg, msg_length);
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Processes a single non-blocking response IPC message.
 */
static void ipc_recv_process_async_message(IPC_SIGNAL_ID id, const void *msg,
                                           uint16 msg_length)
{
    if(ipc_recv_process_static_callback_message(id, msg, msg_length))
    {
        return;
    }

    if(ipc_recv_process_autogen_message(id, msg, msg_length))
    {
        return;
    }

    panic_diatribe(PANIC_IPC_UNHANDLED_MESSAGE_ID, id);
}

/**
 * @brief Processes a single IPC message.
 *
 * @return TRUE if the message was the blocking message that ipc_recv is waiting
 *         for. FALSE if it was a static callback or autogen handled message.
 */
static bool ipc_recv_process_message(IPC_SIGNAL_ID blocking_id,
                                     void *blocking_msg,
                                     const void *msg, uint16 msg_length)
{
    /* The header always comes first so we can always cast to it */
    IPC_SIGNAL_ID id = ((const IPC_HEADER *)msg)->id;

    if(id == blocking_id)
    {
        /* Received the response for the expected blocking message. */
        assert(blocking_msg);
        memcpy(blocking_msg, msg, msg_length);
        return TRUE;
    }

    ipc_recv_process_async_message(id, msg, msg_length);

    return FALSE;
}

bool ipc_recv_handler(IPC_SIGNAL_ID blocking_id, void *blocking_msg)
{
    uint16f n_processed = 0;
    bool blocking_message_seen = FALSE;

    block_interrupts();
    ipc_data.pending = FALSE;
    unblock_interrupts();

    /* We consume everything there is because IPC is a relatively high-priority
     * task */
    while(BUF_ANY_MSGS_TO_SEND(ipc_data.recv) &&
                                        n_processed < IPC_MAX_RECV_MSGS)
    {
        const void *msg = (const void *)buf_map_back_msg(ipc_data.recv);
        const uint16 msg_length = buf_get_back_msg_len(ipc_data.recv);

        buf_update_back(ipc_data.recv);

        blocking_message_seen |=
            ipc_recv_process_message(blocking_id, blocking_msg,
                                     msg, msg_length);

        /* Free ipc message as it has already been processed in
         * message handler called above */
        ipc_recv_update_behind();

        n_processed++;
    }

    /* Reschedule ourselves if there's anything left to process */
    if (BUF_ANY_MSGS_TO_SEND(ipc_data.recv))
    {
        ipc_data.pending = TRUE;
        GEN_BG_INT(ipc);
    }

    return blocking_message_seen;
}
