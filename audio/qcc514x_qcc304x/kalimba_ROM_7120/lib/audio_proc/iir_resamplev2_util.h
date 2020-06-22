/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file
 * \ingroup audio_proc
 * IIR RESAMPLER's constant tables is organised as a linked list
 */
#ifndef IIR_RESAMPLER_CONSTANT_TABLES_H
#define IIR_RESAMPLER_CONSTANT_TABLES_H

#include "types.h"

typedef struct IIR_TABLE_INSTANCE
{

    /** Pointer to table */
    void *internal_descriptor;

    /** external descriptor */
    void *external_descriptor;

    /** next item in the list */
    struct IIR_TABLE_INSTANCE *next;

} IIR_TABLE_INSTANCE;

/****************************************************************************
Public Function Definitions
*/

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
bool iir_resamplerv2_add_config_to_list(void *const_table);

/**
 *  Detele the IIR RESAMPLER configurations linked list.
 *  The configurations will still exist but the list pointing
 *  to them will be destroyed.
 */
void iir_resamplerv2_delete_config_list(void);

#endif /* IIR_RESAMPLER_CONSTANT_TABLES_H */
