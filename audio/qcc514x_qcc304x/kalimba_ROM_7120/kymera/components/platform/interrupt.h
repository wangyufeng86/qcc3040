// *****************************************************************************
// Copyright (c) 2005 - 2017 Qualcomm Technologies International, Ltd.        http://www.csr.com
// %%version
//
// $Change: 3453471 $  $DateTime: 2020/06/09 17:30:09 $
// *****************************************************************************

#ifndef INTERRUPT_HEADER_INCLUDED
#define INTERRUPT_HEADER_INCLUDED

#ifdef DEBUG_ON
   #ifndef INTERRUPT_PROFILER_ON
      #define INTERRUPT_PROFILER_ON
   #endif
#endif

.CONST HYDRA_PUSHED_BEFORE_CHECK 6;
.CONST HYDRA_STORE_STATE_SIZE    48;

// Total stack use by interrupt handler, used in stack size calculation
// (in units of words)
#ifdef CHIP_BASE_BC7
.CONST $INTERRUPT_STORE_STATE_SIZE (HYDRA_STORE_STATE_SIZE + 1);    // one extra used for clock speed
.CONST PUSHED_BEFORE_CHECK (HYDRA_PUSHED_BEFORE_CHECK+1);
#else
.CONST $INTERRUPT_STORE_STATE_SIZE HYDRA_STORE_STATE_SIZE;
.CONST PUSHED_BEFORE_CHECK HYDRA_PUSHED_BEFORE_CHECK;
#endif

// Constants for stack-checking in the interrupt handler (debug mode).
// Note that the "store" and "restore" sizes are different because there
// is already some data on the stack at the point of doing the "store" check.
.CONST $INTERRUPT_STORE_CHECK_SIZE     ($INTERRUPT_STORE_STATE_SIZE - PUSHED_BEFORE_CHECK);
.CONST $INTERRUPT_RESTORE_CHECK_SIZE   ($INTERRUPT_STORE_STATE_SIZE - 1); // one word already popped by the time of restore

#endif
