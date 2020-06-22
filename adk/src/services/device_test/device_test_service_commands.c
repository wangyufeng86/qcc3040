/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the auth commands for device test service
*/

#include "device_test_service.h"
#include "device_test_service_data.h"
#include "device_test_service_config.h"
#include "device_test_service_messages.h"
#include "device_test_parse.h"

#include <phy_state.h>

#include <stdio.h>
#include <logging.h>

#ifdef INCLUDE_DEVICE_TEST_SERVICE

/*! The base part of the AT command response */
#define DTS_TEST_MODE_RESPONSE "+DTSTESTMODE:"

/*! The length of the full response, including the maximum length of 
    any variable portion. As we use sizeof() this will include the 
    terminating NULL character.
    Initially, the response has a single character but could expand
    to a full 16 bit (65535) so 5 characters allowed. */
#define FULL_DTS_TEST_MODE_RESPONSE_LEN (sizeof(DTS_TEST_MODE_RESPONSE) + 5)


void DeviceTestServiceCommand_HandleDtsTestModeQuery(Task task)
{
    char response_buffer[FULL_DTS_TEST_MODE_RESPONSE_LEN];
    uint16 test_mode;

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleDtsTestModeQuery");

    if (!DeviceTestServiceIsActive() || !DeviceTestServiceIsAuthenticated())
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    test_mode = DeviceTestService_TestMode();
    sprintf(response_buffer, DTS_TEST_MODE_RESPONSE "%u", test_mode);

    DeviceTestService_CommandResponse(task, response_buffer, 
                                      FULL_DTS_TEST_MODE_RESPONSE_LEN);
    DeviceTestService_CommandResponseOk(task);
}


void DeviceTestServiceCommand_HandleDtsEndTesting(Task task,
                        const struct DeviceTestServiceCommand_HandleDtsEndTesting *end_test)
{
    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleDtsEndTesting reboot:%d",
                        end_test->reboot);

    if (   !DeviceTestServiceIsActive()
        || !DeviceTestServiceIsAuthenticated()
        || end_test->reboot > 1)
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    /* Always end testing as this can clean up the device database */
    MessageSend(DeviceTestServiceGetTask(),
                DEVICE_TEST_SERVICE_INTERNAL_END_TESTING, NULL);

    if (end_test->reboot)
    {
        MessageSendLater(DeviceTestServiceGetTask(), 
                         DEVICE_TEST_SERVICE_INTERNAL_REBOOT, NULL,
                         DeviceTestService_RebootTimeOutMs());
    }

    if (DeviceTestService_TestMode())
    {
        /* If test mode is still enabled make sure we know to reboot */
        appPhyStateRegisterClient(DeviceTestServiceGetTask());
    }

    DeviceTestService_CommandResponseOk(task);
}


void DeviceTestServiceCommand_HandleDtsSetTestMode(Task task,
                        const struct DeviceTestServiceCommand_HandleDtsSetTestMode *set_test_mode)
{
    if (   !DeviceTestServiceIsActive() 
        || !DeviceTestServiceIsAuthenticated()
        || set_test_mode->testmode > 1)
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }
    DeviceTestService_SaveTestMode(set_test_mode->testmode);

    DeviceTestService_CommandResponseOkOrError(task, 
                            DeviceTestService_TestMode() == set_test_mode->testmode);
}

#else /* INCLUDE_DEVICE_TEST_SERVICE */

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleDtsTestModeQuery(Task task)
{
    UNUSED(task);
}

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleDtsEndTesting(Task task,
                        const struct DeviceTestServiceCommand_HandleDtsEndTesting *end_test)
{
    UNUSED(task);
    UNUSED(end_test);
}

/* Stub command included if the device test service is not included.
   Allows initial compile and link. Should be remmoved in final link as functions unused */
void DeviceTestServiceCommand_HandleDtsSetTestMode(Task task,
                        const struct DeviceTestServiceCommand_HandleDtsSetTestMode *set_test_mode)
{
    UNUSED(task);
    UNUSED(set_test_mode);
}

#endif /* INCLUDE_DEVICE_TEST_SERVICE */
