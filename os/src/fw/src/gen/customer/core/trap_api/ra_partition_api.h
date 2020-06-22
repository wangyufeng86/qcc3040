#ifndef __RA_PARTITION_API_H__
#define __RA_PARTITION_API_H__
#include <app/ra_partition/ra_partition_if.h>

    /*! file  @brief Access Traps for RA Partition.
    **
    **
    RA Partition traps.
    */
    
#if TRAPSET_RA_PARTITION

/**
 *  \brief Initialize an handle to the (R)andom (A)ccess partition.
 *         Initialize an handle to the RA partition. This shall be used as a
 *  parameter
 *         in subsequent operations on the RA partition.
 *   
 *         On success, the handle is initialized and result is returned as success.
 *         On failure, the handle is uninitialized (set to 0) and result is an
 *         appropriate error code from the defined set.
 *         
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \param partition_info Pointer/reference to structure containing RA partition information obtained
 *  with RaPartitionOpen().
 *  \return Result of open operation on RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
raPartition_result RaPartitionOpen(raPartition_handle * handle, raPartition_info * partition_info);

/**
 *  \brief Close access to (R)andom (A)ccess partition.
 *         Invalidate the handle to the RA partition. So as to disallow subsequent
 *  access
 *         to the RA partition.
 *         The handle isn't unset on sanity test failure for handle and result is
 *  an
 *         appropriate error code from the defined set.
 *         In all other cases the handle is always unset (set to 0) and result is
 *         returned as success.
 *         
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \return Result of close operation on RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
raPartition_result RaPartitionClose(raPartition_handle * handle);

/**
 *  \brief Erase the sector within RA partition, in which the offset in contained.
 *         Erase the sector within RA partition that contains the specified offset.
 *         The sector/block size is based on the Flash configuration. This
 *  information is known as part of successful RaPartitionOpen().
 *         Mostly it may be either 64KB or 4KB sized.
 *         On success, result is returned as success.
 *         On failure, result is returned as an appropriate error code from the
 *  defined set.
 *         
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \param offset Offset (absolute) within RA partition.
 *  \return Result of erase operation on RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
raPartition_result RaPartitionErase(raPartition_handle * handle, uint32 offset);

/**
 *  \brief Write specified number of bytes from the buffer onto the RA partition and at
 *  the specified offset onwards.
 *         Write specified number of bytes from the buffer onto the RA partition
 *  and at the specified offset onwards.
 *         If specified number of bytes to write > (Partition size - offset)
 *             Here Partition size: is known as part of successful
 *  RaPartitionOpen().
 *                  offset: is the (absolute) location within RA partition.
 *         then, bytes are written in a wrap around fashion i.e. the exceeding
 *  bytes are written
 *               from the Partition start.
 *         All bytes are sequentially written.
 *         The caller shall prepare enough headroom of erased locations at least
 *  equivalent to
 *         the write buffer to be committed to the RA partition. If not then the
 *  written byte value may be
 *         undefined (especially because Flash charactistics and no internal erase
 *  prior to programming the
 *         write bytes.
 *         On success, specified bytes are written to RA partiton and result is
 *  returned as success.
 *         On failure, result is returned as an appropriate error code from the
 *  defined set.
 *         
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \param offset Offset (absolute) within RA partition.
 *  \param length Number of bytes to write onto RA partition.
 *  \param buffer Caller provided buffer (i.e. array of bytes) to write onto RA partition.
 *  \return Result of write operation on RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
raPartition_result RaPartitionWrite(raPartition_handle * handle, uint32 offset, uint32 length, const uint8 * buffer);

/**
 *  \brief Read specified number of bytes into the buffer from specified offset onwards
 *  and within RA partition.
 *         Read specified number of bytes into the buffer from the RA partition
 *  and from the specified offset onwards.
 *         If specified number of bytes to read > (Partition size - offset)
 *             Here Partition size: is known as part of successful
 *  RaPartitionOpen().
 *                  offset: is the (absolute) location within RA partition.
 *         then, bytes are read in a wrap around fashion i.e. the exceeding bytes
 *  are read
 *               from the Partition start.
 *         All bytes are sequentially read.
 *         The caller shall manage the memory for the buffer into which specified
 *  number of bytes are read.
 *         Also the buffer shall be large enough to hold the specified number of
 *  bytes to read.
 *         On success, specified bytes are read from RA partiton and result is
 *  returned as success.
 *         On failure, result is returned as an appropriate error code from the
 *  defined set.
 *         
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \param offset Offset (absolute) within RA partition.
 *  \param length Number of bytes to read from RA partition.
 *  \param buffer Caller provided buffer (i.e. array of bytes) to read from RA partition.
 *  \return Result of read operation on RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
raPartition_result RaPartitionRead(raPartition_handle * handle, uint32 offset, uint32 length, uint8 * buffer);

/**
 *  \brief Erase the sector within RA partition (in back-ground/non-blocking fashion), in
 *  which the offset in contained.
 *       Erase the sector within RA partition that contains the specified offset.
 *       The sector/block size is based on the Flash configuration. This
 *  information is known as part of successful RaPartitionOpen().
 *       Mostly it may be either 64KB or 4KB sized.
 *       On completion, MESSAGE_RA_PARTITION_BG_ERASE_STATUS message is sent to the
 *       task registered with MessageSystemTask() Apps P1.
 *       For corresponding message content refer \#MessageRaPartitionBgEraseStatus
 *       
 *  \param handle Pointer/reference to RA Partition handle obtained with RaPartitionOpen().
 *  \param offset Offset (absolute) within RA partition.
 * 
 * \ingroup trapset_ra_partition
 */
void RaPartitionBgErase(raPartition_handle * handle, uint32 offset);
#endif /* TRAPSET_RA_PARTITION */
#endif
