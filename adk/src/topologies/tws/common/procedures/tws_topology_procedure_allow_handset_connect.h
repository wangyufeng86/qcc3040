/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to inform connection manager that connections from the handset 
            are allowed
*/

#ifndef TWS_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H
#define TWS_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H

#include "tws_topology_procedures.h"

/*! Structure definining the parameters for the procedure to 
    allow connection from the handset.

    This should normally be used via the predefined constant structures
    PROC_ALLOW_HANDSET_CONNECT_DATA_ENABLE and PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE
*/
typedef struct
{
    /*! Whether to enable or disable handset connections */
    bool enable;
} ALLOW_HANDSET_CONNECT_PARAMS_T;


/*! Constant definition of parameters to enable handset connections */
extern const ALLOW_HANDSET_CONNECT_PARAMS_T proc_allow_handset_connect_enable;
/*! Recommended parameter for use in topology scripts when enabling 
    handset connections */
#define PROC_ALLOW_HANDSET_CONNECT_DATA_ENABLE  ((Message)&proc_allow_handset_connect_enable)

/*! Constant definition of parameters to disable handset connections */
extern const ALLOW_HANDSET_CONNECT_PARAMS_T proc_allow_handset_connect_disable;
/*! Recommended parameter for use in topology scripts when disabling 
    handset connections */
#define PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE  ((Message)&proc_allow_handset_connect_disable)

extern const procedure_fns_t proc_allow_handset_connect_fns;

#endif /* TWS_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H */
