/****************************************************************************
Copyright (c) 2004 - 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    dm_baseband_scan.c        

DESCRIPTION
    This file contains the implementation of the entity responsible for 
    configuring the local baseband. This includes making the device
    discoverable, connectable etc.

NOTES

*/


/****************************************************************************
    Header files
*/
#include    "connection.h"
#include    "connection_private.h"
#include    "common.h"
#include    "dm_baseband_scan.h"

#include <message.h>
#include <panic.h>
#include <string.h>
#include <vm.h>


/*****************************************************************************/
void ConnectionWriteInquiryScanType(hci_scan_type type)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ);

    message->type = type;
    
    MessageSend(connectionGetCmTask(),
                CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ,
                message);
}

/*****************************************************************************/
void ConnectionWritePageScanType(hci_scan_type type)
{
    MAKE_CL_MESSAGE(CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ);

    message->type = type;
    
    MessageSend(connectionGetCmTask(),
                CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ,
                message);
}

/*****************************************************************************/
void ConnectionWriteScanEnable(hci_scan_enable mode)
{
    /* Check params are within allowed values - debug build only */
#ifdef CONNECTION_DEBUG_LIB    
    if (mode > hci_scan_enable_inq_and_page)
    {
        CL_DEBUG(("Out of range scan enable 0x%x\n", mode));
    }
#endif
    {
        signed delta;
        /* All requests are sent through the internal state handler */
        MAKE_CL_MESSAGE(CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ);
        message->mode = mode;
        /* Cancel any previous message before issuing a new one */
        delta = 1 - MessageCancelAll(connectionGetCmTask(), CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ);
        MessageSend(connectionGetCmTask(), CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ, message);
        connectionOutstandingWriteScanEnableReqsSetDelta(delta);
    }
}

void connectionHandleWriteScanEnableCfm(const DM_HCI_WRITE_SCAN_ENABLE_CFM_T *cfm)
{
    Task scanEnableTask = connectionGetScanEnableTask();
    
    connectionOutstandingWriteScanEnableReqsSetDelta(-1);
    
    if (scanEnableTask != NULL)
    {
        MAKE_CL_MESSAGE(CL_DM_WRITE_SCAN_ENABLE_CFM);
        message->status = cfm->status;
        message->outstanding = connectionOutstandingWriteScanEnableReqsGet();
        MessageSend(scanEnableTask, CL_DM_WRITE_SCAN_ENABLE_CFM, message);
    }
    else
    {
        checkStatus();
    }
}
