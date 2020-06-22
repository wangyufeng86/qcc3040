/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\ingroup device_test_service
\brief      File containing values that can be used to configure the device
            test service

@{
*/

#include <message.h>

#ifndef DEVICE_TEST_SERVICE_CONFIG_H
#define DEVICE_TEST_SERVICE_CONFIG_H

/*! Channel for RFComm that will be requested for the SPP channel.

    A value of 0 indicates that no channel is preferred and the libraries
    will allocate.

    Use of a fixed channel may be useful for debugging.
 */
#define DeviceTestService_RecommendedSppRfcommChannel() 0


/*! The PSKEY to use to determine if Device Test Mode is enabled
 */
#define DeviceTestService_EnablingPskey() 3


/*! The delay to use before rebooting a device when leaving test mode

    \note The configuration should have been saved at the point 
        this is called, so the value used can be quite short.
        Sufficient time should be allowed for the command response to 
        be sent back to the host.
 */
#define DeviceTestService_RebootTimeOutMs() D_SEC(2)

/*! The delay to use for rebooting a device when entering the case out of test mode

    \note The configuration should have been saved at the point 
        this is called, so the value used can be quite short.
 */
#define DeviceTestService_InCaseRebootTimeOutMs() 200

/*! Delay to use for cleaning the device database after boot

    If testing is ended with a reboot, then there may not have been an 
    opportunity to correctly clean up, as the entries cannot be removed
    if the entry is for a current connection.

    This is only called on boot so has limited overhead.
 */
#define DeviceTestService_CleanupTimeOutMs() D_SEC(2)


#endif /* DEVICE_TEST_SERVICE_CONFIG_H */

/*! @} End of group documentation */
