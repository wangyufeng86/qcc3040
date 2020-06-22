/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file 
 * Private header file for fault appcmd interface.
 */

#ifndef FAULT_APPCMD_H
#define FAULT_APPCMD_H

#include "appcmd/appcmd.h"
#include "hydra_log/hydra_log.h"

#if defined(APPCMD_MODULE_PRESENT) && defined(ENABLE_APPCMD_TEST_ID_FAULT_TEST)
#include "timed_event/rtime_types.h"

/** Keeps the timestamp of the last groan. */
extern volatile TIME fault_test_last_groan;

/** Registers the appcmd handler for fault. */
#define fault_install_appcmd_handler() \
    (void)appcmd_add_test_handler(APPCMD_TEST_ID_FAULT_TEST, fault_test)
APPCMD_RESPONSE fault_test(APPCMD_TEST_ID command,
                           uint32 * params,
                           uint32 * result);

/** Lets the test code know that the groan has been groaned. */
#define record_last_fault() fault_test_last_groan = MAX(hal_get_time(), 1);

#else /* APPCMD_MODULE_PRESENT && ENABLE_APPCMD_TEST_ID_FAULT_TEST */
#define record_last_fault()
#endif /* APPCMD_MODULE_PRESENT && ENABLE_APPCMD_TEST_ID_FAULT_TEST */

#endif /* FAULT_APPCMD_H */
