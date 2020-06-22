/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if.c

DESCRIPTION
    Implementation of functions which (largely) interact with the firmware.

NOTES

*/

#include <stdlib.h>
#include <string.h>
#include "upgrade_partitions.h"


#include <byte_utils.h>
#include <csrtypes.h>
#include <panic.h>
#include <partition.h>
#include <print.h>
#include <sink.h>
#include <stream.h>

#include "upgrade_ctx.h"

#include "upgrade_psstore.h"
#include "upgrade_partitions.h"
#include "upgrade_private.h"
#include "upgrade_msg_host.h"

#include "upgrade_fw_if.h"
#include "upgrade_fw_if_priv.h"

/******************************************************************************
NAME
    UpgradeFWIFInit

DESCRIPTION
    Initialise the context for the Upgrade FW IF.

*/
void UpgradeFWIFInit(void)
{
    UpgradeFWIFCtx *fwctx = UpgradeCtxGetFW();

    memset(fwctx, 0, sizeof(*fwctx));
}

/******************************************************************************
NAME
    UpgradeFWIFGetPartitionID

DESCRIPTION
    Get the identifier for a partition header within an upgrade file.

RETURNS
    const char * Pointer to the partition header string.
*/
const char *UpgradeFWIFGetPartitionID(void)
{
    return "PARTDATA";
}

/******************************************************************************
NAME
    UpgradeFWIFGetFooterID

DESCRIPTION
    Get the identifier for the footer of an upgrade file.

RETURNS
    const char * Pointer to the footer string.
*/
const char *UpgradeFWIFGetFooterID(void)
{
    return "APPUPFTR";
}

/******************************************************************************
NAME
    UpgradeFWIFGetDeviceVariant

DESCRIPTION
    Get the identifier for the current device variant.

RETURNS
    const char * Pointer to the device variant string.
*/
const char *UpgradeFWIFGetDeviceVariant(void)
{
    return ( const char * )UpgradeCtxGet()->dev_variant;
}

/******************************************************************************
NAME
    UpgradeFWIFGetAppVersion

DESCRIPTION
    Get the current (running) app version.

RETURNS
    uint16 The running app version.
*/
uint16 UpgradeFWIFGetAppVersion(void)
{
    return 2;
}

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartition

DESCRIPTION
    Given a logical partition number return the physical partition number into 
    which we can write data. This includes the SQUIF selector, use 
    UPGRADE_PARTITION_PHYSICAL_PARTITION to extract partition ID.

RETURNS
    uint16 Number of the physical partition available for write.
*/
uint16 UpgradeFWIFGetPhysPartition(uint16 logicPartition)
{
    return UpgradePartitionsPhysicalPartition(logicPartition,UpgradePartitionUpgradable);
}

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartitionNum

DESCRIPTION
    Work out how many partitions there are in the serial flash.

    Just repeatedly ask the firmware for partition info, incremeting a counter
    until the firmware tells us it failed.

RETURNS
    uint16 Number of partitions in the serial flash.
*/
uint16 UpgradeFWIFGetPhysPartitionNum(void)
{
    uint32 value = 0;
    uint16 partition_count = 0;

    while (PartitionGetInfo(PARTITION_SERIAL_FLASH, partition_count,
                            PARTITION_INFO_TYPE, &value))
    {
        partition_count++;
    }

    return partition_count;
}

/******************************************************************************
NAME
    UpgradeFWIFGetPhysPartitionSize

DESCRIPTION
    Find out the size of a specified partition in bytes.

RETURNS
    uint32 Size of physPartition in bytes.
*/
uint32 UpgradeFWIFGetPhysPartitionSize(uint16 physPartition)
{
    uint32 size = 0;

    if (PartitionGetInfo(PARTITION_SERIAL_FLASH, physPartition,
                     PARTITION_INFO_SIZE, &size))
    {
        return (size * 2);
    }

    return 0;
}

/******************************************************************************
NAME
    UpgradeFWIFValidPartitionType

DESCRIPTION
    Determine if the partition type (in the flash) is a valid type that the
    upgrade library is expecting (and knows how to handle).

RETURNS
    bool TRUE if type is valid, FALSE if type is invalid
*/
bool UpgradeFWIFValidPartitionType(UpgradeFWIFPartitionType type, uint16 physPartition)
{
    uint32 ptn_type;
    static const partition_type valid_types[UPGRADE_FW_IF_PARTITION_TYPE_NUM] =
    {
        PARTITION_TYPE_FILESYSTEM, /* UPGRADE_FW_IF_PARTITION_TYPE_EXE */
        PARTITION_TYPE_RAW_SERIAL, /* UPGRADE_FW_IF_PARTITION_TYPE_DFU */
        PARTITION_TYPE_FILESYSTEM, /* UPGRADE_FW_IF_PARTITION_TYPE_CONFIG */
        PARTITION_TYPE_FILESYSTEM, /* UPGRADE_FW_IF_PARTITION_TYPE_DATA */
        PARTITION_TYPE_RAW_SERIAL  /* UPGRADE_FW_IF_PARTITION_TYPE_DATA_RAW_SERIAL */
    };

    if ((type < UPGRADE_FW_IF_PARTITION_TYPE_EXE) ||
        (type >= UPGRADE_FW_IF_PARTITION_TYPE_NUM))
    {
        PRINT(("UPG: unknown partition header type %u\n", type));
        return FALSE;
    }

    /* Add check that header partition type is compatible with type in partition info */
    if (!PartitionGetInfo(PARTITION_SERIAL_FLASH, physPartition, PARTITION_INFO_TYPE, &ptn_type))
    {
        PRINT(("UPG: Failed to get partition info of partition %u\n", physPartition));
        return FALSE;
    }

    if (valid_types[type] != (partition_type)ptn_type)
    {
        PRINT(("UPG: valid partition type [%u] %u does not match actual type %lu\n",
            type, valid_types[type], ptn_type));
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionOpen

DESCRIPTION
    Open a write only handle to a physical partition on the external flash.
    For initial testing, the CRC check on the partition is also disabled.

PARAMS
    logic Logic
    physPartition Physical partition number in external flash.
    firstWord First word of partition data.

RETURNS
    UpgradeFWIFPartitionHdl A valid handle or NULL if the open failed.
*/
UpgradeFWIFPartitionHdl UpgradeFWIFPartitionOpen(uint16 logic,uint16 physPartition, uint16 firstWord)
{
    Sink sink;

    PRINT(("UPG: Opening partition %u for resume\n", physPartition));

    sink = StreamPartitionResumeSink(PARTITION_SERIAL_FLASH, physPartition, firstWord);
    if (!sink)
    {
        PRINT(("UPG: Failed to open raw partition %u for resume\n", physPartition));
        return (UpgradeFWIFPartitionHdl)(int)NULL;
    }

    /* Configure the sink such that messages are not received */
    SinkConfigure(sink, VM_SINK_MESSAGES, VM_MESSAGES_NONE);

    /* Disable the crc check */
    PartitionSetMessageDigest(sink, PARTITION_MESSAGE_DIGEST_SKIP, NULL, 0);

    /* This can't be used in link loss scenario until FW will fix its bug */
#if 0
    if (!PartitionSetMessageDigest(sink, PARTITION_MESSAGE_DIGEST_SKIP, NULL, 0))
    {
        PRINT(("UPG: Failed to set message digest on partition %u 0x%x\n", physPartition, (uint16)sink));
        SinkClose(sink);
        return (UpgradeFWIFPartitionHdl)(int)NULL;
    }
#endif

    UpgradePartitionsMarkUpgrading(logic);

    PRINT(("Early offset is %ld\n",PartitionSinkGetPosition(sink)));

    UpgradeCtxGetFW()->partitionNum = physPartition;

    return (UpgradeFWIFPartitionHdl)(int)sink;
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionClose

DESCRIPTION
    Close a handle to an external flash partition.

PARAMS
    handle Handle to a writeable partition.

RETURNS
    bool TRUE if a valid handle is given, FALSE otherwise.
*/
UpgradeHostErrorCode UpgradeFWIFPartitionClose(UpgradeFWIFPartitionHdl handle)
{

    Sink sink = (Sink)(int)handle;

    PRINT(("UPG: Closing partition\n"));

    if (!sink)
        return FALSE;

    if (!UpgradePSSpaceForCriticalOperations())
    {
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED_PS_SPACE;
    }

    if (!SinkClose(sink))
    {
        PRINT(("Unable to close SINK\n"));
        return UPGRADE_HOST_ERROR_PARTITION_CLOSE_FAILED;
    }
    /* last_closed_partition == partition_num + 1
     * so value 0 means no partitions were closed
     */
    UpgradeCtxGetPSKeys()->last_closed_partition = UpgradeCtxGetFW()->partitionNum + 1;
    UpgradeSavePSKeys();
    PRINT(("P&R: last_closed_partition is %d\n", UpgradeCtxGetPSKeys()->last_closed_partition));

    return UPGRADE_HOST_SUCCESS;
}

/******************************************************************************
NAME
    UpgradeFWIFPartitionGetOffset
*/
uint32 UpgradeFWIFPartitionGetOffset(UpgradeFWIFPartitionHdl handle)
{
    Sink sink = (Sink)(int)handle;
    uint32 offset = PartitionSinkGetPosition(sink);

    PRINT(("UPG: Nonzero partition offset %ld\n", offset));

    return offset;
}
