/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Procedure that allows acceptance of connections that arrive over a BREDR carrier

        This can include ACL connection as well as profiles.
*/

#ifndef TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_BREDR_H
#define TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_BREDR_H

#include "tws_topology_procedures.h"

/*! Structure definining the parameters for the procedure to 
    allow connection over bredr.

    This should normally be used via the predefined constant structures
    PROC_ALLOW_CONNECTION_OVER_BREDR_ENABLE and PROC_ALLOW_CONNECTION_OVER_BREDR_DISABLE
*/
typedef struct
{
    /*! Whether to enable or disable bredr connections if they arrive */
    bool enable;
} ALLOW_CONNECTION_OVER_BREDR_T;

/*! Constant definition of parameters to enable connections using BREDR */
extern const ALLOW_CONNECTION_OVER_BREDR_T proc_allow_connections_over_bredr_enable;
/*! Recommended parameter for use in topology scripts when enabling 
    incoming bredr connections */
#define PROC_ALLOW_CONNECTION_OVER_BREDR_ENABLE  ((Message)&proc_allow_connections_over_bredr_enable)

/*! Constant definition of parameters to disable connections using BREDR */
extern const ALLOW_CONNECTION_OVER_BREDR_T proc_allow_connections_over_bredr_disable;
/*! Recommended parameter for use in topology scripts when 
    disabling incoming bredr connections */
#define PROC_ALLOW_CONNECTION_OVER_BREDR_DISABLE  ((Message)&proc_allow_connections_over_bredr_disable)


extern const procedure_fns_t proc_allow_connection_over_bredr_fns;

#endif /* TWS_TOPOLOGY_PROC_ALLOW_CONN_OVER_BREDR_H */
