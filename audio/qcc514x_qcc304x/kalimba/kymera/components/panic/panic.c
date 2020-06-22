/****************************************************************************
 * Copyright (c) 2010 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file panic.c
 * \ingroup panic
 *
 * Protest and die
 *
 * \section panic DESCRIPTION
 * This file contains all of the functions dealing with panicking the chip
 *
 */

#include "panic/panic_private.h"
#include "hal/hal.h"
#if defined(SUPPORTS_MULTI_CORE)
#include "ipc/ipc.h"
#endif
#ifndef KALIMBA_BUILD
#include "hal/halmgt.h"
#endif

#define UNUSED(x) ((void)(x))

#if defined(INSTALL_HYDRA)
#define report_panic(deathbed_confession, last_arg) \
            subreport_panic((deathbed_confession), (last_arg))
#else
#define report_panic(deathbed_confession, last_arg) UNUSED(last_arg)
#endif

/*
 * A static function to preserve data at panic
 * Kalimba needs a different approach
 */
static void panic_preserve_data(DIATRIBE_TYPE diatribe,
                                uint32 panic_time,
                                panicid deathbed_confession)
{
    volatile preserved_struct *this_preserved = get_preserved_struct();

    this_preserved->panic.last_arg = diatribe;
    /* Note when it all went wrong */
    this_preserved->panic.last_time = panic_time;
    /* Store the reason code in a preserved variable so that it can be
       reported after the chip has been reset (this also makes it easy to
       access from a debugger) */
    this_preserved->panic.last_id = deathbed_confession;
}

/**
 * Protest volubly and die
 *
 * IMPLEMENTATION NOTE
 *
 * Ideally this function would only perform trivial operations that do not
 * rely on any memory contents or other state. This is because it is likely
 * to be called when something has gone disasterously wrong, in which case
 * the state cannot be assumed to be valid. Functions from the HAL can be
 * considered reasonably safe, but other functions should generally be
 * avoided.
 *
 * Unfortunately compromises need to be made, such as signing the preserved
 * memory to ensure that the panic can be correctly reported after the chip
 * has been reset. These exceptions should be kept to a minimum.
 */
void panic_diatribe(panicid deathbed_confession, DIATRIBE_TYPE diatribe)
{
    TIME panic_time = hal_get_time();

    /* Block all interrupts to ensure that the state is accurately preserved */
#ifndef KALIMBA_BUILD
    hal_block_interrupts();
#else
    block_interrupts();
    patch_fn(panic_diatribe);
#endif
    L0_DBG_MSG2("PANIC 0x%x %d", deathbed_confession, diatribe);
    panic_preserve_data(diatribe, panic_time, deathbed_confession);

    /* Currently panic data is only sent to the curator, so only HYDRA */
    volatile preserved_struct *this_preserved = get_preserved_struct();

#if defined(SUPPORTS_MULTI_CORE)
    if (PROC_PRIMARY_CONTEXT())
    {
        PROC_ID_NUM proc;
        /* P0 is panicking, before dying: report panic details to the curator
         * and try to halt other cores, so to leave the system in
         * a coherent state. */
        report_panic(deathbed_confession, this_preserved->panic.last_arg);
        for (proc = PROC_PROCESSOR_1; proc < PROC_PROCESSOR_MAX; proc++)
        {
            ipc_halt_processor(proc);
        }
    }
    else
    {
        /* Px is panicking, before dying: report panic details to P0. */

        /* Cast away from pointer to address and back to a pointer to discard
         * volatile qualifier.
         * In this case it can be safe to send the data address over a message
         * to the other core, and the other core can also read the data as
         * non-"volatile". The message is unexpected, the address is not stored
         * or cached and the writing core has already stopped when data is read.
         * */
        uintptr_t address = (uintptr_t)&this_preserved->panic;
        ipc_panic_report(sizeof(panic_preserved), (void *)address);
    }
#else
    report_panic(deathbed_confession, this_preserved->panic.last_arg);
#endif /* defined(SUPPORTS_MULTI_CORE) */


#ifndef KALIMBA_BUILD
    /* Ask any other processors to halt (but do not wait for acknowledgement
       in case they are unresponsive) and prepare this processor's preserved
       memory */
    reset_panic_prepare();

    /* Perform a stack backtrace (using the address of the function parameter
       as a rough starting point). The implementation of this function is
       (very) target specific, so is provided as part of the CRT. */

    /* Indicate the reason code on the LEDs and/or PIOs if required */
    PIO_TRACE_POINT(PANIC);
#endif

#if !defined(DESKTOP_TEST_BUILD) && !defined(PRODUCTION_BUILD)
    /* The watchdog may have been enabled, so kick it continuously to ensure
       that it never causes a reset */
    for (;;)
    {
#ifndef KALIMBA_BUILD
        hal_kick_watchdog();
#endif
    }
#else /* !DESKTOP_TEST_BUILD && !PRODUCTION_BUILD */

    /*
     * On a standalone chip we would reset or halt the processor as
     * appropriate.  Here, we wait for the Curator to take action
     * based on the subreport message.
     */

    /* On an embedded platform this enters an infinite loop (at least until
       the chip is reset, e.g. by the watchdog if enabled) */
    /*NOTREACHED*/
    exit((int) deathbed_confession);
#endif /* !DESKTOP_TEST_BUILD && !PRODUCTION_BUILD */
}

/**
 * Protest pathetically and die
 */
void panic(panicid deathbed_confession)
{
    panic_diatribe(deathbed_confession, 0U);
}

#if defined(SUPPORTS_MULTI_CORE)
/* PX panicked and reported panic details to P0. This is P0 receiving such data.
 * P0 reports other core's panic details, then panics (see panic_diatribe for P0). */
void panic_remote_proc(PROC_ID_NUM remote_proc_id,
                       uint16 data_length,
                       void *panic_data)
{
    volatile panic_preserved *preserved_data;
    uint16 arg;
    uint8 proc_id;

    proc_id = proc_serialize(remote_proc_id);
    if (PROC_PRIMARY_CONTEXT())
    {
        if (data_length == sizeof(panic_preserved) && panic_data != NULL)
        {
            preserved_data = (volatile panic_preserved *) panic_data;
            arg = (uint16) preserved_data->last_arg;
            subreport_event(CCP_EVENT_LEVEL_PANIC,
                            preserved_data->last_id,
                            proc_id,
                            1,
                            preserved_data->last_time,
                            &arg,
                            1);
        }
    }
    panic_diatribe(PANIC_AUDIO_PX_PANICKED, proc_id);
}
#endif /* defined(SUPPORTS_MULTI_CORE) */
