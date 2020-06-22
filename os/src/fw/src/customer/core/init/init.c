/*
 * %%fullcopyright(2016)
 * %%version
 */
/**
 *  \file
 * Initialises core modules for P1
 * 
 */
#define IO_DEFS_MODULE_K32_CORE
#define IO_DEFS_MODULE_K32_PREFETCH
#define IO_DEFS_MODULE_K32_DOLOOP_CACHE

#include "io/io.h"
#include "hal/hal.h"

#include "init/init_private.h"

void initialise_os(void)
{
#if CHIP_HAS_CLEAN_SHALLOW_SLEEP_CLOCK_STOP_THAT_DEFAULTS_OFF
    /* Get shallow sleep to stop the clock cleanly */
    hal_set_reg_clock_stop_wind_down_sequence_en(1);
#endif

    L0_DBG_MSG1("Processor %d", THIS_PROCESSOR);
    L0_DBG_MSG1("Firmware ID %d", build_id_number);

    init_int();
    excep_enable(); /* Relies on int */
    memprot_enable(); /* Relies on int */
    init_itime();
    init_pmalloc();
    pioint_init(); /* Currently a no-op. */
    init_sched();
    ipc_init();
    appcmd_init();
    init_fault(); /* fault uses itime and sched */
#ifdef ENABLE_CACHE_TEST_LARGE_CODE
    cache_test();
#endif /* ENABLE_CACHE_TEST_LARGE_CODE */

    /* Disable internal counters to save current */
    /* Do not disable dbg_counters - QMDE needs them (TF-22835) */
    hal_set_prefetch_config_counters_en(0);
    hal_set_doloop_cache_config_counters_en(0);

    _init();
}
