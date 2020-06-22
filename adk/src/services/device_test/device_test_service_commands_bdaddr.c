/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the bdaddr command, provided as an example, for the 
            device test service.

            This illustrates that commands are allowed to take some time to process
            and may use tasks. When the response is finally generated, the current
            state of the device test service should be checked, using 
            DeviceTestService_CommandsAllowed().
*/

#include "device_test_service.h"
#include "device_test_service_auth.h"
#include "device_test_parse.h"

#include <bt_device.h>
#include <stdio.h>
#include <logging.h>

/*! The base part of the AT command response */
#define LOCAL_BDADDR_RESPONSE "+LOCALBDADDR:"

/*! The length of the full response, including the maximum length of 
    any variable portion. As we use sizeof() this will include the 
    terminating NULL character*/
#define FULL_BDADDR_RESPONSE_LEN (sizeof(LOCAL_BDADDR_RESPONSE) + 12)

static void deviceTestService_BdaddrMessageHandler(Task task, MessageId id, Message message);

/* Structure to hold information for this command handler, including 
   the Task */
static struct
{
    TaskData    bdaddr_task;
    bool        single_response_due;
} device_test_service_bdaddr_taskdata = { .bdaddr_task = deviceTestService_BdaddrMessageHandler,
                                        };

/*! Message hander.

    Although we only expect one message, the function includes
    safety checks on the message ID and whether we are expecting
    to be called.

    If we are expecting to respond, then we ensure that a response 
    is sent.

    \param task[in] The task this message was sent to
    \param id       Identifier of the message
    \param message  The message content
 */
static void deviceTestService_BdaddrMessageHandler(Task task, MessageId id, Message message)
{
    DEBUG_LOG_DEBUG("deviceTestService_BdaddrMessageHandler");

    if (!device_test_service_bdaddr_taskdata.single_response_due)
    {
        return;
    }

    device_test_service_bdaddr_taskdata.single_response_due = FALSE;

    if (   DeviceTestService_CommandsAllowed()
        && CL_DM_LOCAL_BD_ADDR_CFM == id)
    {
        const CL_DM_LOCAL_BD_ADDR_CFM_T *local_cfm = (CL_DM_LOCAL_BD_ADDR_CFM_T *)message;

        if (hci_success == local_cfm->status)
        {
            char response[FULL_BDADDR_RESPONSE_LEN];

            sprintf(response, LOCAL_BDADDR_RESPONSE "%04x%02x%06lx", 
                    local_cfm->bd_addr.nap, local_cfm->bd_addr.uap, local_cfm->bd_addr.lap);

            DeviceTestService_CommandResponse(task, response, FULL_BDADDR_RESPONSE_LEN);
            DeviceTestService_CommandResponseOk(task);
            return;
        }
    }
    DeviceTestService_CommandResponseError(task);
}


/*! Command handler for AT+LOCALBDADDR?

    The function decides if the command is allowed, and if so requests
    the local address, using a task for the response.

    Otherwise errors are reported

    \param[in] task The task to be used in command responses
 */
void DeviceTestServiceCommand_HandleLocalBdaddr(Task task)
{
    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleLocalBdaddr");

    if (!DeviceTestService_CommandsAllowed())
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    device_test_service_bdaddr_taskdata.single_response_due = TRUE;
    ConnectionReadLocalAddr(&device_test_service_bdaddr_taskdata.bdaddr_task);
}

