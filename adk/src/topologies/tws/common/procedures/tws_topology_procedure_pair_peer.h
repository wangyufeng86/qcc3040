/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_PAIR_PEER_H
#define TWS_TOPOLOGY_PROC_PAIR_PEER_H

#include "tws_topology_private.h"
#include "tws_topology_procedures.h"
#include "script_engine.h"

typedef enum
{
    PROC_PAIR_PEER_RESULT = TWSTOP_INTERNAL_PROCEDURE_RESULTS_MSG_BASE | tws_topology_procedure_pair_peer,
} procPeerPairMessages;

typedef struct
{
    bool success;
} PROC_PAIR_PEER_RESULT_T;

extern const procedure_fns_t proc_pair_peer_fns;

extern const procedure_script_t pair_peer_script;


#endif /* TWS_TOPOLOGY_PROC_PAIR_PEER_H */
