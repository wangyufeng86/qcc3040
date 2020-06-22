/****************************************************************************
 * Copyright (c) 2011 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  kip_mgr.h
 * \ingroup kip_mgr
 *
 */

#ifndef KIP_MGR_H
#define KIP_MGR_H

/****************************************************************************
Include Files
*/

#include "status_prim.h"
#include "adaptor/connection_id.h"
#include "ipc/ipc_kip.h"

/****************************************************************************
Type Declarations
*/
typedef struct
{
    /* For now, just data channel ID. It is assumed the initiator of the kick
     * mirrors the data channel ID such that it can be understood by 'receiver' of
     * the kick on the other processor!
     */
    uint16 data_channel_id;

} KICK_SIGNAL_PARAMS;

#if defined(SUPPORTS_MULTI_CORE)
/* Info kept for in-progress processor start/stop cmd from some command adaptor */
typedef struct
{
    CONNECTION_LINK con_id;

    bool (*callback)(CONNECTION_LINK con_id, STATUS_KYMERA status);

    /** Load information */
    /**
     * TODO: This state informations needs to be extended to
     *       support more than one secondary processors.
     */
    PROC_ID_NUM processor_id;
    IPC_NVMEM_WIN_CONFIG_TYPE memory_type; /* Boot target type (ROM, SQIF,..) */

} AUX_PROC_KIP_IPC_STATE;
#endif /* defined(SUPPORTS_MULTI_CORE) */

/****************************************************************************
Public Constant Declarations
*/
/* Base value of indirect message IDs (resps have 0x8000 added to these) */
#define KIP_INDIRECT_MSG_ID_BASE        0x4000

/* Signal IDs and max number of signals to unmask */
#define KIP_SIGNAL_ID_KICK              IPC_SIGNAL_KALIMBA_0
#define KIP_SIGNAL_ID_REVERSE_KICK      IPC_SIGNAL_KALIMBA_1
#define KIP_SIGNAL_ID_SCO_PARAMS        IPC_SIGNAL_KALIMBA_2
#define KIP_SIGNAL_ID_DYN_CONFIG        IPC_SIGNAL_KALIMBA_3

#define KIP_MAX_NUM_SIGNALS             4

/****************************************************************************
Public Variable Definitions
*/
#if defined(SUPPORTS_MULTI_CORE)
extern AUX_PROC_KIP_IPC_STATE aux_proc_kip_ipc_state;
#endif

/****************************************************************************
Public Function Declarations
*/
/* initilise KIP */
void kip_init(void);

/* Init the KIP Mgr */
bool kip_mgr_init(void);

/* Shutdown KIP Mgr - just final clean-up steps, optionally called at system shutdown only */
void kip_mgr_shutdown(void);

/* Get the message channel ID for a certain destination processor, returns 0 for fail */
uint16 get_msg_chn_id_for_processor(PROC_ID_NUM processor_id);

/* Get maximum valid processor ID depending on what image and where is it called */
PROC_ID_NUM kip_get_max_processor_id(void);

/* Setup message and non-static signal callbacks, keep message channel ID in housekeeping table */
IPC_STATUS kip_setup_comms_to_processor(PROC_ID_NUM processor_id);

/* Register KIP mgr communications for current Kymera FW version */
IPC_STATUS kip_register_comms(void);

/* Add a kip signal pointer for a given data channel id */
bool kip_add_signal_ptr(uint16 data_channel_id, uint16 signal_id);

/* Remove a kip signal pointer for a given data channel id */
bool kip_remove_signal_ptr(uint16 data_channel_id, uint16 signal_id);

/* Find a kip signal pointer for a given data channel id */
uint16 *kip_get_signal_ptr(uint16 data_channel_id, uint16 signal_id);

/* Determine whether an auxiliary processor is up-and-running or not (proc_id 1..3) */
bool kip_aux_processor_has_started(PROC_ID_NUM proc_id);

#if defined(SUPPORTS_MULTI_CORE)

/**
 * \brief  Disables secondary processor after boot but before it has been started.
 *
 * \param  processor_id Processor ID
 * \return              TRUE if successful and FALSE in case of failure.
 */
bool kip_aux_processor_disable(PROC_ID_NUM processor_id);

/* Start secondary processor. Return readily usable STATUS_OK or STATUS_CMD_FAILED toward the command
 * adaptor (it translates IPC status) */
unsigned kip_aux_processor_start(CONNECTION_LINK con_id,
                                 PROC_ID_NUM processor_id,
                                 bool (*callback)(CONNECTION_LINK con_id,
                                                  STATUS_KYMERA status));
/* Stop secondary processor. Return readily usable STATUS_OK or STATUS_CMD_FAILED toward the command
 * adaptor (it translates IPC status) */
unsigned kip_aux_processor_stop(CONNECTION_LINK con_id,
                                PROC_ID_NUM processor_id,
                                bool (*callback)(CONNECTION_LINK con_id,
                                                 STATUS_KYMERA status));

/* Check whether the auxiliary processors are alive. */
bool kip_aux_processor_not_hanging(void);
#else
/* If there is only one processor then it cannot be hanging. */
#define kip_aux_processor_not_hanging() TRUE
#endif /* defined(SUPPORTS_MULTI_CORE) */

#endif /* KIP_MGR_H */
