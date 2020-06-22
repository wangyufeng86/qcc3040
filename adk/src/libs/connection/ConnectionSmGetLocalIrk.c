/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionSmGetLocalIrk.c        

DESCRIPTION
    This file is a stub for the management entity responsible retrieving 
    local Identity Resolving Key (IRK).

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

#include <message.h>
#include <string.h>
#include <vm.h>


/*****************************************************************************/
bool ConnectionSmGetLocalIrk(cl_irk *clirk)
{
    UNUSED(clirk);
    
    /* Not supported */    
    return FALSE;
}
