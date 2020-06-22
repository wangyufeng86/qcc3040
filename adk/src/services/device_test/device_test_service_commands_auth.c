/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of the auth commands for device test service
*/

#include "device_test_service.h"
#include "device_test_service_data.h"
#include "device_test_service_auth.h"
#include "device_test_service_key.h"
#include "device_test_parse.h"

#include <util.h>
#include <logging.h>
#include <cryptovm.h>

static void deviceTestService_GenerateRandom(DTS_AUTH_KEY_T *random_key)
{
    int r;
    uint16 *key = (uint16*)random_key->aligned_u16;

    DEBUG_LOG_DEBUG("deviceTestService_GenerateRandom");

    for (r = 0; r < DTS_KEY_SIZE_WORDS; r++)
    {
        key[r] = UtilRandom();
    }
}


static char deviceTestService_HexChar(unsigned value)
{
    value = value & 0xF;

    if (value>=10)
    {
        return 'A' + value - 10;
    }
    return '0' + value;
}


static void deviceTestService_WriteByteAsHex(char *dest, uint8 value)
{
    dest[0] = deviceTestService_HexChar(value >> 4);
    dest[1] = deviceTestService_HexChar(value);
}


static void deviceTestService_CancelInProgressAuthentication(void)
{
    if (device_test_service.authenticated == device_test_service_authentication_in_progress)
    {
        DEBUG_LOG_FN_ENTRY("deviceTestService_CancelInProgressAuthentication");
        device_test_service.authenticated = device_test_service_not_authenticated;
    }
}

void DeviceTestServiceCommand_HandleAuthStart(Task task)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();
    char response[] = "+AUTHSTART:________________________________";
    int octet;
    int offset = 11;

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAuthStart");

    if (!DeviceTestServiceIsActive() || DeviceTestServiceIsAuthenticated())
    {
        DeviceTestService_CommandResponseError(task);
        return;
    }

    deviceTestService_GenerateRandom(&dts->challenge);
    for (octet = 0; octet < DTS_KEY_SIZE_OCTETS; octet++,offset+=2)
    {
        deviceTestService_WriteByteAsHex(&response[offset],dts->challenge.key_u8[octet]);
    }
    dts->authenticated = device_test_service_authentication_in_progress;

    DeviceTestService_CommandResponse(task, response, 0);
    DeviceTestService_CommandResponseOk(task);
}

static uint8 deviceTestService_HexToNumber(char hex_char)
{
    if ('A' <= hex_char && hex_char <= 'F')
    {
        return (hex_char - 'A' + 10);
    }
    if ('0' <= hex_char && hex_char <= '9')
    {
        return (hex_char - '0');
    }
    if ('a' <= hex_char && hex_char <= 'f')
    {
        return (hex_char - 'a' + 10);
    }

    /* Choose to return a value rather than Panic.
       Not exactly an attack vector, but better */
    return 0;
}


static uint8 deviceTestService_HexPairToNumber(char upper, char lower)
{
    return   (deviceTestService_HexToNumber(upper) << 4)
           + deviceTestService_HexToNumber(lower);
}


static void deviceTestService_ConvertHexToKey(const struct DeviceTestServiceCommand_HandleAuthResp *resp,
                                              DTS_AUTH_KEY_T *key)
{
    unsigned key_index;
    const char* hex_char = (const char*)resp->response.data;

    for (key_index = 0; key_index < DTS_KEY_SIZE_OCTETS; key_index++, hex_char+=2)
    {
        key->key_u8[key_index] = deviceTestService_HexPairToNumber(hex_char[0], hex_char[1]);
    }
}


static void deviceTestService_GenerateRespLocally(DTS_AUTH_KEY_T *response)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    CryptoVmAesCmac(device_test_service_auth_key.key_u8, 
                    dts->challenge.key_u8, 
                    DTS_KEY_SIZE_OCTETS, 
                    response->key_u8);
}


static bool deviceTestService_CheckAuthResp(DTS_AUTH_KEY_T *response, 
                                            DTS_AUTH_KEY_T *local)
{
    DEBUG_LOG_VERBOSE("Received: %02X%02X...%02X%02X",response->key_u8[0],response->key_u8[1],response->key_u8[14],response->key_u8[15]);
    DEBUG_LOG_VERBOSE("Local: %02X%02X...%02X%02X",local->key_u8[0],local->key_u8[1],local->key_u8[14],local->key_u8[15]);
    bool compare = !memcmp(response->key_u8, local->key_u8, sizeof(local->key_u8));
    DEBUG_LOG_INFO("deviceTestService_CheckAuthResp:%d",compare);

    return compare;
}


void DeviceTestServiceCommand_HandleAuthResp(Task task, const struct DeviceTestServiceCommand_HandleAuthResp *resp)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAuthResp %d octets", resp->response.length);

    if (   DeviceTestServiceIsActive() 
        && !DeviceTestServiceIsAuthenticated()
        && dts->authenticated == device_test_service_authentication_in_progress
        && resp->response.length == DTS_KEY_SIZE_HEX_NIBBLES)
    {
        DTS_AUTH_KEY_T local;
        DTS_AUTH_KEY_T received;

        deviceTestService_ConvertHexToKey(resp, &received);
        deviceTestService_GenerateRespLocally(&local);
        if (deviceTestService_CheckAuthResp(&received, &local))
        {
            dts->authenticated = device_test_service_authenticated;
            
            DeviceTestService_CommandResponseOk(task);
            return;
        }
    }
    DeviceTestService_CommandResponseError(task);
    deviceTestService_CancelInProgressAuthentication();
}


void DeviceTestServiceCommand_HandleAuthDisable(Task task)
{
    device_test_service_data_t *dts = DeviceTestServiceGetData();

    DEBUG_LOG_ALWAYS("DeviceTestServiceCommand_HandleAuthDisable");

    if (   !DeviceTestServiceIsActive() 
        || !DeviceTestServiceIsAuthenticated())
    {
        deviceTestService_CancelInProgressAuthentication();
        DeviceTestService_CommandResponseError(task);
        return;
    }

    dts->authenticated = device_test_service_not_authenticated;
    DeviceTestService_CommandResponseOk(task);
}


bool DeviceTestService_CommandsAllowed(void)
{
    if (DeviceTestServiceIsActive())
    {
        if (DeviceTestServiceIsAuthenticated())
        {
            return TRUE;
        }
        deviceTestService_CancelInProgressAuthentication();
    }
    return FALSE;
}

