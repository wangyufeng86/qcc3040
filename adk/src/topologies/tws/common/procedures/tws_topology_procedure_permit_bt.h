/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      
*/

#ifndef TWS_TOPOLOGY_PROC_PERMIT_BT_H
#define TWS_TOPOLOGY_PROC_PERMIT_BT_H

#include "tws_topology_procedures.h"


/*! Structure definining the parameters for the procedure to 
    permit (or block) Bluetooth activity.

    This should normally be used via the predefined constant structures
    PROC_PERMIT_BT_ENABLE and PROC_PERMIT_BT_DISABLE
*/
typedef struct
{
    /*! Whether to enable or disable Bluetooth activity */
    bool enable;
} PERMIT_BT_OPERATIONS_T;

/*! Constant definition of parameters to enable Bluetooth */
extern const PERMIT_BT_OPERATIONS_T proc_permit_bt_enable;
/*! Recommended parameter for use in topology scripts when enabling 
    Bluetooth */
#define PROC_PERMIT_BT_ENABLE  ((Message)&proc_permit_bt_enable)

/*! Constant definition of parameters to disable Bluetooth */
extern const PERMIT_BT_OPERATIONS_T proc_permit_bt_disable;
/*! Recommended parameter for use in topology scripts when disabling 
    Bluetooth */
#define PROC_PERMIT_BT_DISABLE  ((Message)&proc_permit_bt_disable)



extern const procedure_fns_t proc_permit_bt_fns;

#endif /* TWS_TOPOLOGY_PROC_PERMIT_BT_H */
