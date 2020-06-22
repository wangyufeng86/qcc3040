/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionSetRootKeys.c

DESCRIPTION
    This file is a stub for the management entity responsible for updating 
    root keys

NOTES
    Builds requiring this should include CONFIG_KEY_MANIPULATION in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_KEY_MANIPULATION
*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"

#include    <message.h>
#include    <string.h>
#include    <vm.h>
#include    <ps.h>

/*****************************************************************************/
bool ConnectionSetRootKeys(cl_root_keys* root_keys)
{
    UNUSED(root_keys);

    /* Not supported */    
    return FALSE;
}


