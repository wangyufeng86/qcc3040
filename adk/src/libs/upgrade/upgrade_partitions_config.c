/****************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partitions_config.c

DESCRIPTION

    Implementation of partition management, config dependant parts
    of for the upgrade_partitions.c module.


IMPLEMENTATION NOTES

See the the upgrade_partitions.c module.

*/
    
#include <ps.h>
#include <print.h>
#include <partition.h>
#include <sink.h>
#include <stream.h>
#include "upgrade_private.h"
#include "upgrade_ctx.h"
#include "upgrade_partitions.h"

/****************************************************************************
NAME
    UpgradePartitionsEraseAllManaged  -  Erase partitions

DESCRIPTION
    Loop through all the logical partitions and erase the partitions that we
    should be keeping erased.
        double banked partitions
        (transient) single banked partitions, such as DFU
*/
UpgradePartitionsState UpgradePartitionsEraseAllManaged(void)
{
    bool    successful=(UpgradeCtxGet()->upgradeNumLogicalPartitions>0);

    uint16  logical;

    for (logical = 0;logical < UpgradeCtxGet()->upgradeNumLogicalPartitions;logical++)
    {
        UPGRADE_LOGICAL_PARTITION_ARRANGEMENT banking = UpgradeCtxGet()->upgradeLogicalPartitions[logical].banking;

        switch (banking)
        {
        case UPGRADE_LOGICAL_BANKING_SINGLE_ERASE_TO_UPDATE:
            /* These banks are in use and will (eventually) need to be 
             * erased before an upgrade */
            break;
            
        case UPGRADE_LOGICAL_BANKING_SINGLE_KEEP_ERASED:
        case UPGRADE_LOGICAL_BANKING_DOUBLE_UNMOUNTED:
        case UPGRADE_LOGICAL_BANKING_DOUBLE_MOUNTED:
            {
                uint16 physical = UpgradePartitionsPhysicalPartition(logical,UpgradePartitionUpgradable);
                Sink sink;

                physical = UPGRADE_PARTITION_PHYSICAL_PARTITION(physical);
                sink = StreamPartitionOverwriteSink(PARTITION_SERIAL_FLASH, physical);
                if (sink)
                {
                    SinkClose(sink);
                }
                else
                {
                    successful = FALSE;
                }
            }
            break;
            
        default:
            break;
        }
    }

    if (successful)
    {
        return UPGRADE_PARTITIONS_ERASED;
    }
    else
    {
        return UPGRADE_PARTITIONS_ERROR;
    }
  
}
