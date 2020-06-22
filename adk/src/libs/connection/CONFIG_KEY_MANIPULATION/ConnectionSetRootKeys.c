/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    ConnectionSetRootKeys.c

DESCRIPTION
    This file contains the management entity responsible for updating root keys

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
/*! 
    @brief This function updates Updates ER (Encryption Root) 
    and IR (Identity Root) root key values of the device.

    @return 
*/
bool ConnectionSetRootKeys(cl_root_keys* root_keys)
{
    packed_root_keys rtks;
    DM_SM_KEY_STATE_T irer;

    if(!root_keys)
        return FALSE;

    memmove(&rtks.er, root_keys->er, sizeof(rtks.er));
    memmove(&rtks.ir, root_keys->ir, sizeof(rtks.ir));
    memmove(&irer.er, root_keys->er, sizeof(irer.er));
    memmove(&irer.ir, root_keys->ir, sizeof(irer.ir));

    if(VmUpdateRootKeys(&rtks))
    {
        return PsStore( PSKEY_SM_KEY_STATE_IR_ER,
                        &irer,
                        PS_SIZE_ADJ(sizeof(DM_SM_KEY_STATE_T))
                        );

    }

    return FALSE;
}


