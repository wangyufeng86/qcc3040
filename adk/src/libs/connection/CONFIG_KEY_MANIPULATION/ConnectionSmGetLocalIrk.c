/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionSmGetLocalIrk.c        

DESCRIPTION
    This file contains the management entity responsible retrieving 
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
/*! 
    @brief This function retrieves the local Identity Resolving Key (IRK).
    @return true if successfully retrieved the IRK
*/
bool ConnectionSmGetLocalIrk(cl_irk *clirk)
{
    packed_irk pirk;

    if(clirk == NULL)
        return FALSE;

    if(VmGetLocalIrk(&pirk))
    {
        memmove(&clirk->irk, &pirk, sizeof(packed_irk));
        return TRUE;
    }
    
    return FALSE;
}
