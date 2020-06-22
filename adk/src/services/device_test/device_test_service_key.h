/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\ingroup    device_test_service

\brief      Header file for the Device Test Service secret key.

The Device Test Service secret key is a key that must be known to users wishing to
authenticate for access.
The secret key is held in device_test_service_auth_key which is compiled 
into the application.
*/

#ifndef __DEVICE_TEST_SERVICE_KEY_H__
#define __DEVICE_TEST_SERVICE_KEY_H__


#include <device_test_service_data.h>

extern const DTS_AUTH_KEY_T device_test_service_auth_key;


#endif  /* __DEVICE_TEST_SERVICE_KEY_H__ */
