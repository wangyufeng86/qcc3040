/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_fw_if_config.c

DESCRIPTION
    Configuration dependent functions which (largely) interact with the firmware.

NOTES

*/
#include <stdlib.h>
#include <string.h>

#include <panic.h>
#include <print.h>
#include <sink.h>
#include <stream.h>

#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_fw_if_priv.h"

/******************************************************************************
NAME
    UpgradeFWIFAudioDFUExists

DESCRIPTION
    Checks if the audio dfu bit is set in the partition map.

RETURNS
    bool TRUE if the bit audio dfu is set.
*/
bool UpgradeFWIFAudioDFUExists(void)
{
    return FALSE;
}


/******************************************************************************
NAME
    UpgradeFWIFGetHeaderID

DESCRIPTION
    Get the identifier for the header of an upgrade file.

RETURNS
    const char * Pointer to the header string.
*/
const char *UpgradeFWIFGetHeaderID(void)
{
    return "APPUHDR2";
}

/***************************************************************************
NAME
    UpgradeFWIFPartitionWrite

DESCRIPTION
    Write data to an open external flash partition. Each byte of the data
    is copied to the partition in a byte by byte copy operation.

PARAMS
    handle Handle to a writeable partition.
    data Pointer to the data to write.
    len Number of bytes (not words) to write.

RETURNS
    uint16 The number of bytes written, or 0 if there was an error.
*/
uint16 UpgradeFWIFPartitionWrite(UpgradeFWIFPartitionHdl handle, uint8 *data, uint16 len)
{
    uint8 *dst;
    Sink sink = (Sink)(int)handle;

    if (!sink)
        return 0;

    /* For 1st pass don't worry about size of writes between flushes */

    dst = SinkMap(sink);
    if (!dst)
    {
        PRINT(("UPG: Failed to map sink %p\n", (void *)sink));
        return 0;
    }

    if (SinkClaim(sink, len) == 0xFFFF)
    {
        PRINT(("UPG: Failed to claim %u bytes for writing\n", len));
        return 0;
    }

    memmove(dst, data, len);

    if (!SinkFlush(sink, len))
    {
        PRINT(("UPG: Failed to flush data to partition: sink %p, len %d\n", (void *) sink, len));
        return 0;
    }

    return len;
}

void UpgradeFWIFValidateInit(void)
{
}

bool UpgradeFWIFValidateUpdate(uint8 *buffer, uint16 len)
{
    UNUSED(buffer);
    UNUSED(len);
    return TRUE;
}

bool UpgradeFWIFValidateFinalize(uint8 *signature)
{
    UNUSED(signature);
    return TRUE;
}

bool UpgradeFWIFValidateAddPartitionData(uint16 partition, uint32 skip, uint32 *read)
{
    UNUSED(partition);
    UNUSED(skip);
    UNUSED(read);
    return TRUE;
}

UpgradeFWIFPartitionValidationStatus UpgradeFWIFValidateExecutablePartition(uint16 physPartition)
{
    UNUSED(physPartition);
    return UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;
}

UpgradeFWIFApplicationValidationStatus UpgradeFWIFValidateApplication(void)
{
    return UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP;
}
