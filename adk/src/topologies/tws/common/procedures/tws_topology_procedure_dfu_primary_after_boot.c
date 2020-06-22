/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Procedure for resuming a DFU after a scheduled restart

            Already using the DFU rules, this procedure switches to the primary
            address and allows connecting to the peer and handset.
*/

#include "tws_topology_procedure_dfu_primary_after_boot.h"

#include "tws_topology_procedure_set_address.h"
#include "tws_topology_procedure_permit_bt.h"
#include "tws_topology_procedure_allow_connection_over_le.h"
#include "tws_topology_procedure_enable_connectable_handset.h"
#include "tws_topology_procedure_allow_handset_connect.h"

    /*! Definition of script for restarted DFU as primary

        Need to set address as this may the device that "owns" the 
        secondary address. 

        No need to do anything for peer connection as the primary
        will initiate the connection */
#define DFU_PRIMARY_AFTER_BOOT_SCRIPT(ENTRY) \
    ENTRY(proc_set_address_fns, PROC_SET_ADDRESS_TYPE_DATA_PRIMARY), \
    ENTRY(proc_permit_bt_fns, PROC_PERMIT_BT_ENABLE), \
    ENTRY(proc_allow_connection_over_le_fns, PROC_ALLOW_CONNECTION_OVER_LE_ENABLE), \
    ENTRY(proc_allow_handset_connect_fns, PROC_ALLOW_HANDSET_CONNECT_DATA_ENABLE), \
    ENTRY(proc_enable_connectable_handset_fns, PROC_ENABLE_CONNECTABLE_HANDSET_DATA_ENABLE)

/* Define the dfu_role_script */
DEFINE_TOPOLOGY_SCRIPT(dfu_primary_after_boot, DFU_PRIMARY_AFTER_BOOT_SCRIPT);

