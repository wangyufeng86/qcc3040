/*******************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.


FILE NAME
    config_test_debug.c

DESCRIPTION
    This library is for testing the library build configurations
*/

#include "config_test.h"
#include "config_test_private.h"

char* configTestDebug(void)
{
#ifdef CONFIG_TEST_CFLAGS
    return "Testing changing defines";
#else
    return "Default - no tests enabled";
#endif
}
