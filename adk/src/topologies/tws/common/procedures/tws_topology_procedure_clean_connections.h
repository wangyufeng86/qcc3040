/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to ask connection manager to clean up any connections that may
            still exist.
*/

#ifndef TWS_TOPOLOGY_PROC_CLEAN_CONNECTIONS_H
#define TWS_TOPOLOGY_PROC_CLEAN_CONNECTIONS_H

#include "tws_topology_procedures.h"

/*! Value used as a fail-safe timeut in case handset ACL is not closed.

    This allows procedures/scripts to continue. */
#define TWSTOP_PROC_CLOSE_HANDSET_IND_TIMEOUT_MS    (500)

extern const procedure_fns_t proc_clean_connections_fns;

#endif /* TWS_TOPOLOGY_PROC_CLEAN_CONNECTIONS_H */
