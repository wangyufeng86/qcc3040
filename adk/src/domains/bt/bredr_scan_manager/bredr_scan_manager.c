/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       bredr_scan_manager.c
\brief	    Implementation of module managing inquiry and page scanning.
*/
#include "bredr_scan_manager_private.h"

#include <connection.h>
#include <stdlib.h>
#include "sdp.h"
#include "bt_device.h"

/*! Qualcomm Bluetooth SIG company ID */
#define appConfigBtSigCompanyId() (0x00AU)

/*! Macro to insert UUID into EIR data, order of octets swapped as EIR data is little-endian */
#define EIR_UUID128_2(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)  p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a
#define EIR_UUID128(uuid)  EIR_UUID128_2(uuid)

/*! Maximum packet size of a DM3 packet  */
#define MAX_PACKET_SIZE_DM3 (121)
/*! Maximum size of content in an extended inquiry response (EIR) packet */
#define EIR_MAX_SIZE        (MAX_PACKET_SIZE_DM3)

/*! \name Tags that can be used in an extended inquiry response (EIR) */
#define EIR_TYPE_LOCAL_NAME_COMPLETE        (0x09)
#define EIR_TYPE_LOCAL_NAME_SHORTENED       (0x08)
#define EIR_TYPE_UUID16_COMPLETE            (0x03)
#define EIR_TYPE_UUID128_COMPLETE           (0x07)
#define EIR_TYPE_MANUFACTURER_SPECIFIC      (0xFF)
#define EIR_SIZE_MANUFACTURER_SPECIFIC      (10)  /* Doesn't include size field */


/*! \brief Scan Manager component main data structure. */
bredr_scan_manager_state_t bredr_scan_manager;

static void bredrScanManager_MessageHandler(Task task, MessageId id, Message msg);


bool BredrScanManager_Init(Task init_task)
{
    DEBUG_LOG("BredrScanManager_Init");

    memset(&bredr_scan_manager, 0, sizeof(bredr_scan_manager));
    bredr_scan_manager.task_data.handler = bredrScanManager_MessageHandler;
    bredrScanManager_InstanceInit(bredrScanManager_PageScanContext(), 0);
    bredrScanManager_InstanceInit(bredrScanManager_InquiryScanContext(), INQ_SCAN_MESSAGE_OFFSET);

    ConnectionScanEnableRegisterTask(&bredr_scan_manager.task_data);
    ConnectionWritePageScanType(hci_scan_type_interlaced);
    ConnectionWriteInquiryScanType(hci_scan_type_interlaced);

    MessageSend(init_task, BREDR_SCAN_MANAGER_INIT_CFM, NULL);

    return TRUE;
}

void BredrScanManager_PageScanParametersRegister(const bredr_scan_manager_parameters_t *params)
{
    DEBUG_LOG("BredrScanManager_PageScanParametersRegister %p", params);
    bredrScanManager_InstanceParameterSetRegister(bredrScanManager_PageScanContext(), params);
}

void BredrScanManager_InquiryScanParametersRegister(const bredr_scan_manager_parameters_t *params)
{
    DEBUG_LOG("BredrScanManager_InquiryScanParametersRegister %p", params);
    bredrScanManager_InstanceParameterSetRegister(bredrScanManager_InquiryScanContext(), params);
}

void BredrScanManager_PageScanParametersSelect(uint8 index)
{
    DEBUG_LOG("BredrScanManager_PageScanParametersSelect %d", index);
    bredrScanManager_InstanceParameterSetSelect(bredrScanManager_PageScanContext(), index);
}

void BredrScanManager_InquiryScanParametersSelect(uint8 index)
{
    DEBUG_LOG("BredrScanManager_InquiryScanParametersSelect %d", index);
    bredrScanManager_InstanceParameterSetSelect(bredrScanManager_InquiryScanContext(), index);
}

void BredrScanManager_InquiryScanRequest(Task client, bredr_scan_manager_scan_type_t inq_type)
{
    DEBUG_LOG("BredrScanManager_InquiryScanRequest client %p type %d", client, inq_type);
    bredrScanManager_InstanceClientAddOrUpdate(bredrScanManager_InquiryScanContext(), client, inq_type);
}

void BredrScanManager_InquiryScanRelease(Task client)
{
    DEBUG_LOG("BredrScanManager_InquiryScanRelease client %d", client);
    bredrScanManager_InstanceClientRemove(bredrScanManager_InquiryScanContext(), client);
}

void BredrScanManager_PageScanRequest(Task client, bredr_scan_manager_scan_type_t page_type)
{
    DEBUG_LOG("BredrScanManager_PageScanRequest client %d type %d", client, page_type);
    bredrScanManager_InstanceClientAddOrUpdate(bredrScanManager_PageScanContext(), client, page_type);
}

void BredrScanManager_PageScanRelease(Task client)
{
    DEBUG_LOG("BredrScanManager_PageScanRelease client %d", client);
    bredrScanManager_InstanceClientRemove(bredrScanManager_PageScanContext(), client);
}

bool BredrScanManager_IsPageScanEnabledForClient(Task client)
{
    return bredrScanManager_InstanceIsScanEnabledForClient(bredrScanManager_PageScanContext(), client);
}

void BredrScanManager_ScanDisable(Task disabler)
{
    Task old_disabler;
    PanicNull(disabler);
    old_disabler = bredrScanManager_SetDisableTask(disabler);
    PanicFalse(old_disabler == NULL || old_disabler == disabler);

    bredrScanManager_InstancePause(bredrScanManager_PageScanContext());
    bredrScanManager_InstancePause(bredrScanManager_InquiryScanContext());

    if (BredrScanManager_IsScanDisabled())
    {
        bredrScanManager_SendDisableCfm(TRUE);
    }
}

void BredrScanManager_ScanEnable(void)
{
    if (   !BredrScanManager_IsScanDisabled() 
        && bredrScanManager_GetDisableTask())
    {
        bredrScanManager_SendDisableCfm(FALSE);
    }
    bredrScanManager_SetDisableTask(NULL);

    bredrScanManager_InstanceResume(bredrScanManager_PageScanContext());
    bredrScanManager_InstanceResume(bredrScanManager_InquiryScanContext());
}

/*! @brief Set up Eir data and write to the connection library. */
static void bredrScanManager_InitialiseEir(const uint8 *local_name, uint16 size_local_name)
{
    static const uint8 eir_16bit_uuids[] =
    {
        EIR_TYPE_UUID16_COMPLETE, /* Complete list of 16-bit Service Class UUIDs */
#ifdef INCLUDE_HFP
        0x1E, 0x11,     /* HFP 0x111E */
        0x08, 0x11,     /* HSP 0x1108 */
#endif
#ifdef INCLUDE_AV
        0x0B, 0x11,     /* AudioSink 0x110B */
        0x0D, 0x11,     /* A2DP 0x110D */
        0x0E, 0x11      /* AVRCP 0x110E */
#endif
    };

#ifndef DISABLE_TWS_PLUS
    static const uint8 eir_128bit_uuids[] =
    {
        EIR_TYPE_UUID128_COMPLETE, /* Complete list of 128-bit Service Class UUIDs */
        EIR_UUID128(UUID_TWS_SINK_SERVICE)
    };
#endif

    uint8 *const eir = (uint8 *)PanicUnlessMalloc(EIR_MAX_SIZE + 1);
    uint8 *const eir_end = eir + EIR_MAX_SIZE + 1;
    uint8 *eir_ptr = eir;
    uint16 eir_space;
    bdaddr peer_bdaddr;

    DEBUG_LOG("scanManager_InitialiseEir");

    /* Get peer device address, default to all 0's if we haven't paired */
    BdaddrSetZero(&peer_bdaddr);
    appDeviceGetPeerBdAddr(&peer_bdaddr);

    /* Copy 16 bit UUIDs into EIR packet */
    *eir_ptr++ = sizeof(eir_16bit_uuids);
    memmove(eir_ptr, eir_16bit_uuids, sizeof(eir_16bit_uuids));
    eir_ptr += sizeof(eir_16bit_uuids);

#ifndef DISABLE_TWS_PLUS
    /* Copy 128 bit UUIDs into EIR packet */
    *eir_ptr++ = sizeof(eir_128bit_uuids);
    memmove(eir_ptr, eir_128bit_uuids, sizeof(eir_128bit_uuids));
    eir_ptr += sizeof(eir_128bit_uuids);

    /* Add TWS+ EIR data */
    *eir_ptr++ = EIR_SIZE_MANUFACTURER_SPECIFIC;
    *eir_ptr++ = EIR_TYPE_MANUFACTURER_SPECIFIC;
    *eir_ptr++ = (appConfigBtSigCompanyId() >> 0) & 0xFF;  /* LSB of CompanyID */
    *eir_ptr++ = (appConfigBtSigCompanyId() >> 8) & 0xFF;  /* MSB of CompanyID */
    *eir_ptr++ = 0x20;                              /* Form factor for BD_ADDR */
    *eir_ptr++ = (peer_bdaddr.lap >>  0) & 0xFF;    /* LSB of LAP */
    *eir_ptr++ = (peer_bdaddr.lap >>  8) & 0xFF;
    *eir_ptr++ = (peer_bdaddr.lap >> 16) & 0xFF;    /* MSB of LAP */
    *eir_ptr++ = peer_bdaddr.uap;                   /* UAP */
    *eir_ptr++ = (peer_bdaddr.nap >>  0) & 0xFF;    /* LSB of NAP */
    *eir_ptr++ = (peer_bdaddr.nap >>  8) & 0xFF;    /* MSB of NAP */
#endif

    /* Calculate space for local device name */
    eir_space = (eir_end - eir_ptr) - 3;  /* Take 3 extra from space for type and size fields and zero terminator */

    /* Check if name need trucating */
    if (eir_space < size_local_name)
    {
        /* Store header for truncated name */
        *eir_ptr++ = eir_space + 1;
        *eir_ptr++ = EIR_TYPE_LOCAL_NAME_SHORTENED;

        /* Clip size of name to space available */
        size_local_name = eir_space;
    }
    else
    {
        /* Store header for complete name */
        *eir_ptr++ = size_local_name + 1;
        *eir_ptr++ = EIR_TYPE_LOCAL_NAME_COMPLETE;
    }

    /* Copy local device name into EIR packet */
    memmove(eir_ptr, local_name, size_local_name);
    eir_ptr += size_local_name;

    /* Add termination character */
    *eir_ptr++ = 0x00;

    /* Write EIR data */
    ConnectionWriteEirData(FALSE, eir_ptr - eir, eir);

    /* Free the EIR data */
    free(eir);
}

/*! \brief Local name request completed

    The request for the local device name has completed, pass the name to
    scanManager_InitialiseEir() to allow the Extended Inquiry Response to be initialised.
*/
static void scanManager_HandleClDmLocalNameComplete(CL_DM_LOCAL_NAME_COMPLETE_T *msg)
{
    bool setup_success = FALSE;

    DEBUG_LOG("scanManager_HandleClDmLocalNameComplete, status %d", msg->status);

    if(msg->status == hci_success)
    {
        /* Initialise pairing module, this will set EIR data */
        bredrScanManager_InitialiseEir(msg->local_name, msg->size_local_name);
        setup_success = TRUE;
    }
    bredr_scan_manager.eir_setup_in_progress = FALSE;

    if(bredr_scan_manager.eir_setup_complete_callback)
    {
        bredr_scan_manager.eir_setup_complete_callback(setup_success);
    }
}


static void bredrScanManager_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case CL_DM_WRITE_SCAN_ENABLE_CFM:
            bredrScanManager_ConnectionHandleClDmWriteScanEnableCfm(msg);
        break;

        case CL_DM_LOCAL_NAME_COMPLETE:
            scanManager_HandleClDmLocalNameComplete((CL_DM_LOCAL_NAME_COMPLETE_T *)msg);
        break;

        default:
            DEBUG_LOG("bredrScanManager_MessageHandler unhandled message 0x%x", id);
            Panic();
        break;
    }
}

bool ScanManager_ConfigureEirData(eir_setup_complete_callback_t callback_function)
{
    DEBUG_LOG("ScanManager_ConfigureEirData");

    if(!bredr_scan_manager.eir_setup_in_progress)
    {
        bredr_scan_manager.eir_setup_in_progress = TRUE;
        bredr_scan_manager.eir_setup_complete_callback = callback_function;
        /* Get device name so that we can initialise EIR response */
        ConnectionReadLocalName(&bredr_scan_manager.task_data);
        return TRUE;
    }
    return FALSE;
}

/*! \brief Determine if scanning is disabled in all scan contexts.
    \return TRUE if disabled.
*/
bool BredrScanManager_IsScanDisabled(void)
{
    return ((bredr_scan_manager.page_scan.state == BSM_SCAN_DISABLED) &&
            (bredr_scan_manager.inquiry_scan.state == BSM_SCAN_DISABLED));
}

