/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionSmSetLocalBdaddr.c        

DESCRIPTION
    This file contains the management entity for overriding the local BDADDR

NOTES

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
bool ConnectionSmSetLocalBdaddr(const bdaddr* bd_addr)
{
    if(!bd_addr)
        return FALSE;
        
    return VmOverrideBdaddr(bd_addr);
}
