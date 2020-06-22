/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_baseband_scan.h

DESCRIPTION
    This file contains the prototypes for baseband scan msgs from Bluestack.

NOTES

*/

/****************************************************************************
NAME
    connectionHandleWriteScanEnableCfm

DESCRIPTION
    Handle the DM_HCI_WRITE_SCAN_ENABLE_CFM message from Bluestack and pass it
    on to the application that initialised the CL.
*/
void connectionHandleWriteScanEnableCfm(
        const DM_HCI_WRITE_SCAN_ENABLE_CFM_T *cfm);
