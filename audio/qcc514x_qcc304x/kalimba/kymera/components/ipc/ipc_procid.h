/****************************************************************************
 * Copyright (c) 2016 - 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ipc_procid.h
 * \ingroup ipc
 *
 * Inter Processor Communication Public Header.
 * This provides the basic idea of a processor to the rest of the system. 
 */
#ifndef IPC_PROCID_H
#define IPC_PROCID_H

#include "proc/proc.h"

/**
 * \brief  Returns TRUE if the processor number passed
 * as argument is invalid or not started.
 *
 * \param[in] pid - Number of the processor to test for.
 */
extern bool ipc_invalid_processor_id(PROC_ID_NUM pid);

#endif /* IPC_PROCID_H */
