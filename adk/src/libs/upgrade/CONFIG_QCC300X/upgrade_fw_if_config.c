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
#include <partition.h>
#include <print.h>
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <sqif.h>
#include <validation.h>

#include "upgrade_ctx.h"
#include "upgrade_fw_if_priv.h"



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

/***************************************************************************
NAME
    UpgradeFWIFValidateInit

DESCRIPTION
    Initialise the context for validating the MD5 checksum of some data.
    Only one context is supported at a time.

RETURNS
*/
void UpgradeFWIFValidateInit(void)
{
    PRINT(("UPG: UpgradeFWIFValidateInit\n"));

    if (UpgradeCtxGetFW()->vctx)
    {
        PRINT(("UPG: Warning validation context already exists.\n"));
    }

    UpgradeCtxGetFW()->vctx = ValidationInitialise();
    if (UpgradeCtxGetFW()->vctx == VALIDATION_INVALID_CONTEXT)
        Panic();
}

/***************************************************************************
NAME
    UpgradeFWIFValidateUpdate

DESCRIPTION
    Update the validation context with the next set of data.

PARAMS
    buffer Pointer to the next set of data.
    len length of the data.

RETURNS
    bool TRUE if a validation context is updated successfully, FALSE otherwise.
*/
bool UpgradeFWIFValidateUpdate(uint8 *buffer, uint16 len)
{
    return ValidationUpdate(UpgradeCtxGetFW()->vctx, buffer, len);
}

/***************************************************************************
NAME
    UpgradeFWIFValidateFinalize

DESCRIPTION
    Verify the accumulated data in the validation context against
    the given signature.

PARAMS
    signature Pointer to the signature to compare against.

RETURNS
    bool TRUE if validation is successful, FALSE otherwise.
*/
bool UpgradeFWIFValidateFinalize(uint8 *signature)
{
    bool rc = ValidationFinalise(UpgradeCtxGetFW()->vctx, (const uint16 *)signature);
    PRINT(("UPG: UpgradeFWIFValidateFinalize %s\n", rc ? "PASSED" : "FAILED"));
    return rc;
}

/******************************************************************************
NAME
    UpgradeFWIFValidateExecutablePartition

DESCRIPTION
    Initiate validation of partition specified by physPartition. May or may
    not be an executable partition, we don't know until we try. Firmware will
    either start validation or report not an executable.

RETURNS
    UpgradeFWIFPartitionValidationStatus
*/
UpgradeFWIFPartitionValidationStatus UpgradeFWIFValidateExecutablePartition(uint16 physPartition)
{
    validation_init_status status;
    UpgradeFWIFPartitionValidationStatus rc = UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;

    status = ValidationInitiateExecutablePartition(physPartition);
    switch (status)
    {
        case VALIDATE_BOOTUP_VALIDATION_RUNNING:
            /* fall-thru */
        case VALIDATE_PARTITION_VALIDATION_RUNNING:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_IN_PROGRESS;
            break;

        case VALIDATE_PARTITION_VALIDATION_TRIGGERED:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_TRIGGERED;
            break;

        case VALIDATE_PARTITION_TYPE_NOT_FS:
            /* fall-thru */
        case VALIDATE_FS_NOT_EXECUTABLE:
            rc = UPGRADE_FW_IF_PARTITION_VALIDATION_SKIP;
            break;
    }

    return rc;
}

/******************************************************************************
NAME
    UpgradeFWIFValidateApplication

DESCRIPTION
    Query the firmware to check the status of the application partition
    validation.
    
    The firmware automatically starts this check during boot. However it takes
    a reasonable amount of time so may still be in progress by the time the vm
    application is started.

    Upgrade needs to know the result so that it knows if it is safe to commit
    an upgrade after a reboot.
    
    Note: There is no 'fail' error code because if the check failed the device
          would fail to boot and this function would never be called.

RETURNS
    UpgradeFWIFApplicationValidationStatus
*/
UpgradeFWIFApplicationValidationStatus UpgradeFWIFValidateApplication(void)
{
    sqif_app_validate_status status;
    UpgradeFWIFApplicationValidationStatus rc = UPGRADE_FW_IF_APPLICATION_VALIDATION_SKIP;
    
    status = SqifGetAppValidateStatus();
    PRINT(("UPG: sqif app validate status 0x%x\n", status));
    switch (status)
    {
        case SQIF_APP_VALIDATION_RUNNING:
            rc = UPGRADE_FW_IF_APPLICATION_VALIDATION_RUNNING;
            break;

        case SQIF_APP_VALIDATION_PASS:
            /* fall-thru */
        case SQIF_APP_VALIDATION_PASS_NO_APP:
            /* fall-thru */
        case SQIF_APP_VALIDATION_NOT_REQUIRED:
            rc = UPGRADE_FW_IF_APPLICATION_VALIDATION_PASS;
            break;
    }

    return rc;
}

/******************************************************************************
NAME
    UpgradeFWIFValidateAddPartitionData

DESCRIPTION
    Add the data from an existing partition to the validation context.

    This function is used when resuming an upgrade to add partition data that
    has already been downloaded and written to flash to the validation context.

    This ensures that the calculated validation hash is the same as if the
    upgrade had not been resumed at this point.

    It can add data from partitions that are either complete or partially
    complete. The only restriction is that the partition is in external flash.

PARAMS
    [in] partition Physical partition number.
    [in] skip Number of octets to skip from the beginning of the partition data.
    [out] Total number of octets added to the validation context.
    
RETURNS
    TRUE if the partition was successfully opened and read; FALSE otherwise.
*/
bool UpgradeFWIFValidateAddPartitionData(uint16 partition, uint32 skip, uint32 *read)
{
    Source src = (Source)0;
    const uint8 *src_data = NULL;
    uint32 total_read = 0;
    uint16 src_size = 0;

    PRINT(("UPG: UpgradeFWIFValidateAddPartitionData partition %u skip %lu\n", partition, skip));

    src = PartitionGetRawSource(PARTITION_SERIAL_FLASH, partition);    
    if (SourceIsValid(src))
    {
        src_data = SourceMap(src);
        if (!src_data)
        {
            PRINT(("UPG: Failed to map Source for partition %u\n", partition));
            return FALSE;
        }

        src_size = SourceSize(src);

        /* Skip the first octet(s) of the partition if required; assume they
           have been added to the validation context already. */
        if (src_size >= skip)
        {
            SourceDrop(src, skip);
            src_size = SourceSize(src);
        }

        /* Read the rest of the existing partition data and add it to 
           the validation context. */
        while (src_size)
        {
            PRINT(("UPG: src_size %d src_data %p total_read %lu\n", src_size, src_data, total_read));

            UpgradeFWIFValidateUpdate((uint8 *)src_data, src_size);
            total_read += src_size;

            SourceDrop(src, src_size);    
            src_size = SourceSize(src);
        }

        PRINT(("UPG: total_read %lu\n", total_read));

        SourceClose(src);
    }
    else
    {
        PRINT(("UPG: Failed to get Source for partition %u\n", partition));
        return FALSE;
    }

    *read = total_read;
    return TRUE;
}
