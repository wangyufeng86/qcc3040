/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of responses to AT commands
*/

#include "device_test_service.h"
#include "device_test_service_data.h"

#include <vmtypes.h>
#include <sink.h>
#include <panic.h>
#include <byte_utils.h>
#include <logging.h>

#ifdef INCLUDE_DEVICE_TEST_SERVICE

#define MAX_RESPONSE_LEN 128

#define CONST_STRING(_s) (_s),(sizeof(_s)-1)

static const uint8 device_test_service_fixed_response_OK[] = "\r\nOK\r\n";
static const uint8 device_test_service_fixed_response_ERROR[] = "\r\nERROR\r\n";


static void deviceTestService_Respond(Task task, const uint8 *response, unsigned response_length)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    UNUSED(task);

    if (!dts->active || !dts->rfc_sink)
    {
        return;
    }

    SinkClaim(dts->rfc_sink, response_length);
    uint8 *base = SinkMap(dts->rfc_sink);
    memcpy(base, response, response_length);
    SinkFlush(dts->rfc_sink, response_length);

}

void DeviceTestService_CommandResponse(Task task, const char *response, unsigned length)
{
    uint8 extended_buffer[MAX_RESPONSE_LEN+4] = "\r\n";
    uint16 target_string_length = length ? MIN(length, MAX_RESPONSE_LEN) : MAX_RESPONSE_LEN;
    uint16 string_length;

    string_length = ByteUtilsStrnlen_S((const uint8 *) response, target_string_length);
    PanicZero(string_length);

    DEBUG_LOG_DEBUG("DeviceTestService_CommandResponse: Starts %c%c",response[0],response[1]);

    memcpy(extended_buffer+2, response, string_length);
    extended_buffer[string_length+2] = '\r';
    extended_buffer[string_length+3] = '\n';

    deviceTestService_Respond(task, extended_buffer, string_length + 4);
}


void DeviceTestService_CommandResponseOk(Task task)
{
    DEBUG_LOG_FN_ENTRY("DeviceTestService_CommandResponseOK");

    deviceTestService_Respond(task, CONST_STRING(device_test_service_fixed_response_OK));
}

void DeviceTestService_CommandResponseError(Task task)
{
    /* Include command errors in test log */
    DEBUG_LOG_ALWAYS("DeviceTestService_CommandResponseERROR");

    deviceTestService_Respond(task, CONST_STRING(device_test_service_fixed_response_ERROR));
}

void DeviceTestService_CommandResponseOkOrError(Task task, bool success)
{
    if (success)
    {
        DeviceTestService_CommandResponseOk(task);
    }
    else
    {
        DeviceTestService_CommandResponseError(task);
    }
}

#endif /* INCLUDE_DEVICE_TEST_SERVICE */
