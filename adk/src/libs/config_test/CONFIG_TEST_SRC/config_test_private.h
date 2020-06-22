/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file   config_test_private.h
@brief  Private header file for the config_test library.

        This library is for testing the library build configurations
*/

#ifndef CONFIG_TEST_PRIVATE_H_
#define CONFIG_TEST_PRIVATE_H_

char* configTestDebug_TEST_SRC(void);

#define configTestDebug() configTestDebug_TEST_SRC()

#endif /*CONFIG_TEST_PRIVATE_H_*/
