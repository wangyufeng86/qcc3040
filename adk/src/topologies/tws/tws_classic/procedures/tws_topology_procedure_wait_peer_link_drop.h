/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Interface to procedure for a Secondary to wait for peer link to drop.
*/

#ifndef TWS_TOPOLOGY_PROC_WAIT_PEER_LINK_DROP_H
#define TWS_TOPOLOGY_PROC_WAIT_PEER_LINK_DROP_H

#include "tws_topology.h"
#include "tws_topology_procedures.h"

extern const procedure_fns_t proc_wait_peer_link_drop_fns;

/*! Structure holding parameters for the wait_peer_link_drop procedure */
typedef struct
{
        /*! Timeout, in ms, to use in the procedure. 
            0 is not valid.
            If the link is not dropped within this time, the procedure 
            will complete with a timeout code.
         */
    uint32 timeout_ms;
} WAIT_PEER_LINK_DROP_TYPE_T;

/*! Timeout used in the \ref wait_peer_link_drop_default_timeout set of parameters below.
*/
#define TWSTOP_PROC_WAIT_PEER_LINK_DROP_DEFAULT_TIMEOUT_MS    (1000)


/*! Variable with the default timeout for the wait_peer_link_drop procedure */
extern const WAIT_PEER_LINK_DROP_TYPE_T wait_peer_link_drop_default_timeout;
/*! Define that can be used in a script to set parameters for the wait_peer_link_drop procedure */
#define PROC_PEER_LINK_DROP_DATA_DEFAULT_TIMEOUT    ((Message)&wait_peer_link_drop_default_timeout)


#endif /* TWS_TOPOLOGY_PROC_WAIT_PEER_LINK_DROP_H */
