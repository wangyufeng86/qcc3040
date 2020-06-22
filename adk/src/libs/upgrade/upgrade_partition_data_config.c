/****************************************************************************
Copyright (c) 2014 - 2018 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_partition_data.c

DESCRIPTION
    Upgrade file processing state machine.
    It is parsing and validating headers.
    All received data are passed to MD5 validation.
    Partition data are written to SQIF.

NOTES

*/

#include <string.h>
#include <stdlib.h>

#include <byte_utils.h>
#include <panic.h>
#include <print.h>

#include "upgrade_partition_data.h"
#include "upgrade_partition_data_priv.h"
#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_partitions.h"

#define EXPECTED_SIGNATURE_SIZE 128
#define FIRST_WORD_SIZE 2

uint8 first_word_size = FIRST_WORD_SIZE;

bool UpgradePartitionDataInit(bool *waitForEraseComplete)
{
    UpgradePartitionDataCtx *ctx;
    *waitForEraseComplete = FALSE;

    ctx = UpgradeCtxGetPartitionData();
    if (!ctx)
    {
        ctx = malloc(sizeof(*ctx));
        if (!ctx)
        {
            PRINT(("\n"));
            return FALSE;
        }
        UpgradeCtxSetPartitionData(ctx);
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE);

    UpgradeFWIFInit();
    UpgradeFWIFValidateInit();
    
    return TRUE;
}



/****************************************************************************
NAME
    UpgradePartitionDataHandleHeaderState  -  Parser for the main header.

DESCRIPTION
    Validates content of the main header.

RETURNS
    Upgrade library error code.

NOTES
    Currently when main header size will grow beyond block size it won't work.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleHeaderState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    UpgradeHostErrorCode rc = UPGRADE_HOST_SUCCESS;
    upgrade_version newVersion;
    upgrade_version currVersion;
    uint16 newPSVersion;
    uint16 currPSVersion;
    uint16 compatibleVersions;
    uint8 *ptr; /* Pointer into variable length portion of header */
    unsigned i;

    if(!reqComplete)
    {
        /* TODO: Handle a case when header is bigger than blockSize.
         * Currently such situation will cause this error.
         */
        return UPGRADE_HOST_ERROR_INTERNAL_ERROR_2;
    }

    if((strlen(UpgradeFWIFGetDeviceVariant()) > 0) &&
       (strncmp((char *)data, UpgradeFWIFGetDeviceVariant(), ID_FIELD_SIZE)))
    {
        return UPGRADE_HOST_ERROR_WRONG_VARIANT;
    }

    newVersion.major = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE]);
    newVersion.minor = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE+2]);
    compatibleVersions = ByteUtilsGet2BytesFromStream(&data[ID_FIELD_SIZE+4]);
    currVersion.major = UpgradeCtxGetPSKeys()->version.major;
    currVersion.minor = UpgradeCtxGetPSKeys()->version.minor;

    PRINT(("Current Version: %d.%d [%d]\n",currVersion.major,currVersion.minor,UpgradeCtxGetPSKeys()->config_version));
    PRINT(("Number compat %d\n",compatibleVersions));

    ptr = &data[ID_FIELD_SIZE+6];
    for (i = 0;i < compatibleVersions;i++)
    {
        upgrade_version version;
        version.major = ByteUtilsGet2BytesFromStream(ptr);
        version.minor = ByteUtilsGet2BytesFromStream(ptr+2);
        PRINT(("Test version: %d.%d\n",version.major,version.minor));
        if (    (version.major == currVersion.major)
            && (    (version.minor == currVersion.minor)
                 || (version.minor == 0xFFFFu)))
        {
            /* Compatible */
            break;
        }
        ptr += 4;
    }

    /* We failed to find a compatibility match */
    if (i == compatibleVersions)
    {
        return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
    }

    ptr = &data[ID_FIELD_SIZE+6+4*compatibleVersions];
    currPSVersion = UpgradeCtxGetPSKeys()->config_version;
    newPSVersion = ByteUtilsGet2BytesFromStream(ptr);
    PRINT(("PS: Curr %d, New %d\n",currPSVersion,newPSVersion));
    if (currPSVersion != newPSVersion)
    {
        compatibleVersions = ByteUtilsGet2BytesFromStream(&ptr[2]);
        ptr+=4;
        PRINT(("Number of compatible PS %d\n",compatibleVersions));
        for (i = 0;i < compatibleVersions;i++)
        {
            uint16 version;
            version = ByteUtilsGet2BytesFromStream(ptr);
            PRINT(("Test PS compatibility %d\n",version));
            if (version == currPSVersion)
            {
                /* Compatible */
                break;
            }
            ptr += 2;
        }
        if (i == compatibleVersions)
        {
            return UPGRADE_HOST_WARN_APP_CONFIG_VERSION_INCOMPATIBLE;
        }
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_UPGRADE_HEADER;
    }

    /* Store the in-progress upgrade version */
    UpgradeCtxGetPSKeys()->version_in_progress.major = newVersion.major;
    UpgradeCtxGetPSKeys()->version_in_progress.minor = newVersion.minor;
    UpgradeCtxGetPSKeys()->config_version_in_progress = newPSVersion;

    PRINT(("Saving versions %d.%d [%d]\n",newVersion.major,newVersion.minor,newPSVersion));

    /* At this point, partitions aren't actually dirty - but want to minimise PSKEYS
     * @todo: Need to check this variable before starting an upgrade
     */
    UpgradeCtxGetPSKeys()->state_of_partitions = UPGRADE_PARTITIONS_UPGRADING;

    /*!
        @todo Need to minimise the number of times that we write to the PS
              so this may not be the optimal place. It will do for now.
    */
    UpgradeSavePSKeys();

    UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE);
    ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

    return rc;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleHeaderState  -  Parser for the partition data header.

DESCRIPTION
    Validates content of the partition data header.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataHeaderState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint16 physPartNum, logicPartNum, firstWord;
    UpgradeFWIFPartitionType partType;
    uint32 total_len;
    UNUSED(reqComplete);

    if(len < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
    {
        return UPGRADE_HOST_ERROR_BAD_LENGTH_DATAHDR_RESUME;
    }

    logicPartNum = ByteUtilsGet2BytesFromStream(&data[2]);
    PRINT(("PART_DATA: logic part %u\n", logicPartNum));

    physPartNum = UpgradeFWIFGetPhysPartition(logicPartNum);
    PRINT(("PART_DATA: phys part %u\n", physPartNum));
    physPartNum = UPGRADE_PARTITION_PHYSICAL_PARTITION(physPartNum);
    PRINT(("PART_DATA: phys part %u\n", physPartNum));

    firstWord = data[PARTITION_SECOND_HEADER_SIZE + 1];
    firstWord |= (uint16)data[PARTITION_SECOND_HEADER_SIZE] << 8;
    PRINT(("PART_DATA: first word is 0x%x\n", firstWord));

    if(physPartNum >= UpgradeFWIFGetPhysPartitionNum())
    {
        return UPGRADE_HOST_ERROR_WRONG_PARTITION_NUMBER;
    }

    PRINT(("last closed %d, open next %d\n", UpgradeCtxGetPSKeys()->last_closed_partition, ctx->openNextPartition));

    if(UpgradeCtxGetPSKeys()->last_closed_partition && !ctx->openNextPartition)
    {
        PRINT(("not going to open this partition but we will open next %d\n", (UpgradeCtxGetPSKeys()->last_closed_partition == physPartNum + 1)));
        if(UpgradeCtxGetPSKeys()->last_closed_partition == physPartNum + 1)
        {
            ctx->openNextPartition = TRUE;
        }
        
        /* The upgrade is being resumed so the completed partition header data
           needs to be added to the validation calculation. */
        if(!UpgradeFWIFValidateUpdate(data, len))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1;
        }
        
        /* The upgrade is being resumed so the completed partition data needs
           to be added to the validation calculation.

           Note: skip the first word of the partition - this is already
                 included in the call to UpgradeFWIFValidateUpdate above. */
        if (!UpgradeFWIFValidateAddPartitionData(physPartNum, FIRST_WORD_SIZE, &total_len))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA;
        }
    
        PRINT(("PART_DATA: total_len %lu part_len %lu\n", total_len, ctx->partitionLength));

        ctx->nextOffset = ctx->partitionLength - FIRST_WORD_SIZE;
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

        return UPGRADE_HOST_SUCCESS;
    }
    else
    {
        ctx->openNextPartition = FALSE;
    }


    partType = ByteUtilsGet2BytesFromStream(data);
    if(!UpgradeFWIFValidPartitionType(partType, physPartNum))
    {
        return UPGRADE_HOST_ERROR_PARTITION_TYPE_NOT_MATCHING;
    }

    if (partType == UPGRADE_FW_IF_PARTITION_TYPE_DFU)
    {
        if (UpgradePartitionDataIsDfuUpdate() &&
                (UpgradePartitionDataGetDfuPartition() != physPartNum))
        {
            return UPGRADE_HOST_ERROR_PARTITION_TYPE_TWO_DFU;
        }

        PRINT(("PART_DATA: DFU detected in partition %d\n", physPartNum));

        /* DFU partition number will be stored in non volatile space upon partition close */
        UpgradeCtxGetPSKeys()->dfu_partition_num = physPartNum + 1;
    }

    if(ctx->partitionLength > UpgradeFWIFGetPhysPartitionSize(physPartNum))
    {
        return UPGRADE_HOST_ERROR_PARTITION_SIZE_MISMATCH;
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER1;
    }

    ctx->partitionHdl = UpgradeFWIFPartitionOpen(logicPartNum, physPartNum, firstWord);
    if(!ctx->partitionHdl)
    {
        return UPGRADE_HOST_ERROR_PARTITION_OPEN_FAILED;
    }

    ctx->nextOffset = UpgradeFWIFPartitionGetOffset(ctx->partitionHdl);

    PRINT(("PART_DATA: partition length is %ld and offset is: %ld\n", ctx->partitionLength, ctx->nextOffset));

    if(ctx->nextOffset == 0)
    {
        if(!UpgradeFWIFValidateUpdate(&data[PARTITION_SECOND_HEADER_SIZE], FIRST_WORD_SIZE))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_HEADER2;
        }

        if(FIRST_WORD_SIZE != UpgradeFWIFPartitionWrite(ctx->partitionHdl, &data[PARTITION_SECOND_HEADER_SIZE], FIRST_WORD_SIZE))
        {
            return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_HEADER;
        }
    }
    if(ctx->nextOffset < ctx->partitionLength)
    {

        if(ctx->nextOffset >= FIRST_WORD_SIZE)
        {
            ctx->nextOffset -= FIRST_WORD_SIZE;
        }

        /* The upgrade is being resumed so the incomplete partition data needs
           to be added to the validation calculation.

           Note: skip the first word of the partition - this is already
                 been included in the call to UpgradeFWIFValidateUpdate above. */
        if (!UpgradeFWIFValidateAddPartitionData(physPartNum, FIRST_WORD_SIZE, &total_len))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA;
        }

        UpgradePartitionDataRequestData(ctx->partitionLength - ctx->nextOffset - FIRST_WORD_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA;
    }
    else if(ctx->nextOffset == ctx->partitionLength)
    {
        UpgradeHostErrorCode closeStatus;

        /* The upgrade is being resumed so the incomplete partition data needs
           to be added to the validation calculation.

           Note: skip the first word of the partition - this is already
                 been included in the call to UpgradeFWIFValidateUpdate above. */
        if (!UpgradeFWIFValidateAddPartitionData(physPartNum, FIRST_WORD_SIZE, &total_len))
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA;
        }

        /* A case when all data are in but partition is not yet closed */
        closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        if(UPGRADE_HOST_SUCCESS != closeStatus)
        {
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;

        ctx->nextOffset -= FIRST_WORD_SIZE;
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;
    }
    else
    {
        /* It is considered bad when offset is bigger than partition size */

        return UPGRADE_HOST_ERROR_INTERNAL_ERROR_3;
    }

    PRINT(("PART_DATA: partition length is %ld and offset is: %ld bigreq is %ld, nextReq is %ld\n", ctx->partitionLength, ctx->nextOffset, ctx->bigReqSize, ctx->nextReqSize));


    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleDataState  -  Partition data handling.

DESCRIPTION
    Writes data to a SQIF and sends it the MD5 validation.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleDataState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_PARTITION_DATA;
    }

    if(len != UpgradeFWIFPartitionWrite(ctx->partitionHdl, data, len))
    {
        return UPGRADE_HOST_ERROR_PARTITION_WRITE_FAILED_DATA;
    }

    if(reqComplete)
    {
        UpgradeHostErrorCode closeStatus;
        UpgradePartitionDataRequestData(HEADER_FIRST_PART_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_GENERIC_1ST_PART;

        closeStatus = UpgradeFWIFPartitionClose(ctx->partitionHdl);
        if(UPGRADE_HOST_SUCCESS != closeStatus)
        {
            return closeStatus;
        }

        ctx->openNextPartition = TRUE;
    }

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataHandleGeneric1stPartState  -  Parser for ID & length part of a header.

DESCRIPTION
    Parses common beginning of any header and determines which header it is.
    All headers have the same first two fields which are 'header id' and
    length.

RETURNS
    Upgrade library error code.
*/
UpgradeHostErrorCode UpgradePartitionDataHandleGeneric1stPartState(uint8 *data, uint16 len, bool reqComplete)
{
    UpgradePartitionDataCtx *ctx = UpgradeCtxGetPartitionData();
    uint32 length;
    UNUSED(reqComplete);

    if(len < HEADER_FIRST_PART_SIZE)
    {
        return UPGRADE_HOST_ERROR_BAD_LENGTH_TOO_SHORT;
    }

    length = ByteUtilsGet4BytesFromStream(&data[ID_FIELD_SIZE]);

    PRINT(("1st part header id %c%c%c%c%c%c%c%c len 0x%lx, 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3],
            data[4], data[5], data[6], data[7], length, data[8], data[9], data[10], data[11]));

    if(0 == strncmp((char *)data, UpgradeFWIFGetHeaderID(), ID_FIELD_SIZE))
    {
        if(length < UPGRADE_HEADER_MIN_SECOND_PART_SIZE)
        {
            return UPGRADE_HOST_ERROR_BAD_LENGTH_UPGRADE_HEADER;
        }

        UpgradePartitionDataRequestData(length);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_HEADER;
    }
    else if(0 == strncmp((char *)data, UpgradeFWIFGetPartitionID(), ID_FIELD_SIZE))
    {
        if(length < PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE)
        {
            return UPGRADE_HOST_ERROR_BAD_LENGTH_PARTITION_HEADER;
        }
        UpgradePartitionDataRequestData(PARTITION_SECOND_HEADER_SIZE + FIRST_WORD_SIZE);
        ctx->state = UPGRADE_PARTITION_DATA_STATE_DATA_HEADER;
        ctx->partitionLength = length - PARTITION_SECOND_HEADER_SIZE;
    }
    else if(0 == strncmp((char *)data, UpgradeFWIFGetFooterID(), ID_FIELD_SIZE))
    {
        if(length != EXPECTED_SIGNATURE_SIZE)
        {
            /* The length of signature must match expected length.
             * Otherwise OEM signature checking could be omitted by just
             * setting length to 0 and not sending signature.
             */
            return UPGRADE_HOST_ERROR_BAD_LENGTH_SIGNATURE;
        }

        UpgradePartitionDataRequestData(length);

        ctx->signature = malloc(length/2 + length%2);
        if (!ctx->signature)
        {
            return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_MEMORY;
        }

        ctx->state = UPGRADE_PARTITION_DATA_STATE_FOOTER;
    }
    else
    {
        return UPGRADE_HOST_ERROR_UNKNOWN_ID;
    }

    if(!UpgradeFWIFValidateUpdate(data, len))
    {
        return UPGRADE_HOST_ERROR_OEM_VALIDATION_FAILED_HEADERS;
    }

    return UPGRADE_HOST_SUCCESS;
}

/****************************************************************************
NAME
    UpgradePartitionDataCopyFromStream  -  Copy from stream.

DESCRIPTION
    Accounts for differences in offset value with CONFIG_HYDRACORE version.

RETURNS
    Nothing.
*/
void UpgradePartitionDataCopyFromStream(uint8 *signature, uint16 offset, uint8 *data, uint16 len)
{
    ByteUtilsMemCpyFromStream(&signature[offset/2], data, len);
}

