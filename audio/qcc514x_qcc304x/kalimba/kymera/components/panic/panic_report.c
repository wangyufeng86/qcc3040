/****************************************************************************
 * Copyright (c) 2010 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file panic_report.c
 * \ingroup panics
 *
 * Report the last panic ID to the host
 */

#include "panic/panic_private.h"

/**
 * Report the last panic ID to the host
 */
void panic_report(void)
{
    volatile preserved_struct *this_preserved = get_preserved_struct();

    fault_diatribe(FAULT_HYDRA_PANIC, (uint16) this_preserved.panic.last_id);
}
