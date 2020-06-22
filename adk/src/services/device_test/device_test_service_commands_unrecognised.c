/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Implementation of unrecognised command handler for the device test service
*/
//#define DEVELOPMENT_DEBUG

#include "device_test_service.h"
#include "device_test_parse.h"

#include <logging.h>

#include <ctype.h>

/*! Handle a string that the parser did not recognise.

    Check a received string for containing 'AT' at the beginning and
    respond with an ERROR if so.

    Other incorrect strings will be ignored
 */
void DeviceTestServiceParser_handleUnrecognised(const uint8 *data, uint16 length, Task task)
{
#ifdef INCLUDE_DEVICE_TEST_SERVICE
    DEBUG_LOG_ALWAYS("DeviceTestServiceParser_handleUnrecognised Called with %d length", length);

#ifdef DEVELOPMENT_DEBUG
    uint16 to_print = MIN(length,20);
    uint16 i;

#define P(_i) ((_i<to_print)?(isprint(data[_i])?data[_i]:'.'):'_')
    for (i=0; i < to_print; i+=5)
    {
        DEBUG_LOG_VERBOSE("??: %c%c%c%c%c",P(i),P(i+1),P(i+2),P(i+3),P(i+4));
    }
#endif

    /* Skip and CR\LF at the beginning of the string. The parser can 
       have trouble with these. */
    while (   length >= 2 
           && (data[0] == '\r' || data[0] == '\n'))
    {
        data++;
        length --;
    }

    if (   length >= 2
        && toupper(data[0]) == 'A'
        && toupper(data[1]) == 'T')
    {
        DeviceTestService_CommandResponseError(task);
    }
    
#else /* INCLUDE_DEVICE_TEST_SERVICE */
    UNUSED(data);
    UNUSED(length);
    UNUSED(task);
#endif /* INCLUDE_DEVICE_TEST_SERVICE */
}

