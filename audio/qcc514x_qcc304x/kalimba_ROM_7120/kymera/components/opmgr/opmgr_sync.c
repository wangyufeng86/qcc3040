/****************************************************************************
 * Copyright (c) 2017 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file     opmgr_sync.c
 * \ingroup  opmgr
 *
 * Operator Manager Operator synchronisation utilities
 */

/****************************************************************************
Include Files
*/
#include "opmgr_private.h"

/* Check for valid choices of implementations of suspend_processing */

#ifndef OPMGR_SYNC_KICKED_IMPLEMENTATION
#error Define OPMGR_SYNC_KICKED_IMPLEMENTATION in the build configuration
#elif (OPMGR_SYNC_KICKED_IMPLEMENTATION != OPMGR_SYNC_KICKED_LEGACY) \
      && (OPMGR_SYNC_KICKED_IMPLEMENTATION != OPMGR_SYNC_KICKED_BASIC)
#error Unimplemented choice of OPMGR_SYNC_KICKED_IMPLEMENTATION
#endif

#ifndef OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION
#error Define OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION in the build configuration
#elif (OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION != OPMGR_SYNC_STRICT_TIMER_LEGACY)
#error Unimplemented choice of OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION
#endif

/**
 * \brief Helper function for finding out where an operator is running
 * or not.
 *
 * \param op_data The data structure of the operator to query
 */
bool opmgr_op_is_running(OPERATOR_DATA *op_data)
{
    return op_data->state == OP_RUNNING;
}

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call this from command/message handlers before modifying
 *        complex data shared between command/message handlers and data
 *        processing.
 * \param op_data       The data structure of the operator
 */
void opmgr_op_suspend_processing(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

    PL_ASSERT(!op_data->processing_suspended);

#if OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_LEGACY
    interrupt_block();
    op_data->processing_suspended = TRUE;
#elif OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_BASIC
    interrupt_block();
    op_data->processing_suspended = TRUE;
    op_data->processing_rerun_requested = FALSE;
    interrupt_unblock();
#endif /* OPMGR_SYNC_KICKED_IMPLEMENTATION */
}

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call this from command/message handlers after modifying
 *        complex data shared between command/message handlers and data
 *        processing.
 * \param op_data       The data structure of the operator
 */
void opmgr_op_resume_processing(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

    PL_ASSERT(op_data->processing_suspended);

#if OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_LEGACY
    op_data->processing_suspended = FALSE;
    interrupt_unblock();
#elif OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_BASIC
    interrupt_block();
    op_data->processing_suspended = FALSE;
    if (op_data->processing_rerun_requested)
    {
        op_data->processing_rerun_requested = FALSE;
        if (OP_RUNNING == op_data->state)
        {
            interrupt_unblock();
            /* Raise a bg int to process */
            opmgr_kick_operator(op_data);
            return;
        }
    }
    interrupt_unblock();
#endif /* OPMGR_SYNC_KICKED_IMPLEMENTATION */
}

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Call from command/message handlers while processing is
 *        suspended, to force data processing to run after
 *        opmgr_op_resume_processing. Without this call, data processing will
 *        only be triggered by opmgr_op_resume_processing if the operator
 *        was kicked while processing was suspended.
 * \param op_data       The data structure of the operator
 */
void opmgr_op_process_after_resume(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

#if OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_LEGACY
    NOT_USED(op_data);
#elif OPMGR_SYNC_KICKED_IMPLEMENTATION == OPMGR_SYNC_KICKED_BASIC
    PL_ASSERT(op_data->processing_suspended);

    /* Since the flags are declared packed, there is no guarantee
     * that the assignment is atomic (though it would hopefully
     * be a single byte access instruction)
     */
    interrupt_block();
    op_data->processing_rerun_requested = TRUE;
    interrupt_unblock();
#endif /* OPMGR_SYNC_KICKED_IMPLEMENTATION */
}

/**
 * \brief Helper for synchronisation between command/message handlers and data
 *        processing. Intended for asserts and similar checks that code
 *        accessing shared data is run only when processing is suspended.
 * \param op_data       The data structure of the operator
 * \return True if processing is suspended
 */
bool opmgr_op_is_processing_suspended(OPERATOR_DATA *op_data)
{
    return op_data->processing_suspended;
}

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this from command/message
 *        handlers before modifying complex data shared between
 *        command/message handlers and strict timer callback.
 * \param op_data       The data structure of the operator
 */
void opmgr_op_suspend_processing_strict(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

#if OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION == OPMGR_SYNC_STRICT_TIMER_LEGACY
    interrupt_block();
    op_data->processing_suspended = TRUE;
#endif
}

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this from command/message
 *        handlers after modifying complex data shared between
 *        command/message handlers and strict timer callback.
 * \param op_data       The data structure of the operator
 * \param timer_id      Pointer to the timer id field
 * \param timer_fn      Strict timer callback function
 * \param timer_data    Strict timer callback data
 */
void opmgr_op_resume_processing_strict(OPERATOR_DATA *op_data,
                                       tTimerId* timer_id,
                                       tTimerEventFunction timer_fn,
                                       void* timer_data)
{
    patch_fn_shared(opmgr);

#if OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION == OPMGR_SYNC_STRICT_TIMER_LEGACY
    NOT_USED(timer_id);
    NOT_USED(timer_fn);
    NOT_USED(timer_data);

    op_data->processing_suspended = FALSE;
    interrupt_unblock();
#endif
}

/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call from command/message
 *        handlers while processing is suspended, to force the timer
 *        handler to be run after opmgr_op_resume_processing. Without this
 *        call, opmgr_op_resume_processing_strict will only trigger
 *        the timer callback if opmgr_op_is_processing_suspended_strict
 *        was called while processing was suspended.
 * \param op_data       The data structure of the operator
 */
void opmgr_op_process_after_resume_strict(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

#if OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION == OPMGR_SYNC_STRICT_TIMER_LEGACY
    NOT_USED(op_data);
#endif
}


/**
 * \brief Helper for synchronisation between command/message handlers and
 *        strict timer driven data processing. Call this at the start of the
 *        strict timer callback. If it returns true, quit before accessing
 *        shared data. The timer callback will then be triggered again from
 *        opmgr_op_resume_processing_strict.
 * \param op_data       The data structure of the operator
 * \return True if processing is suspended
 */
bool opmgr_op_is_processing_suspended_strict(OPERATOR_DATA *op_data)
{
    patch_fn_shared(opmgr);

#if OPMGR_SYNC_STRICT_TIMER_IMPLEMENTATION == OPMGR_SYNC_STRICT_TIMER_LEGACY
    return op_data->processing_suspended;
#else
    /* Satisfy compiler if no implementation is selected */
    NOT_USED(op_data);
    return FALSE;
#endif
}
