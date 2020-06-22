/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  iir_resampler_util.c
 * \ingroup  audio_proc
 *
 * a few utilities for iir_resamplerv2 algorithm.
 *
 */

/****************************************************************************
Include Files
*/
#include "pmalloc/pl_malloc.h"
#include "iir_resamplev2_util.h"
#include "mem_utils/exported_constant_files.h"

IIR_TABLE_INSTANCE * iir_resamplev2_head;
unsigned iir_resamplev2_num_users = 0;

static bool add_table_to_list(void* const_table);

/**
 *  The utility registers the interest of capability that made the call
 *  in the constant table list holding the IIR RESAMPLER configurations.
 *  First, we attempt to add two default tables to this list:
 *  iir_resamplev2_DynamicMemDynTable_Main
 *  iir_resamplev2_DynamicMemLowMipsDynTable_Main
 *  If const_table passed in isn't NULL, it will be added at the end of the list.
 *
 * \param const_table - table of resampler configurations
 * \return True on success, False on failure
 */
bool iir_resamplerv2_add_config_to_list(void *const_table)
{

    if (!add_table_to_list(iir_resamplev2_DynamicMemDynTable_Main))
    {
        return(FALSE);
    }
    if (!add_table_to_list(iir_resamplev2_DynamicMemLowMipsDynTable_Main))
    {
        return(FALSE);
    }
    if (const_table != NULL)
    {
        if (!add_table_to_list(const_table))
        {
            return(FALSE);
        }
    }

    iir_resamplev2_num_users += 1;

    return TRUE;
}

/**
 *  Detele the IIR RESAMPLER configurations linked list.
 *  The configurations will still exist but the list pointing
 *  to them will be destroyed.
 */
void iir_resamplerv2_delete_config_list(void)
{
    IIR_TABLE_INSTANCE *current_elem = iir_resamplev2_head;
    IIR_TABLE_INSTANCE *next_elem = NULL;

    iir_resamplev2_num_users -= 1;
    if(iir_resamplev2_num_users == 0)
    {
        /* free all elements in the list */
        while(current_elem != NULL)
        {
            next_elem = current_elem->next;
            pfree(current_elem);
            current_elem = next_elem;
        }
        /* clear the head of the list */
        iir_resamplev2_head = NULL;
    }

}

static bool add_table_to_list(void* const_table)
{
    IIR_TABLE_INSTANCE *current_elem = iir_resamplev2_head;
    IIR_TABLE_INSTANCE *prev_elem = iir_resamplev2_head;
    IIR_TABLE_INSTANCE *new_elem;

    bool table_found = FALSE;

    /* traverse the list searching for the entry passed in */
    while(current_elem != NULL)
    {
        prev_elem = current_elem;
        if(current_elem->internal_descriptor == const_table)
        {
            table_found = TRUE;
            break;
        }
        current_elem = current_elem->next;
    }
    /* const_table doesn't exist in the list - add it below */
    if(!table_found)
    {
        new_elem = xzpmalloc(sizeof(IIR_TABLE_INSTANCE));
        /* fail if we cannot allocate memory for current entry */
        if(new_elem == NULL)
        {
            return FALSE;
        }
        new_elem->internal_descriptor = const_table;
        new_elem->external_descriptor = NULL;
        new_elem->next = NULL;
        if(iir_resamplev2_head == NULL)
        {
            iir_resamplev2_head = new_elem;
        }
        else
        {
            prev_elem->next = new_elem;
        }
    }
    return TRUE;
}