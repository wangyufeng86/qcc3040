/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       ama_ble.h
\brief      Implementation for AMA BLE.
*/
    
/*! \brief Register AMA LE Advertising Manager
    \param void
    \return void
*/
void AmaBle_RegisterAdvertising(void);

/*! \brief Send AMA data using BLE
    \param data Pointer to uint8 data
    \param length Number of octets to send
    \return TRUE if data was sent, FALSE otherwise
*/
bool AmaBle_SendData(uint8* data, uint16 length);
