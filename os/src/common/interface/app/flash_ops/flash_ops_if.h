/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*   %%version */
/****************************************************************************
FILE
    flash_ops_if.h

CONTAINS
    Definitions used by flash operation traps.

*/

#ifndef __APP_FLASH_OPS_IF_H__
#define __APP_FLASH_OPS_IF_H__

/*! @brief Enumeration of flash UID retrieval methods
 */
typedef enum
{
    /*! UID read method 0
     * Typically used by:
     *   GigaDevice
     *   Winbond
     *   Puya
     *   ISSI
     *   Some XMC devices
     *
     * Read sequence:
     *   0x4B   UID read command
     *   0x00   Address[0]
     *   0x00   Address[1]
     *   0x00   Address[2]
     *   0x00   Dummy byte
     *   16     Bytes read
     */
    FLASH_OPS_UID_READ_METHOD0,

    /*! UID read method 1
     * Typically used by:
     *   Adesto
     *   Fidelix
     *   Macronix
     *
     * Read sequence:
     *   0xB1   OTP enter command
     *   0x03   SPI read command
     *   0x00   Address[0]
     *   0x00   Address[1]
     *   0x00   Address[2]
     *   16     Bytes read
     *   0xC1   OTP exit command
     */
    FLASH_OPS_UID_READ_METHOD1,

    /*! UID read method 2
     * Typically used by:
     *   Cypress (Spansion)
     *
     * Read sequence:
     *   0x4B   OTP read command
     *   0x00   Address[0]
     *   0x00   Address[1]
     *   0x00   Address[2]
     *   0x00   Dummy byte
     *   16     Bytes read
     */
    FLASH_OPS_UID_READ_METHOD2,
    
    /*! UID read method 3 
     * Typically used by:
     *   ESMT
     *   Some XMC devices
     *
     * Read sequence:
     *   0x5A   SFDP read command
     *   0x00   Address[0]
     *   0x00   Address[1]
     *   0x80   Address[2]
     *   0x00   Dummy byte
     *   12     Bytes read
     *   4      Bytes zero pad
     */
    FLASH_OPS_UID_READ_METHOD3,
    
    /*! UID read method 4 
     * Typically used by:
     *   Micron
     *
     * Read sequence:
     *   0x9F   JEDEC identification command
     *   0x00   Dummy byte[0]
     *   0x00   Dummy byte[1]
     *   0x00   Dummy byte[2]
     *   0x00   Dummy byte[3]
     *   0x00   Dummy byte[4]
     *   0x00   Dummy byte[5]
     *   14     Bytes read
     *   2      Bytes zero pad
     */
    FLASH_OPS_UID_READ_METHOD4
} flash_ops_uid_read_method;

/*! @brief Enumeration of flash OTP retrieval methods
 */
typedef enum
{
    /*! OTP read method 0
     * Typically used by:
     *   GigaDevice
     *   Winbond
     *   Puya
     *
     * Read sequence:
     *   0x48   UID read command
     *   0xXX   Address[0]
     *   0xXX   Address[1]
     *   0xXX   Address[2]
     *   0x00   Dummy byte
     *   YY     Bytes read
     */
    FLASH_OPS_OTP_READ_METHOD0,

    /*! OTP read method 1 
     * Typically used by:
     *   Adesto
     *   Fidelix
     *   Macronix
     *
     * Read sequence:
     *   0xB1   OTP enter command
     *   0x03   SPI read command
     *   0xXX   Address[0]
     *   0xXX   Address[1]
     *   0xXX   Address[2]
     *   YY     Bytes read
     *   0xC1   OTP exit command
     */
    FLASH_OPS_OTP_READ_METHOD1,

    /*! OTP read method 2 
     * Typically used by:
     *   Cypress (Spansion)
     *   Micron
     *
     * Read sequence:
     *   0x4B   OTP read command
     *   0xXX   Address[0]
     *   0xXX   Address[1]
     *   0xXX   Address[2]
     *   0x00   Dummy byte
     *   YY     Bytes read
     */
    FLASH_OPS_OTP_READ_METHOD2,
    
    /*! OTP read method 3
     * Typically used by:
     *   ESMT
     *   Some XMC devices
     *
     * Read sequence:
     *   0x3A   OTP enter command
     *   0x03   SPI read command
     *   0xXX   Address[0]
     *   0xXX   Address[1]
     *   0xXX   Address[2]
     *   YY     Bytes read
     *   0x04   OTP exit command
     *
     * Note that OTP area is mapped to the top of the top sectors of the flash.
     */
    FLASH_OPS_OTP_READ_METHOD3,
    
    /*! OTP read method 4
    * Typically used by:
    *   ISSI
    *
    * Read sequence:
    *   0x68   OTP read command
    *   0xXX   Address[0]
    *   0xXX   Address[1]
    *   0xXX   Address[2]
    *   0x00   Dummy byte
    *   YY     Bytes read
    */
    FLASH_OPS_OTP_READ_METHOD4
} flash_ops_otp_read_method;

#endif /* __APP_FLASH_OPS_IF_H__ */
