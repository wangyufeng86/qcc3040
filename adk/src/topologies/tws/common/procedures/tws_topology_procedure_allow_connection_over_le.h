/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_LE_H
#define TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_LE_H

#include "tws_topology_procedures.h"

/*! Structure definining the parameters for the procedure to 
    allow connection over LE.

    This should normally be used via the predefined constant structures
    PROC_ALLOW_CONNECTION_OVER_LE_ENABLE and PROC_ALLOW_CONNECTION_OVER_LE_DISABLE
*/
typedef struct
{
    /*! Whether to enable or disable LE connections if they arrive */
    bool enable;
} ALLOW_CONNECTION_OVER_LE_T;

/*! Constant definition of parameters to enable connections using LE */
extern const ALLOW_CONNECTION_OVER_LE_T proc_allow_connections_over_le_enable;
/*! Recommended parameter for use in topology scripts when enabling 
    incoming LE connections */
#define PROC_ALLOW_CONNECTION_OVER_LE_ENABLE  ((Message)&proc_allow_connections_over_le_enable)

/*! Constant definition of parameters to enable connections using LE */
extern const ALLOW_CONNECTION_OVER_LE_T proc_allow_connections_over_le_disable;
/*! Recommended parameter for use in topology scripts when disabling 
    incoming LE connections */
#define PROC_ALLOW_CONNECTION_OVER_LE_DISABLE  ((Message)&proc_allow_connections_over_le_disable)


extern const procedure_fns_t proc_allow_connection_over_le_fns;

#endif /* TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_LE_H */
