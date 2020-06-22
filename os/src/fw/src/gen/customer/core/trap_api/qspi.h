#ifndef __QSPI_H__
#define __QSPI_H__
#include <app/flash_ops/flash_ops_if.h>

        /*! file  @brief Functions for accessing miscellaneous flash properties
        such as registers, not including the main flash array. */
      
#if TRAPSET_FLASH_OPS

/**
 *  \brief Retrieve the identification reponse for Apps flash
 *   At boot, the system manager sends the JEDEC ID command (0x9F) to the
 *  Application flash to
 *   load the correct flash configuration. This trap retrieves the saved response
 *  from system
 *   manager RAM and writes to storage locations passed as parameters.
 *         
 *  \param jedec_id             Pointer to storage for JEDEC ID
 *             
 *  \param device_id             Pointer to storage for device ID
 *             
 * 
 * \ingroup trapset_flash_ops
 */
void QspiIdentityRequest(uint8 * jedec_id, uint16 * device_id);

/**
 *  \brief Read the Unique ID of the Apps flash
 *   Blocks to read the Unique ID of the Apps flash.
 *   Commands and command sequences are different among flash manufacturers.
 *  'uid_method' supplied will determine
 *   which read method is used. Refer to documentation to see which flash IDs
 *  should use which method.
 *   Sixteen bytes will always be written to 'uid'. UID methods for flash parts
 *  with UIDs smaller than 128-bits
 *   will be zero-padded where possible.
 *         
 *  \param uid_method             UID read method to be used. See documentation for available methods
 *  and the flash parts they are suitable for.
 *             
 *  \param uid             Pointer to storage for UID. 16 bytes will be written to this
 *  address, whether the flash device has a 128-bit
 *             Unique ID or not.
 *             
 *  \return           Boolean to indicate whether the operation was successful or not.
 *           
 * 
 * \ingroup trapset_flash_ops
 */
bool QspiUIDRequest(flash_ops_uid_read_method uid_method, uint8 * uid);

/**
 *  \brief Read a portion of the One Time Programmable area of the Apps flash
 *   Blocks to read the One Time Programmable area of the Apps flash.
 *   Commands and command sequences are different among flash manufacturers.
 *  'otp_method' supplied will determine
 *   which read method is used. Refer to documentation to see which flash IDs
 *  should use which method.
 *   Up to 8KB of OTP data can be read per call. Passing a read length of zero
 *  will result in the maximum length
 *   being written to the memory location pointed to by 'otp'.
 *         
 *  \param otp_method             UID read method to be used. See documentation for available methods
 *  and the flash parts they are suitable for.
 *             
 *  \param otp_address             Address within the flash OTP area from which data is to be read.
 *             
 *  \param otp_length             Number of bytes to be read from the flash OTP area and then written
 *  to the memory location pointed to by 'otp'.
 *             
 *  \param otp             Pointer to storage for OTP data
 *             
 *  \return           Boolean to indicate whether the operation was successful or not.
 *           
 * 
 * \ingroup trapset_flash_ops
 */
bool QspiOTPRequest(flash_ops_otp_read_method otp_method, uint32 otp_address, uint16 otp_length, uint8 * otp);
#endif /* TRAPSET_FLASH_OPS */
#endif
