/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       authentication.c
\brief      Authentication and security functions
*/

#include <connection.h>
#include <sink.h>

#include "app_task.h"
#include "authentication.h"
#include "authentication_config.h"
#include "bt_device.h"
#include "adk_log.h"

/*! \brief Write the Authenticated Payload Timeout */
static void appAuthWriteApt(const bdaddr *bd_addr, uint16 timeout)
{
    tp_bdaddr device_addr;

    device_addr.transport = TRANSPORT_BREDR_ACL;
    device_addr.taddr.type = TYPED_BDADDR_PUBLIC;
    device_addr.taddr.addr = *bd_addr;

    if(timeout > HFP_AUTHENTICATED_PAYLOAD_TIMEOUT_SC_MAX_MS)
        timeout = HFP_AUTHENTICATED_PAYLOAD_TIMEOUT_SC_MAX_MS;

    DEBUG_LOG("appAuthWriteApt: %d", timeout);

    /* Write the APT value to the controller for the link
     * NB: Timeout is in units of 10ms */
    ConnectionWriteAPT(appGetAppTask(), &device_addr, timeout / 10, cl_apt_application);
}

/*! \brief  Sets the device link mode attributes based on the encryption type

    Updates the link mode to #DEVICE_LINK_MODE_SECURE_CONNECTION or
    #DEVICE_LINK_MODE_NO_SECURE_CONNECTION based on the encryption type.

    The Authenticated Payload Timeout (APT) is updated if secure connection
    is selected.

    \param  bd_addr         Bluetooth address of device whose attributes to update
    \param  encrypt_type    encryption type to use in the update
*/
static void appAuthHandleEncryptionChange(const bdaddr *bd_addr, cl_sm_encryption_key_type encrypt_type)
{
    /* Get the encryption type of BR/EDR link
     * if cl_sm_encryption_e0_brdedr_aes_ccm_le  then it is  non-SC  pairing
     * if cl_sm_encryption_aes_ccm_bredr then it is SC pairing. Store this
     * information to trigger APT write after HFP SLC for subsequent reconnection
     */

    if (encrypt_type == cl_sm_encryption_aes_ccm_bredr)
    {
        DEBUG_LOG("appAuthHandleEncryptionChange: link_mode SECURE_CONNECTION");
        appDeviceSetLinkMode(bd_addr, DEVICE_LINK_MODE_SECURE_CONNECTION);

        /* Write the APT value for the BR/EDR link */
        appAuthWriteApt(bd_addr, DEFAULT_BR_EDR_AUTHENTICATED_PAYLOAD_TIMEOUT);
    }
    else
    {
        DEBUG_LOG("appAuthHandleEncryptionChange: link_mode NO_SECURE_CONNECTION");
        appDeviceSetLinkMode(bd_addr, DEVICE_LINK_MODE_NO_SECURE_CONNECTION);
    }
}

/*! \brief Handle encryption change requests

    This function is called to handle a CL_SM_ENCRYPTION_CHANGE_IND message, this message is sent from the
    connection library when an encrption key is created or changed

    \param  ind     The change indication to process
*/
static void appAuthHandleClSmEncryptionChangeInd(CL_SM_ENCRYPTION_CHANGE_IND_T *ind)
{
    DEBUG_LOG("appAuthHandleClSmEncryptionChangeInd");

    if(ind->encrypted && ind->tpaddr.transport == TRANSPORT_BREDR_ACL)
        appAuthHandleEncryptionChange(&ind->tpaddr.taddr.addr, ind->encrypt_type);
}


bool appAuthHandleConnectionLibraryMessages(MessageId id, Message message,
                                                   bool already_handled)
{
    switch (id)
    {
        case CL_SM_ENCRYPTION_CHANGE_IND:
            appAuthHandleClSmEncryptionChangeInd((CL_SM_ENCRYPTION_CHANGE_IND_T *)message);
            return TRUE;
    }
    return already_handled;
}

