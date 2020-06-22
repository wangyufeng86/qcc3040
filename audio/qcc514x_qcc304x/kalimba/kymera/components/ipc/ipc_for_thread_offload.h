/*****************************************************************************
*
* Copyright (c) 2018 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/** \file ipc_for_thread_offload.h
 *
 *  Semi-public API for thread offload feature only.
 *
 */

#ifndef IPC_FOR_THREAD_OFFLOAD_H
#define IPC_FOR_THREAD_OFFLOAD_H

/****************************************************************************
Include Files
*/

/****************************************************************************
Public Type Declarations
*/


/****************************************************************************
Public Constant and macros
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/

extern void raise_p0_2_p1_message_int(void);

extern void raise_p1_2_p0_message_int(void);

extern void raise_p0_2_p1_signal_int(void);

extern void raise_p1_2_p0_signal_int(void);

extern bool ipc_audio_thread_start(void);

#endif /* IPC_FOR_THREAD_OFFLOAD_H */
