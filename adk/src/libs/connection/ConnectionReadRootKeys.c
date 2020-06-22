/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionReadRootKeys.c        

DESCRIPTION
    This file contains the management entity responsible reading root keys

NOTES

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
bool ConnectionReadRootKeys(cl_root_keys* root_keys)
{
    DM_SM_KEY_STATE_T irer;
    
    if (PsRetrieve(
            PSKEY_SM_KEY_STATE_IR_ER,
            &irer,
            PS_SIZE_ADJ(sizeof(DM_SM_KEY_STATE_T))
            ))
    {
        /*! The local device root keys */
        memmove(root_keys->er, irer.er, sizeof(root_keys->er));
        memmove(root_keys->ir, irer.ir, sizeof(root_keys->ir));
        return TRUE;
    }
    else 
    {          
        memset(root_keys, 0, sizeof(root_keys));
        return FALSE;
    }                         
}
