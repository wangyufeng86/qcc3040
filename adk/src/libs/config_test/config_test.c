/*******************************************************************************
Copyright (c) 2015 - 2016 Qualcomm Technologies International, Ltd.


FILE NAME
    config_test.c

DESCRIPTION
    This library is for testing the library build configurations
*/

#include <config_test.h>
#include <config_test_private.h>

#include <stdio.h>

char* ConfigTest(void)
{
    printf(configTestDebug());
    
    return configTestDebug();
}
