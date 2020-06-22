/****************************************************************************
 * Copyright (c) 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file proc.h
 * \ingroup proc
 *
 * Processor abstraction API.
 * This provides the basic idea of a processor to the rest of the system.
 * This header file is needed even for architectures with a single core.
 */
#ifndef PROC_H
#define PROC_H

#include "types.h"

/**
 * Enumeration for processor identity number
 */
typedef enum
{
    PROC_PROCESSOR_0 = 0,           /*!< Primary processor with exclusive
                                         access to some ressources. */
    PROC_PROCESSOR_1 = 1,           /*!< Secondary processor available
                                         on some architectures. */
#if defined(SUPPORTS_MULTI_CORE)
    PROC_PROCESSOR_BUILD = CHIP_NUM_CORES,
#else
    /* Simulator builds support a single core even
       if the target chip has more. */
    PROC_PROCESSOR_BUILD = 1,       /*!< Number of processors
                                         available in the build. */
#endif
    PROC_PROCESSOR_MAX = 2,         /*!< Maximum number of processors
                                         supported by Kymera. */
    PROC_PROCESSOR_INVALID = 0xFFFF 
} PROC_ID_NUM;

/**
 * \brief  Returns the processor ID of the processor
 * running the calling function.
 */
#if defined(SUPPORTS_MULTI_CORE)
extern PROC_ID_NUM proc_get_processor_id(void);
#else
#define proc_get_processor_id(x) PROC_PROCESSOR_0
#endif

#define PROC_PRIMARY_CONTEXT()   (proc_get_processor_id() == PROC_PROCESSOR_0)
#define PROC_SECONDARY_CONTEXT() (proc_get_processor_id() != PROC_PROCESSOR_0)

#if defined(SUPPORTS_MULTI_CORE)
#define PROC_RETURN_IF_SECONDARY(x) \
    if (PROC_SECONDARY_CONTEXT())   \
    {                               \
        return x;                   \
    }
#define PROC_BREAK_IF_SECONDARY() \
    if (PROC_SECONDARY_CONTEXT()) \
    {                             \
        break;                    \
    }
#else
#define PROC_RETURN_IF_SECONDARY(x)
#define PROC_BREAK_IF_SECONDARY()
#endif

#if defined(SUPPORTS_MULTI_CORE)
extern bool proc_single_core_running(void);
extern bool proc_multiple_cores_running(void);
#else
#define proc_single_core_running() TRUE
#define proc_multiple_cores_running() FALSE
#endif

/* Useful when deserializing messages. */
static inline bool proc_is_valid(unsigned id)
{
    return id < (unsigned) PROC_PROCESSOR_BUILD;
}

/* Returns true if the processor id matches with the current processor context */
static inline bool PROC_ON_SAME_CORE(PROC_ID_NUM id)
{
    return proc_get_processor_id() == id;
}

static inline uint8 proc_serialize(PROC_ID_NUM id)
{
    return (uint8) id;
}

static inline PROC_ID_NUM proc_deserialize(unsigned id)
{
    return (PROC_ID_NUM) id;
}

#endif /* PROC_H */
