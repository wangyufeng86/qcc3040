/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   device_test_service Device Test Service
\ingroup    services
\brief      The Device Test Service provides a service for device testing and
            configuration.

@{
*/

#ifndef DEVICE_TEST_SERVICE_H
#define DEVICE_TEST_SERVICE_H

#include "domain_message.h"


/*! Size of the key used in authentication for the device test service */
/*! @{ */
#define DTS_KEY_SIZE_BITS           128
#define DTS_KEY_SIZE_OCTETS         (DTS_KEY_SIZE_BITS/8)
#define DTS_KEY_SIZE_WORDS          (DTS_KEY_SIZE_OCTETS / 2)
#define DTS_KEY_SIZE_HEX_NIBBLES    (DTS_KEY_SIZE_OCTETS * 2)
/*! @} */

typedef enum
{
    DEVICE_TEST_SERVICE_ENDED = DEVICE_TEST_MESSAGE_BASE,
} device_test_service_message_t;


#ifdef INCLUDE_DEVICE_TEST_SERVICE

/*! \brief Initialises the device test service.

    \param init_task Not used

    \return TRUE
 */
bool DeviceTestService_Init(Task init_task);


/*! Query whether test mode is selected

    \return TRUE if test mode is enabled at present 
 */
bool DeviceTestService_TestMode(void);


/*! Save the test mode, so will apply when next read 

    Used to preserve an updated test mode so that when the
    device reboots, the new mode can be used.

    \param mode The value to save
 */
void DeviceTestService_SaveTestMode(uint16 mode);


/*! Send a string response to the last command

    The task passed should match that sent in the function call.

    Responses will be terminated with \\r\\n automatically.

    The response supplied should be a normal C string, with a zero
    termination. The supplied length is used so it would be 
    legal to send a string with no termination if the length is 
    correct.

        'O''K', length 2
        "OKAY", length 2 will just use "OK"

    \note Maximum response length is 128 characters
    \note Interim commands such as this should eventually be followed
    by an OK (or ERROR)

    \see DeviceTestService_CommandResponseOk

    \param task The task information sent for the incoming command
    \param response The response to be sent.
    \param length The length of the response. 0 is allowed, in which case 
            the length of the string is used.
 */
void DeviceTestService_CommandResponse(Task task, const char *response, unsigned length);

/*! Send an OK response for the current command

    The task passed should match that sent in the function call.
    Only one OK or Error should be sent in response to a command.

    \see DeviceTestService_CommandResponseError

    \param task The task information sent for the incoming command
 */
void DeviceTestService_CommandResponseOk(Task task);


/*! Send an ERROR response for the current command

    The task passed should match that sent in the function call.
    Only one Ok or Error should be sent in response to a command.

    \see DeviceTestService_CommandResponseOk

    \param task The task information sent for the incoming command
 */
void DeviceTestService_CommandResponseError(Task task);


/*! Send an OK or ERROR response for the current command based on parameter

    The task passed should match that sent in the function call.
    Only one Ok or Error should be sent in response to a command.

   \see DeviceTestService_CommandResponseOk

   \param task The task information sent for the incoming command
   \param success Whether we are meant to be representing a success or not.
            TRUE ==> "OK", FALSE ==> "ERROR"
*/
void DeviceTestService_CommandResponseOkOrError(Task task, bool success);


/*! Start the device test service. It will autonomously try to 
    create a connection until stopped.

    A message \ref DEVICE_TEST_SERVICE_ENDED will be sent to the 
    app_task when the device test service is terminated. 

    The service can be terminated by
    \li a call to DeviceTestService_Stop
    \li the Device Test Service in response to a test command 

    \param app_task The task to receive messages from the device test
                    service
 */
void DeviceTestService_Start(Task app_task);


/*! Stop the device test service. Any existing session will terminate.

    When stopped, a DEVICE_TEST_SERVICE_ENDED message will be sent to
    the application task.

    \param app_task The task to receive messages from the device test
                    service. This must be the same as the task supplied
                    in the DeviceTestService_Start command. It is 
                    provided as a parameter here in case the service has
                    already stopped.
 */
void DeviceTestService_Stop(Task app_task);


/*! Report whether the device test service is active

    \return TRUE if the device test service is enabled and active (trying to
            connect or connected), FALSE otherwise
 */
bool DeviceTestService_IsActive(void);

#else
#define DeviceTestService_TestMode() FALSE
#define DeviceTestService_SaveTestMode(_mode) (UNUSED(_mode))

#define DeviceTestService_Start(_task) (UNUSED(_task))
#define DeviceTestService_Stop(_task)  (UNUSED(_task))

#define DeviceTestService_CommandResponse(_task, _response, _length) (UNUSED(_task), UNUSED(_response), UNUSED(_length))

#define DeviceTestService_CommandResponseOk(_task) (UNUSED(_task))
#define DeviceTestService_CommandResponseError(_task) (UNUSED(_task))
#define DeviceTestService_CommandResponseOkOrError(_task, _success) (UNUSED(_task), UNUSED(_success))

#define DeviceTestService_IsActive() FALSE

#endif /* INCLUDE_DEVICE_TEST_SERVICE */

#endif /* DEVICE_TEST_SERVICE_H */

/*! @} End of group documentation */

