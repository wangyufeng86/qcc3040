
/* *************************************************************************  *
   COMMERCIAL IN CONFIDENCE
   Copyright (C) 2018 Qualcomm Technologies International Ltd.

   Qualcomm Technologies International Ltd.
   Churchill House,
   Cambridge Business Park,
   Cowley Park,
   Cambridge, CB4 0WZ. UK
   http://www.qualcomm.com

   $Id: //depot/bg/Staging/QCC514x_QCC304x.SRC.1.0/audio/qcc514x_qcc304x/kalimba_ROM_7120/kymera/components/io/streplus_audio/d00/io_map.h#34 $
   $Name$
   $Source$

   DESCRIPTION
      Hardware declarations header file (lower level).
      Lists memory mapped register addresses.

   INTERFACE
      Entry   :-
      Exit    :-

   MODIFICATIONS
      1.0    12/04/99    RWY    First created.
      1.x    xx/xx/xx    RWY    Automatically generated.

*  *************************************************************************  */

#define __IO_MAP_H__

/* We need this for the types of registers which are enums: */
#include "io_defs.h"




#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_TRACE
#define __IO_MAP_H__IO_MAP_MODULE_K32_TRACE

/* -- k32_trace -- Kalimba 32-bit Trace Control registers. -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  TRACE_0_CFG;                         /* RW  10 bits */
extern volatile uint32  TRACE_0_DMEM_BASE_ADDR;              /* RW  32 bits */
extern volatile uint32  TRACE_0_DMEM_CFG;                    /* RW  13 bits */
extern volatile uint32  TRACE_0_END_TRIGGER;                 /* RW  32 bits */
extern volatile uint32  TRACE_0_START_TRIGGER;               /* RW  32 bits */
extern volatile uint32  TRACE_0_TBUS_BASE_ADDR;              /* RW  32 bits */
extern volatile uint32  TRACE_0_TBUS_CFG;                    /* RW  30 bits */
extern volatile uint32  TRACE_0_TRIGGER_CFG;                 /* RW  12 bits */
extern volatile uint32  TRACE_1_CFG;                         /* RW  10 bits */
extern volatile uint32  TRACE_1_DMEM_BASE_ADDR;              /* RW  32 bits */
extern volatile uint32  TRACE_1_DMEM_CFG;                    /* RW  13 bits */
extern volatile uint32  TRACE_1_END_TRIGGER;                 /* RW  32 bits */
extern volatile uint32  TRACE_1_START_TRIGGER;               /* RW  32 bits */
extern volatile uint32  TRACE_1_TBUS_BASE_ADDR;              /* RW  32 bits */
extern volatile uint32  TRACE_1_TBUS_CFG;                    /* RW  30 bits */
extern volatile uint32  TRACE_1_TRIGGER_CFG;                 /* RW  12 bits */
extern volatile uint32  TRACE_ACCESS_CTRL;                   /* RW   4 bits */
extern volatile uint32  TRACE_DEBUG_SEL;                     /* RW   4 bits */
extern volatile k32_trace__mutex_lock_enum  TRACE_MUTEX_LOCK;                    /* RW   4 bits */

/* Declarations of read only registers */
extern volatile uint32  TRACE_0_TRIGGER_STATUS;              /* R    6 bits */
extern volatile uint32  TRACE_1_TRIGGER_STATUS;              /* R    6 bits */
extern volatile uint32  TRACE_DMEM_STATUS;                   /* R    4 bits */
extern volatile uint32  TRACE_TBUS_STATUS;                   /* R    4 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_TRACE */


#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_DOLOOP_CACHE
#define __IO_MAP_H__IO_MAP_MODULE_K32_DOLOOP_CACHE

/* -- k32_doloop_cache -- Kalimba 32-bit DoLoop Cache Control registers. -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  DOLOOP_CACHE_CONFIG;                 /* RW   2 bits */

/* Declarations of read only registers */
extern volatile uint32  DOLOOP_CACHE_FILL_COUNT;             /* R   32 bits */
extern volatile uint32  DOLOOP_CACHE_HIT_COUNT;              /* R   32 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_DOLOOP_CACHE */





#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_TIMERS
#define __IO_MAP_H__IO_MAP_MODULE_K32_TIMERS

/* -- k32_timers -- Kalimba 32-bit Timers Control registers -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  TIMER1_EN;                           /* RW   1 bits */
extern volatile uint32  TIMER1_TRIGGER;                      /* RW  32 bits */
extern volatile uint32  TIMER2_EN;                           /* RW   1 bits */
extern volatile uint32  TIMER2_TRIGGER;                      /* RW  32 bits */

/* Declarations of read only registers */
extern volatile uint32  TIMER_TIME;                          /* R   32 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_TIMERS */




#if defined(IO_DEFS_MODULE_FREQ_COUNTER) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_FREQ_COUNTER
#define __IO_MAP_H__IO_MAP_MODULE_FREQ_COUNTER

/* -- freq_counter -- Digital frequency counter -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  FREQ_COUNT_CONTROL;                  /* RW  12 bits */
extern volatile uint32  FREQ_COUNT_REF_CLK_TARGET_EDGE_COUNT;  /* RW  32 bits */

/* Declarations of read only registers */
extern volatile uint32  FREQ_COUNT_MON_CLK_EDGE_COUNT;       /* R   32 bits */
extern volatile freq_count_state  FREQ_COUNT_STATE;                    /* R    2 bits */

/* Declarations of register aliases*/
extern volatile uint32  FREQ_COUNT_CONTROL_BT;               /* A   12 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_FREQ_COUNTER */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_FREQ_COUNTER) */


#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_PREFETCH
#define __IO_MAP_H__IO_MAP_MODULE_K32_PREFETCH

/* -- k32_prefetch -- Kalimba 32-bit Prefetch Control registers. -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  PREFETCH_CONFIG;                     /* RW   1 bits */
extern volatile uint32  PREFETCH_FLUSH;                      /* RW   1 bits */

/* Declarations of read only registers */
extern volatile uint32  PREFETCH_DEBUG;                      /* R   25 bits */
extern volatile uint32  PREFETCH_DEBUG_ADDR;                 /* R   32 bits */
extern volatile uint32  PREFETCH_PREFETCH_COUNT;             /* R   32 bits */
extern volatile uint32  PREFETCH_REQUEST_COUNT;              /* R   32 bits */
extern volatile uint32  PREFETCH_WAIT_OUT_COUNT;             /* R   32 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_PREFETCH */






#if defined(IO_DEFS_MODULE_ANC) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_ANC
#define __IO_MAP_H__IO_MAP_MODULE_ANC

/* -- anc -- Control registers for ANC block -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  ANC_CONTROL0;                        /* RW  10 bits */
extern volatile uint32  ANC_CONTROL1;                        /* RW  16 bits */
extern volatile uint32  ANC_CONTROL10;                       /* RW   7 bits */
extern volatile uint32  ANC_CONTROL11;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL12;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL13;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL14;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL15;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL16;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL17;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL18;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL19;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL20;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL21;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL22;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL23;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL26;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL27;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL28;                       /* RW   4 bits */
extern volatile uint32  ANC_CONTROL29;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL30;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL31;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL32;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL33;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL34;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL35;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL38;                       /* RW  16 bits */
extern volatile uint32  ANC_CONTROL39;                       /* RW   8 bits */
extern volatile uint32  ANC_CONTROL40;                       /* RW   4 bits */
extern volatile uint32  ANC_CONTROL9;                        /* RW   7 bits */
extern volatile uint32  CDC_ANC0_IIR_COEFF_1_CTL;            /* RW   8 bits */
extern volatile uint32  CDC_ANC0_IIR_COEFF_2_CTL;            /* RW   8 bits */
extern volatile uint32  CDC_ANC1_IIR_COEFF_1_CTL;            /* RW   8 bits */
extern volatile uint32  CDC_ANC1_IIR_COEFF_2_CTL;            /* RW   8 bits */

/* Declarations of read only registers */
extern volatile uint32  ANC_STATUS0;                         /* R    3 bits */
extern volatile uint32  ANC_STATUS1;                         /* R    3 bits */

/* Declarations of register aliases*/
extern volatile uint32  ANC_CONTROL24;                       /* A    8 bits */
extern volatile uint32  ANC_CONTROL25;                       /* A    8 bits */
extern volatile uint32  ANC_CONTROL36;                       /* A    8 bits */
extern volatile uint32  ANC_CONTROL37;                       /* A    8 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_ANC */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_ANC) */







#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_MC_INTER_PROC_KEYHOLE
#define __IO_MAP_H__IO_MAP_MODULE_K32_MC_INTER_PROC_KEYHOLE

/* -- k32_mc_inter_proc_keyhole -- Kalimba 32-bit Multicore inter-processor communication keyhole register block -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  INTER_PROC_KEYHOLE_ACCESS_CTRL;      /* RW   4 bits */
extern volatile uint32  INTER_PROC_KEYHOLE_ADDR;             /* RW  32 bits */
extern volatile uint32  INTER_PROC_KEYHOLE_CTRL;             /* RW   8 bits */
extern volatile k32_mc_inter_proc_keyhole__mutex_lock_enum  INTER_PROC_KEYHOLE_MUTEX_LOCK;       /* RW   4 bits */
extern volatile uint32  INTER_PROC_KEYHOLE_WRITE_DATA;       /* RW  32 bits */

/* Declarations of read only registers */
extern volatile uint32  INTER_PROC_KEYHOLE_READ_DATA;        /* R   32 bits */
extern volatile inter_proc_keyhole_status  INTER_PROC_KEYHOLE_STATUS;           /* R    1 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_MC_INTER_PROC_KEYHOLE */








#if defined(IO_DEFS_MODULE_K32_MC_CONTROL) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_MC_CONTROL
#define __IO_MAP_H__IO_MAP_MODULE_K32_MC_CONTROL

/* -- k32_mc_control -- Kalimba general control registers for various functions that don't relate to the other registers at this level -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  KALIMBA_CONTROL_ACCESS_CTRL;         /* RW   4 bits */
extern volatile k32_mc_control__mutex_lock_enum  KALIMBA_CONTROL_MUTEX_LOCK;          /* RW   4 bits */
extern volatile kalimba_debug_select  KALIMBA_DEBUG_SELECT;                /* RW   5 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_MC_CONTROL */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_K32_MC_CONTROL) */








#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_CORE
#define __IO_MAP_H__IO_MAP_MODULE_K32_CORE

/* -- k32_core -- Kalimba 32-bit Core Control registers -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  ARITHMETIC_MODE;                     /* RW   5 bits */
extern volatile uint32  BITREVERSE_VAL;                      /* RW  32 bits */
extern volatile uint32  DBG_COUNTERS_EN;                     /* RW   1 bits */
extern volatile uint32  FRAME_POINTER;                       /* RW  32 bits */
extern volatile uint32  MM_DOLOOP_END;                       /* RW  32 bits */
extern volatile uint32  MM_DOLOOP_START;                     /* RW  32 bits */
extern volatile uint32  MM_QUOTIENT;                         /* RW  32 bits */
extern volatile uint32  MM_REM;                              /* RW  32 bits */
extern volatile uint32  MM_RINTLINK;                         /* RW  32 bits */
extern volatile uint32  STACK_END_ADDR;                      /* RW  32 bits */
extern volatile uint32  STACK_POINTER;                       /* RW  32 bits */
extern volatile uint32  STACK_START_ADDR;                    /* RW  32 bits */
extern volatile uint32  TEST_REG_0;                          /* RW  32 bits */
extern volatile uint32  TEST_REG_1;                          /* RW  32 bits */
extern volatile uint32  TEST_REG_2;                          /* RW  32 bits */
extern volatile uint32  TEST_REG_3;                          /* RW  32 bits */

/* Declarations of read only registers */
extern volatile uint32  BITREVERSE_ADDR;                     /* R   32 bits */
extern volatile uint32  BITREVERSE_DATA;                     /* R   32 bits */
extern volatile uint32  BITREVERSE_DATA16;                   /* R   32 bits */
extern volatile uint32  NUM_CORE_STALLS;                     /* R   32 bits */
extern volatile uint32  NUM_INSTRS;                          /* R   32 bits */
extern volatile uint32  NUM_INSTR_EXPAND_STALLS;             /* R   32 bits */
extern volatile uint32  NUM_MEM_ACCESS_STALLS;               /* R   32 bits */
extern volatile uint32  NUM_RUN_CLKS;                        /* R   32 bits */
extern volatile uint32  PC_STATUS;                           /* R   32 bits */
extern volatile uint32  STACK_OVERFLOW_PC;                   /* R   32 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_CORE */





#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_MONITOR
#define __IO_MAP_H__IO_MAP_MODULE_K32_MONITOR

/* -- k32_monitor -- Kalimba 32-bit Monitor Control registers. -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  DM1_PROG_EXCEPTION_REGION_END_ADDR;  /* RW  32 bits */
extern volatile uint32  DM1_PROG_EXCEPTION_REGION_START_ADDR;  /* RW  32 bits */
extern volatile uint32  DM2_PROG_EXCEPTION_REGION_END_ADDR;  /* RW  32 bits */
extern volatile uint32  DM2_PROG_EXCEPTION_REGION_START_ADDR;  /* RW  32 bits */
extern volatile uint32  EXCEPTION_EN;                        /* RW   2 bits */
extern volatile uint32  PM_PROG_EXCEPTION_REGION_END_ADDR;   /* RW  32 bits */
extern volatile uint32  PM_PROG_EXCEPTION_REGION_START_ADDR;  /* RW  32 bits */
extern volatile uint32  PROG_EXCEPTION_REGION_ENABLE;        /* RW   4 bits */

/* Declarations of read only registers */
extern volatile uint32  EXCEPTION_PC;                        /* R   32 bits */
extern volatile exception_type  EXCEPTION_TYPE;                      /* R    4 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_MONITOR */



#if defined(IO_DEFS_MODULE_AOV) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_AOV
#define __IO_MAP_H__IO_MAP_MODULE_AOV

/* -- aov -- AoV registers -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  AOV_ACCESS_CTRL;                     /* RW   4 bits */
extern volatile uint32  AOV_CTL;                             /* RW   9 bits */
extern volatile uint32  AOV_DEBUG_SELECT;                    /* RW   4 bits */
extern volatile uint32  AOV_EVENT_CLEAR;                     /* RW   7 bits */
extern volatile uint32  AOV_EVENT_MASK;                      /* RW   7 bits */
extern volatile uint32  AOV_KEYHOLE;                         /* RW  13 bits */
extern volatile aov__mutex_lock_enum  AOV_MUTEX_LOCK;                      /* RW   4 bits */
extern volatile uint32  AOV_RAMS_DS_EN;                      /* RW   2 bits */
extern volatile uint32  AOV_RAMS_EMA;                        /* RW  10 bits */
extern volatile uint32  AOV_RAMS_LS_EN;                      /* RW   2 bits */
extern volatile uint32  AOV_RAMS_RAWL;                       /* RW   6 bits */
extern volatile uint32  AOV_RAMS_SD_EN;                      /* RW   2 bits */
extern volatile uint32  AOV_RAMS_WABL;                       /* RW   6 bits */
extern volatile uint32  AOV_RAM_CFG;                         /* RW  23 bits */
extern volatile uint32  AOV_WAKEUP_MASK;                     /* RW   7 bits */

/* Declarations of read only registers */
extern volatile uint32  AOV_EVENT_STATUS;                    /* R    7 bits */
extern volatile uint32  AOV_KEYHOLE_READ_DATA;               /* R   24 bits */
extern volatile uint32  AOV_RAM_IF_STATUS1;                  /* R    9 bits */
extern volatile uint32  AOV_RAM_IF_STATUS2;                  /* R   23 bits */
extern volatile uint32  AOV_SEQUENCER_ACTIVE;                /* R    2 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_AOV */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_AOV) */



#if defined(IO_DEFS_MODULE_AUDIO_DMAC) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_AUDIO_DMAC
#define __IO_MAP_H__IO_MAP_MODULE_AUDIO_DMAC

/* -- audio_dmac -- Registers in Audio DMAC  -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  AUDIO_DMAC_CPU0_ACCESS_CTRL;         /* RW   4 bits */
extern volatile audio_dmac__mutex_lock_enum  AUDIO_DMAC_CPU0_MUTEX_LOCK;          /* RW   4 bits */
extern volatile uint32  AUDIO_DMA_DEBUG_SELECT;              /* RW  13 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_ABORT_TRANSFER;      /* RW   1 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_BUFFER_HANDLE;       /* RW  12 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_CLEAR_REQUEST_QUEUE;  /* W    1 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_CLOCKS_ENABLE;       /* RW   1 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_ENABLE;              /* RW   1 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_HALT_ON_EXCEPTION;   /* RW   3 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_REQUEST_FETCH_DISABLE;  /* RW   1 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_TRIGGER_REQUEST;     /* RW   1 bits */
extern volatile uint32  AUDIO_DMA_SOFT_RESET;                /* RW   1 bits */

/* Declarations of read only registers */
extern volatile uint32  AUDIO_DMA_DEBUG_STATUS;              /* R   32 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_REQUEST_COUNT;       /* R    8 bits */
extern volatile uint32  AUDIO_DMA_QUEUE_STATUS;              /* R   26 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_AUDIO_DMAC */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_AUDIO_DMAC) */

#if defined(IO_DEFS_MODULE_AUDIO_SYS_ADPLL) 

#ifndef __IO_MAP_H__IO_MAP_MODULE_AUDIO_SYS_ADPLL
#define __IO_MAP_H__IO_MAP_MODULE_AUDIO_SYS_ADPLL

/* -- audio_sys_adpll -- Audio subsystem clock control registers for the ADPLL -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  ADPLL_ACCESS_CTRL;                   /* RW   4 bits */
extern volatile audio_sys_adpll__mutex_lock_enum  ADPLL_MUTEX_LOCK;                    /* RW   4 bits */
extern volatile uint32  CLKGEN_CLK_SWITCH;                   /* RW  15 bits */
extern volatile uint32  NCO_PLL_COARSE_TRIM_REG;             /* RW   7 bits */
extern volatile uint32  NCO_PLL_CTL;                         /* RW  11 bits */
extern volatile uint32  NCO_PLL_DEBUG;                       /* RW   7 bits */
extern volatile uint32  NCO_PLL_EN;                          /* RW   1 bits */
extern volatile uint32  NCO_PLL_FRAC_INC_REG;                /* RW  24 bits */
extern volatile uint32  NCO_PLL_GAIN;                        /* RW   7 bits */
extern volatile uint32  NCO_PLL_INIT_ACC_VALUE_REG;          /* RW  16 bits */
extern volatile uint32  NCO_PLL_VCO_CTL;                     /* RW   2 bits */

/* Declarations of read only registers */
extern volatile uint32  CLKGEN_CLK_SWITCH_STATUS;            /* R    2 bits */
extern volatile uint32  NCO_PLL_ACC_VALUE;                   /* R   16 bits */
extern volatile uint32  NCO_PLL_CLK_VCO_COUNT_DELTA;         /* R   16 bits */
extern volatile uint32  NCO_PLL_COARSE_TRIM;                 /* R    7 bits */
extern volatile uint32  NCO_PLL_STATUS;                      /* R    4 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_AUDIO_SYS_ADPLL */
#endif /* !defined(IO_DEFS_LIMIT_MODULES) || defined(IO_DEFS_MODULE_AUDIO_SYS_ADPLL) */








#ifndef __IO_MAP_H__IO_MAP_MODULE_K32_MISC
#define __IO_MAP_H__IO_MAP_MODULE_K32_MISC

/* -- k32_misc -- Kalimba 32-bit Misc Control registers -- */

/* Declarations of read/write/mixed registers */
extern volatile uint32  ALLOW_GOTO_SHALLOW_SLEEP;            /* RW   1 bits */
extern volatile uint32  CLOCK_CONT_SHALLOW_SLEEP_RATE;       /* RW   8 bits */
extern volatile clock_divide_rate_enum  CLOCK_DIVIDE_RATE;                   /* RW   2 bits */
extern volatile uint32  CLOCK_STOP_WIND_DOWN_SEQUENCE_EN;    /* RW   1 bits */
extern volatile uint32  DISABLE_MUTEX_AND_ACCESS_IMMUNITY;   /* RW   1 bits */
extern volatile uint32  GOTO_SHALLOW_SLEEP;                  /* W    1 bits */
extern volatile uint32  PMWIN_ENABLE;                        /* RW   1 bits */
extern volatile uint32  PROC_DEEP_SLEEP_EN;                  /* RW   1 bits */

/* Declarations of read only registers */
extern volatile uint32  PROCESSOR_ID;                        /* R    1 bits */
extern volatile uint32  SHALLOW_SLEEP_STATUS;                /* R    1 bits */

#endif /* __IO_MAP_H__IO_MAP_MODULE_K32_MISC */


