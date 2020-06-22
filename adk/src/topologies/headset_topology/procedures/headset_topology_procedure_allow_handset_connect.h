 /*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure to inform connection manager that connections from the handset 
            are allowed
*/
#ifndef HEADSET_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H
#define HEADSET_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H

#include "headset_topology_procedures.h"

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

extern const procedure_fns_t hs_proc_allow_handset_connect_fns;

/*! Constant definition of parameters to enable handset connections */
extern const ALLOW_HANDSET_CONNECT_PARAMS_T hs_proc_allow_handset_connect_enable;
/*! Recommended parameter for use in topology scripts when enabling 
    handset connections */
#define PROC_ALLOW_HANDSET_CONNECT_DATA_ENABLE  ((Message)&hs_proc_allow_handset_connect_enable)

/*! Constant definition of parameters to disable handset connections */
extern const ALLOW_HANDSET_CONNECT_PARAMS_T hs_proc_allow_handset_connect_disable;
/*! Recommended parameter for use in topology scripts when disabling 
    handset connections */
#define PROC_ALLOW_HANDSET_CONNECT_DATA_DISABLE  ((Message)&hs_proc_allow_handset_connect_disable)


#endif /* HEADSET_TOPOLOGY_PROC_ALLOW_HANDSET_CONNECT_H */
