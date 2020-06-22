/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file hydra_cbuff.c
 * \ingroup buffer
 *
 * cbuffer 'C' code.
 */

/****************************************************************************
Include Files
*/

#include "buffer_private.h"
#include "hydra_cbuff.h"
#include "platform/pl_trace.h"



/****************************************************************************
Private Functions
*/
void cbuffer_advance_read_ptr_mmu_buff(tCbuffer *, int);
void cbuffer_advance_write_ptr_mmu_buff(tCbuffer *, int);

/*
 * \brief Maps circular buffer descriptor flags to mmu flags.
 *  packing not supported.
 * \param cb_desc circular buffer descriptor
 * \param get_wr_flags If set, get write flags else get read flags
 * \return mmu flags
 */
static inline unsigned cb_descriptor_to_mmu_flags(unsigned cb_desc, bool get_wr_flags)
{
    unsigned mmu_flags = get_wr_flags ? CBUF_TO_MMU_WR_PROT_FLAGS(cb_desc) : CBUF_TO_MMU_RD_PROT_FLAGS(cb_desc);

    return mmu_flags | CBUF_TO_MMU_SAMP_SIZE_FLAGS(cb_desc);
}

/*
 * \brief Helper function to populate cbuffer fields.
 *  Used by mmu_buffer_create and mmu_buffer_wrap.
 *  Clones write or aux handles as necessary.
 * \param cbuffer_struc_ptr pointer to cbuffer structure that needs filling up
 * \param handle valid mmu handle
 * \param flags cbuffer descriptor
 * \param buffer_size buffer size in chars
 */
static void create_mmu_buffer_common(tCbuffer *cbuffer_struc_ptr, mmu_handle handle,
                                unsigned int flags, unsigned int buffer_size, int *base_addr)
{
    audio_buf_handle_struc *temp_ptr; /* Pointer to the (first) buffer handle we create */

    patch_fn_shared(hydra_cbuff_create);

    /* buffer_size to cbuffer internal units */
    cbuffer_struc_ptr->size = buffer_size;

    /* Get hold of the buffer handle pointer and set up the base address */
    temp_ptr = mmu_buffer_get_handle_ptr_from_idx(handle.index);

    cbuffer_struc_ptr->base_addr = base_addr;

    if(BUF_DESC_RD_PTR_TYPE_MMU(flags) && !BUF_DESC_WR_PTR_TYPE_MMU(flags))
    {
        /* Read handle is MMU, Write handle is SW */
        cbuffer_struc_ptr->read_ptr = (int*)temp_ptr;
        cbuffer_struc_ptr->write_ptr = cbuffer_struc_ptr->base_addr;
    }
    else if (!BUF_DESC_RD_PTR_TYPE_MMU(flags) && BUF_DESC_WR_PTR_TYPE_MMU(flags))
    {
        /* Read handle is SW, Write handle is MMU */
        cbuffer_struc_ptr->read_ptr = cbuffer_struc_ptr->base_addr;
        cbuffer_struc_ptr->write_ptr = (int*)temp_ptr;
    }
    else
    {
        /* The complicated case: both handles are MMU.
         * Use the mmu_handle we have as the read handle, and clone it
         * to use as the write handle.
         */
        unsigned mmu_flags;
        mmu_handle wr_handle = mmu_buffer_clone_handle(handle);
        if (mmu_index_is_null(wr_handle.index))
        {
            /* We must have run out of buffer handles. */
            panic_diatribe(PANIC_AUDIO_BUFFER_HANDLES_EXHAUSTED, (DIATRIBE_TYPE)((uintptr_t)cbuffer_struc_ptr));
        }

        mmu_flags = cb_descriptor_to_mmu_flags(flags, TRUE);

        /* Set the appropriate flags for the write handle */
        mmu_buffer_set_flags(wr_handle, mmu_flags);

        /* make the write pointer point to the new buffer handle */
        cbuffer_struc_ptr->read_ptr = (int *)temp_ptr;
        cbuffer_struc_ptr->write_ptr = (int *)mmu_buffer_get_handle_ptr_from_idx(wr_handle.index);

        /* Now sort out the aux buffer, if necessary. */
        if (BUF_DESC_AUX_PTR_PRESENT(flags))
        {
            /* Clone the mmu_handle again */
            mmu_handle aux_handle = mmu_buffer_clone_handle(handle);
            if (mmu_index_is_null(aux_handle.index))
            {
                /* We must have run out of buffer handles. */
                panic_diatribe(PANIC_AUDIO_BUFFER_HANDLES_EXHAUSTED, (DIATRIBE_TYPE)((uintptr_t)cbuffer_struc_ptr));
            }

            /* Set the appropriate flags */
            if(BUF_DESC_AUX_PTR_TYPE(flags))
            {
                mmu_buffer_set_flags(aux_handle, cb_descriptor_to_mmu_flags(flags, TRUE));
            }
            else
            {
                mmu_buffer_set_flags(aux_handle, cb_descriptor_to_mmu_flags(flags, FALSE));
            }

            /* make the aux pointer point to the new buffer handle */
            cbuffer_struc_ptr->aux_ptr = (int *)mmu_buffer_get_handle_ptr_from_idx(aux_handle.index);
        }
    }

    /* Store the flags. */
    cbuffer_struc_ptr->descriptor = flags & BUF_DESC_STICKY_FLAGS_MASK;
}

/****************************************************************************
 *
 * cbuffer_create_mmu_buffer_preference - used to create local MMU buffers
 *
 * The buffer data space will be allocated inside the function
 *
 * Input arguments:
 *      flags               - aggregated flags referring to read_ptr, write_ptr and aux_ptr
 *      buffer_size         - no of words for local MMU
 *      preference          - MALLOC_PREFERENCE to allocate the data memory from
 *
 * Return value
 *      If successful - pointer to the created cbuffer structure else NULL
 *
 */
static tCbuffer *cbuffer_create_mmu_buffer_preference(unsigned int flags, unsigned int buffer_size, unsigned preference)
{
    tCbuffer *cbuffer_struc_ptr;
    mmu_handle handle; /* (First) mmu handle we create */
    unsigned buffer_size_in_octets;
    bool get_wr_flags;
    int *buffer;
    unsigned usable_octets;

    if(!BUF_DESC_BUFFER_TYPE_MMU(flags))
    {
        /* This combination is effectively the same as cbuffer_create_with_malloc,
         * but for the convenience of other modules we permit it. */
        return cbuffer_create_with_malloc_preference(buffer_size, BUF_DESC_SW_BUFFER, MALLOC_PREFERENCE_NONE, preference);
    }

    patch_fn_shared(hydra_cbuff_create);

    cbuffer_struc_ptr = zpnew(tCbuffer); /* Could panic */

    buffer_size_in_octets = SAMPLES_TO_BAC_OFFSET(buffer_size, CBUF_TO_MMU_SAMP_SIZE_FLAGS(flags));

    /* If both RD and WR are MMU, get RD flags and generate RD handle first */
    get_wr_flags = (!BUF_DESC_RD_PTR_TYPE_MMU(flags) && BUF_DESC_WR_PTR_TYPE_MMU(flags));
    if ((buffer = mmu_buffer_create(&buffer_size_in_octets, &handle, cb_descriptor_to_mmu_flags(flags, get_wr_flags), preference)) == NULL)
    {
        /* Pass on the failure */
        pdelete(cbuffer_struc_ptr);
        return NULL;
    }

    create_mmu_buffer_common(cbuffer_struc_ptr, handle, flags, BAC_OFFSET_TO_CHARS(buffer_size_in_octets, CBUF_TO_MMU_SAMP_SIZE_FLAGS(flags)), buffer);

    /* for Hydra (BAC32) we have control of sample_size in the HW
        this should match the usable_octets in the descriptor */
    usable_octets = BUF_DESC_BAC32_TO_USABLE_OCTETS( (((flags) & BUF_DESC_SAMP_SIZE_MASK) >> BUF_DESC_SAMP_SIZE_SHIFT) );
    cbuffer_set_usable_octets(cbuffer_struc_ptr, usable_octets);
    
    PL_PRINT_P0(TR_CBUFFER, "cbuffer_create_mmu_buffer: ");
    PL_PRINT_BUFFER(TR_CBUFFER, cbuffer_struc_ptr);
    return cbuffer_struc_ptr;
}

/****************************************************************************
 *
 * cbuffer_create_mmu_buffer - used to create local MMU buffers
 *
 * The buffer data space will be allocated from SLOW RAM inside the function
 *
 * Input arguments:
 *      flags               - aggregated flags referring to read_ptr, write_ptr and aux_ptr
 *      buffer_size         - no of words for local MMU
 *
 * Return value
 *      If successful - pointer to the created cbuffer structure else NULL
 *
 */
tCbuffer *cbuffer_create_mmu_buffer(unsigned int flags, unsigned int buffer_size)
{
    return cbuffer_create_mmu_buffer_preference(flags, buffer_size, MALLOC_PREFERENCE_NONE);
}

/****************************************************************************
 *
 * cbuffer_create_mmu_buffer_fast - used to create local MMU buffers
 *
 * The buffer data space will be allocated from FAST RAM inside the function
 *
 * Input arguments:
 *      flags               - aggregated flags referring to read_ptr, write_ptr and aux_ptr
 *      buffer_size         - no of words for local MMU
 *
 * Return value
 *      If successful - pointer to the created cbuffer structure else NULL
 *
 */
tCbuffer *cbuffer_create_mmu_buffer_fast(unsigned int flags, unsigned int buffer_size)
{
    return cbuffer_create_mmu_buffer_preference(flags, buffer_size, MALLOC_PREFERENCE_FAST);
}

/****************************************************************************
 *
 * cbuffer_wrap_remote - used for wrapping remote MMU buffers in cbuffer structures
 *
 * Input arguments:
 *      flags                   - aggregated flags referring to permissions to modify read and write handles
 *      remote_handle1          - read MMU handle (as understood by hydra code)
 *      remote_handle2          - write MMU handle (as understood by hydra code)
 *      buffer_size             - remote MMU buffer size
 *
 * Return value
 *      If successful - pointer to the created cbuffer structure else NULL
 *
 */
tCbuffer *cbuffer_wrap_remote(int flags, mmu_handle read_handle, mmu_handle write_handle, unsigned int buffer_size)
{
    tCbuffer *cbuffer_struc_ptr;

    patch_fn_shared(hydra_cbuff_create);

    cbuffer_struc_ptr = zpnew(tCbuffer); /* Could panic */
    cbuffer_struc_ptr->read_ptr = (int *)(uintptr_t)mmu_handle_pack(read_handle);
    cbuffer_struc_ptr->write_ptr = (int *)(uintptr_t)mmu_handle_pack(write_handle);
    cbuffer_struc_ptr->size = buffer_size;
    cbuffer_struc_ptr->descriptor = flags;

    PL_PRINT_P0(TR_CBUFFER, "cbuffer_wrap_remote: ");
    PL_PRINT_BUFFER(TR_CBUFFER, cbuffer_struc_ptr);
    return cbuffer_struc_ptr;
}



/*
 *    Buffer
 *  |------- |                                     |
 *  |        |                                     |   amount space
 *  |--------|   <- write_offset   ^               v
 *  |########|                     | amount data
 *  |########|                     |
 *  |--------|   <- read_offset    v               ^
 *  |        |                                     |
 *  |--------|                                     |
*/


/****************************************************************************
 *
 * cbuffer_calc_amount_data_remote_buff -
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure to read from
 *
 * Return value
 *      Amount of data in the buffer, in words.
 *
 * NB. This is only used for remote buffers.
 */
unsigned int cbuffer_calc_amount_data_remote_buff(tCbuffer *cbuffer)
{
    int ret;
    mmu_handle rd_handle, wr_handle;
    int rd_offset, wr_offset;    /* they are naturally unsigned, this is one way
                                     to make sure their difference is signed */
    patch_fn_shared(hydra_cbuff_remote);

    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor))
    {
        mmu_handle_unpack(cbuffer->read_ptr, &rd_handle);
        mmu_handle_unpack(cbuffer->write_ptr, &wr_handle);

        rd_offset = mmu_buffer_get_handle_offset(rd_handle);
        wr_offset = mmu_buffer_get_handle_offset(wr_handle);

        /* Amount of data is write offset - read offset (see sketch above). If
         * the result is negative the write offset wrapped and the buffer size
         * needs to be added to the result.
         * Careful: the offsets are in octets, so we need to convert to words. */
        ret = buffer_octets_to_words(wr_offset - rd_offset);  /* must be signed subtraction */
        if (ret < 0)
        {
            ret += cbuffer->size;
        }
    }
    else
    {
        /* Shouldn't use the _remote_buff methods for local buffers. */
        fault_diatribe(FAULT_AUDIO_UNSUPPORTED, (DIATRIBE_TYPE)((uintptr_t)cbuffer));
        return 0;
    }

    return ret;
}



/****************************************************************************
 *
 * cbuffer_calc_amount_space_remote_buff
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure to read from
 *
 * Return value
 *      Amount of space in the buffer, in words.
 *
 * NB. This is only used for remote buffers.
 */
unsigned int cbuffer_calc_amount_space_remote_buff(tCbuffer *cbuffer)
{
    /* Amount of space in buffer is buffer size - amount_data */
    unsigned int space = cbuffer->size - cbuffer_calc_amount_data_remote_buff(cbuffer);

    /* Always say it's 1 word less so that buffer never gets totally filled up */
    return (space - 1);
}


/**/
void cbuffer_advance_read_ptr_mmu_buff(tCbuffer *cbuffer, int amount)
{
    patch_fn_shared(hydra_cbuff_modify);

    /* If the buffer is remote, first check that we're allowed to modify the read handle. */
    if( BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) && !BUF_DESC_REMOTE_RDH_MOD(cbuffer->descriptor))
    {
        panic_diatribe(PANIC_AUDIO_MODIFY_ATTEMPT_ON_PROTECTED_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer));
    }

    /* Call into mmu_buffer if the handle is controlled by the MMU,
          could be remote or local HW buffer */
    if( BUF_DESC_REMOTE_RDH_MOD(cbuffer->descriptor) ||
        BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;
        int offset;
        unsigned int buffer_size_octets;
        /* Get hold of the current read offset */
        if (BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);
        }
        else
        {
            mmu_handle_unpack(cbuffer->read_ptr, &handle);
        }

        offset = mmu_buffer_get_handle_offset(handle); /* offset in octets */

        /* Increment, accounting for wrapping.
         * (The bitwise-and only works because our buffer size is
         * currently always a power of two) */
        buffer_size_octets = CHARS_TO_BAC_OFFSET(cbuffer->size, mmu_buffer_get_flags(handle));
        offset += SAMPLES_TO_BAC_OFFSET(amount, mmu_buffer_get_flags(handle));
        offset &= (buffer_size_octets - 1);
        mmu_buffer_set_handle_offset(handle, offset);
    }
    else
    {
        /* Shouldn't use the _mmu_buff methods for local SW buffers */
        fault_diatribe(FAULT_AUDIO_UNSUPPORTED, (DIATRIBE_TYPE)((uintptr_t)cbuffer->read_ptr));
    }
}


/**/
void cbuffer_advance_write_ptr_mmu_buff(tCbuffer *cbuffer, int amount)
{
    patch_fn_shared(hydra_cbuff_modify);

    /* If the buffer is remote, first check that we're allowed to modify the write handle. */
    if( BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) && !BUF_DESC_REMOTE_WRH_MOD(cbuffer->descriptor))
    {
        panic_diatribe(PANIC_AUDIO_MODIFY_ATTEMPT_ON_PROTECTED_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer));
    }

    /* Call into mmu_buffer if the handle is controlled by the MMU,
          could be remote or local HW buffer */
    if( BUF_DESC_REMOTE_WRH_MOD(cbuffer->descriptor) ||
        BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;
        int offset;
        unsigned int buffer_size_octets;
        /* Get hold of the current write offset */
        if (BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);
        }
        else
        {
            mmu_handle_unpack(cbuffer->write_ptr, &handle);
        }

        offset = mmu_buffer_get_handle_offset(handle); /* offset in octets */

        /* Increment, accounting for wrapping.
         * (The bitwise-and only works because our buffer size is
         * currently always a power of two) TODO could compare and wrap round if over (also _read_)
         */
        buffer_size_octets = CHARS_TO_BAC_OFFSET(cbuffer->size, mmu_buffer_get_flags(handle));
        offset += SAMPLES_TO_BAC_OFFSET(amount, mmu_buffer_get_flags(handle)); /* amount is in words */
        offset &= (buffer_size_octets - 1);

        mmu_buffer_set_handle_offset(handle, offset);
    }
    else
    {
        /* Shouldn't use the _mmu_buff methods for SW buffers */
        fault_diatribe(FAULT_AUDIO_UNSUPPORTED, (DIATRIBE_TYPE)((uintptr_t)cbuffer->write_ptr));
    }
}



/****************************************************************************
 *
 * cbuffer_get_read_mmu_handle - returns read mmu handle
 *
 * Input argument:
 *      cbuffer   - pointer to the cbuffer structure to query
 *
 * Return value
 *      constructed mmu_handle or empty mmu_handle if rd_ptr does not point to MMU buffer handle
 *
 */
mmu_handle cbuffer_get_read_mmu_handle(tCbuffer *cbuffer)
{
    mmu_handle ret_handle = MMU_HANDLE_NULL;

    if( BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) )
    {
        mmu_handle_unpack(cbuffer->read_ptr, &ret_handle);
    }
    else
    {
        if(BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            ret_handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);
        }
    }
    return ret_handle;
}


/****************************************************************************
 *
 * cbuffer_get_write_mmu_handle - returns write mmu handle
 *
 * Input argument:
 *      cbuffer   - pointer to the cbuffer structure to query
 *
 * Return value
 *      constructed mmu_handle or empty mmu_handle if wr_ptr does not point to MMU buffer handle
 *
 */
mmu_handle cbuffer_get_write_mmu_handle(tCbuffer *cbuffer)
{
    mmu_handle ret_handle = MMU_HANDLE_NULL;

    if( BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) )
    {
        mmu_handle_unpack(cbuffer->write_ptr, &ret_handle);
    }
    else
    {
        if(BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            ret_handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);
        }
    }
    return ret_handle;
}

/****************************************************************************
 *
 * cbuffer_get_aux_mmu_handle - returns the aux mmu handle, if present
 *
 * Input argument:
 *      cbuffer   - pointer to the cbuffer structure to query
 *
 * Return value
 *      constructed mmu_handle or empty mmu_handle if wr_ptr does not point to MMU buffer handle
 *
 */
mmu_handle cbuffer_get_aux_mmu_handle(tCbuffer *cbuffer)
{
    mmu_handle ret_handle = MMU_HANDLE_NULL;

    if( BUF_DESC_AUX_PTR_PRESENT(cbuffer->descriptor) )
    {
        ret_handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);
    }
    return ret_handle;
}

/**/
bool cbuffer_is_read_handle_mmu(tCbuffer *cbuffer)
{
    return BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor);
}

bool cbuffer_is_write_handle_mmu(tCbuffer *cbuffer)
{
    return BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor);
}

bool cbuffer_is_remote(tCbuffer *cbuffer)
{
    return BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor);
}

/* Return BUFF_AUX_PTR_TYPE_NONE if there is no aux handle,
 * BUFF_AUX_PTR_TYPE_READ if there is an aux handle used for read,
 * BUFF_AUX_PTR_TYPE_WRITE if there is an aux handle used for write.
 */
int cbuffer_aux_in_use(tCbuffer *cbuffer)
{
    if( BUF_DESC_AUX_PTR_PRESENT(cbuffer->descriptor) )
    {
        return (BUF_DESC_AUX_PTR_TYPE(cbuffer->descriptor)) ?
                                                BUFF_AUX_PTR_TYPE_WRITE :
                                                BUFF_AUX_PTR_TYPE_READ;
    }
    else
    {
        return BUFF_AUX_PTR_TYPE_NONE;
    }
}

/****************************************************************************
 *
 * cbuffer_set_read_shift - set the shift amount for the read mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      amount    - size of shift
 *
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_read_shift(tCbuffer *cbuffer, int amount)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set shift on a local MMU handle */
        return FALSE;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

    /* Set up the shift for the read handle */
    flags = mmu_buffer_get_flags(handle);
    MMU_BUF_SHIFT_SET(flags, amount);
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a read handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same shift for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        MMU_BUF_SHIFT_SET(flags, amount);
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_write_shift - set the shift amount for the write mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      amount    - size of shift
 *
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_write_shift(tCbuffer *cbuffer, int amount)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Write handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set shift on a local MMU handle */
        return FALSE;
    }

    /* Get the write handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

    /* Set up the shift for the write handle */
    flags = mmu_buffer_get_flags(handle);
    MMU_BUF_SHIFT_SET(flags, amount);
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a write handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same shift for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        MMU_BUF_SHIFT_SET(flags, amount);
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_read_byte_swap - set byte swap for the read mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 *
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_read_byte_swap(tCbuffer *cbuffer, bool set)
{
    patch_fn_shared(hydra_cbuff_modify);

    /* We can set a byte swap for remote MMU, but the process is completely
     * different from configuring a local buffer. We need to remember that
     * the buffer is to be byte-swapped, and set the appropriate flag when
      * performing a read. */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) && BUF_DESC_REMOTE_RDH_MOD(cbuffer->descriptor))
    {
        if (set)
        {
            BUF_DESC_REMOTE_RDH_BSWAP_SET(cbuffer->descriptor);
        }
        else
        {
            BUF_DESC_REMOTE_RDH_BSWAP_UNSET(cbuffer->descriptor);
        }

        return TRUE;
    }

    /* Is the read handle a local MMU buffer ?*/
    if (BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        unsigned flags;
        mmu_handle handle;

        /* Get the read handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

        /* Set up the shift for the read handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_BSWAP_SET(flags);
        }
        else
        {
            MMU_BUF_BSWAP_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);

        /* If the aux handle is in use as a read handle */
        if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ)
        {
            /* Get the aux handle */
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

            /* Set up the same shift for the aux handle */
            flags = mmu_buffer_get_flags(handle);
            if (set)
            {
                MMU_BUF_BSWAP_SET(flags);
            }
            else
            {
                MMU_BUF_BSWAP_UNSET(flags);
            }
            mmu_buffer_set_flags(handle, flags);
        }

        /* Succeeded */
        return TRUE;
    }

    /* If we got here, the buffer was either pure SW, or a remote buffer
     * not configured for read */
    return FALSE;
}

/****************************************************************************
 *
 * cbuffer_set_write_byte_swap - set byte swap for the write mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_write_byte_swap(tCbuffer *cbuffer, bool set)
{
    patch_fn_shared(hydra_cbuff_modify);

    /* We can set a byte swap for remote MMU, but the process is completely
     * different from configuring a local buffer. We need to remember that
     * the buffer is to be byte-swapped, and set the appropriate flag when
      * performing a write. */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) && BUF_DESC_REMOTE_WRH_MOD(cbuffer->descriptor))
    {
        if (set)
        {
            BUF_DESC_REMOTE_WRH_BSWAP_SET(cbuffer->descriptor);
        }
        else
        {
            BUF_DESC_REMOTE_WRH_BSWAP_UNSET(cbuffer->descriptor);
        }
        return TRUE;
    }

    /* Is the write handle a local MMU buffer ?*/
    if (BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        unsigned flags;
        mmu_handle handle;

        /* Get the write handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

        /* Set up the shift for the write handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_BSWAP_SET(flags);
        }
        else
        {
            MMU_BUF_BSWAP_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);

        /* If the aux handle is in use as a write handle */
        if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE)
        {
            /* Get the aux handle */
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

            /* Set up the same shift for the aux handle */
            flags = mmu_buffer_get_flags(handle);
            if (set)
            {
                MMU_BUF_BSWAP_SET(flags);
            }
            else
            {
                MMU_BUF_BSWAP_UNSET(flags);
            }
            mmu_buffer_set_flags(handle, flags);
        }

        /* Succeeded */
        return TRUE;
    }

    /* If we got here, the buffer was either pure SW, or a remote buffer
     * not configured for write */
    return FALSE;
}

/****************************************************************************
 *
 * cbuffer_get_read_shift - get the shift amount for the read mmu handle
 *
 * Input arguments:
 *      cbuffer  pointer to a cbuffer structure
 *
 * Return value
 *      size of shift currently configured.
 *
 */
unsigned int cbuffer_get_read_shift(tCbuffer *cbuffer)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set shift on a local MMU handle */
        return 0;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

    /* Get up the shift amount for the read handle */
    flags = mmu_buffer_get_flags(handle);
    return (unsigned int)MMU_BUF_SHIFT_GET(flags);
}

/****************************************************************************
 *
 * cbuffer_get_write_shift - get the shift amount for the write mmu handle
 *
 * Input arguments:
 *      cbuffer  pointer to a cbuffer structure
 *
 * Return value
 *      size of shift currently configured.
 *
 */
unsigned int cbuffer_get_write_shift(tCbuffer *cbuffer)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set shift on a local MMU handle */
        return 0;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

    /* Get up the shift amount for the read handle */
    flags = mmu_buffer_get_flags(handle);
    return (unsigned int)MMU_BUF_SHIFT_GET(flags);
}

/****************************************************************************
 *
 * cbuffer_get_read_byte_swap - get the byte swap for the read mmu handle
 *
 * Input arguments:
 *      cbuffer - pointer to a cbuffer structure
 *
 * Return value
 *      byte swap currently configured (TRUE or FALSE).
 *
 */
bool cbuffer_get_read_byte_swap(tCbuffer *cbuffer)
{
    bool byte_swap = FALSE;
    patch_fn_shared(hydra_cbuff_modify);

    /* Is the read handle for a remote MMU buffer? */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor))
    {
        if (BUF_DESC_REMOTE_RDH_MOD(cbuffer->descriptor))
        {
            byte_swap = BUF_DESC_REMOTE_RDH_BSWAP(cbuffer->descriptor) ? TRUE : FALSE;
        }
        else
        {
            panic_diatribe(PANIC_AUDIO_INVALID_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer->descriptor));
        }
    }
    else
    {
        /* Is the read handle for a local MMU buffer? */
        if (BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            unsigned flags;
            mmu_handle handle;

            /* Get the read handle */
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

            /* Get the byte swap for the given read handle */
            flags = mmu_buffer_get_flags(handle);
            byte_swap = MMU_BUF_BSWAP(flags) ? TRUE : FALSE;
        }
        else
        {
            panic_diatribe(PANIC_AUDIO_INVALID_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer->descriptor));
        }
    }

    return byte_swap;
}

/****************************************************************************
 *
 * cbuffer_get_write_byte_swap - get the byte swap for the write mmu handle
 *
 * Input arguments:
 *      cbuffer - pointer to a cbuffer structure
 *
 * Return value
 *      byte swap currently configured (TRUE or FALSE).
 *
 */
bool cbuffer_get_write_byte_swap(tCbuffer *cbuffer)
{
    bool byte_swap = FALSE;
    patch_fn_shared(hydra_cbuff_modify);

    /* Is the write handle for a remote MMU buffer? */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor))
    {
        if (BUF_DESC_REMOTE_WRH_MOD(cbuffer->descriptor))
        {
            byte_swap = BUF_DESC_REMOTE_WRH_BSWAP(cbuffer->descriptor) ? TRUE : FALSE;
        }
        else
        {
            panic_diatribe(PANIC_AUDIO_INVALID_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer->descriptor));
        }
    }
    else
    {
        /* Is the write handle for a local MMU buffer? */
        if (BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            unsigned flags;
            mmu_handle handle;

            /* Get the write handle */
            handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

            /* Get the byte swap for the given write handle */
            flags = mmu_buffer_get_flags(handle);
            byte_swap = MMU_BUF_BSWAP(flags) ? TRUE : FALSE;
        }
        else
        {
            panic_diatribe(PANIC_AUDIO_INVALID_MMU_HANDLE, (DIATRIBE_TYPE)((uintptr_t)cbuffer->descriptor));
        }
    }

    return byte_swap;
}

/****************************************************************************
 *
 * cbuffer_get_read_mmu_offset - gets the offset into the buffer of the read pointer.
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *
 * Return value
 *      read offset for the supplied cbuffer, in words
 */
unsigned int cbuffer_get_read_mmu_offset(tCbuffer *cbuffer)
{
    int *read_ptr;

    /* OBSOLETE cbuffer_get_read_address_and_size(cbuffer, &read_ptr, &buffer_size); */

    patch_fn_shared(hydra_cbuff_offset);

    read_ptr = cbuffer->read_ptr;

    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;

        mmu_handle_unpack(read_ptr, &handle);
        return buffer_octets_to_words( mmu_buffer_get_handle_offset(handle) );
    }
    else if (BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;

        /* Extract the (local) read handle information from the cbuffer.
         * Could be using an aux handle. */
        if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ )
        {
            read_ptr = cbuffer->aux_ptr;
        }
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)read_ptr);
        return BAC_OFFSET_TO_SAMPLES(mmu_buffer_get_handle_offset(handle), mmu_buffer_get_flags(handle));
    }
    else
    {
        /* otherwise must be SW buffer (although shouldn't use offsets with pure SW buffers)
           more likely a rd handle of a HW WR buffer */
        /* Difference in pointers needs to be divided by number of memory locations */
        return ((uintptr_t)read_ptr - (uintptr_t)cbuffer->base_addr)/sizeof(unsigned int);
    }
}


/****************************************************************************
 *
 * cbuffer_get_write_mmu_offset - gets the offset into the buffer of the write pointer.
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *
 * Return value
 *      write offset for the supplied cbuffer, in words
 */
unsigned int cbuffer_get_write_mmu_offset(tCbuffer *cbuffer)
{
    int *write_ptr;

    /* OBSOLETE cbuffer_get_write_address_and_size(cbuffer, &write_ptr, &buffer_size); */

    patch_fn_shared(hydra_cbuff_offset);

    write_ptr = cbuffer->write_ptr;

    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;

        mmu_handle_unpack(write_ptr, &handle);
        return buffer_octets_to_words( mmu_buffer_get_handle_offset(handle) );
    }
    else if (BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor) )
    {
        mmu_handle handle;

        /* Extract the (local) write handle information from the cbuffer.
         * Could be using an aux handle. */
        if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE )
        {
            write_ptr = cbuffer->aux_ptr;
        }
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)write_ptr);
        return BAC_OFFSET_TO_SAMPLES(mmu_buffer_get_handle_offset(handle), mmu_buffer_get_flags(handle));
    }
    else
    {
        /* otherwise must be SW buffer (although shouldn't use offsets with pure SW buffers)
           more likely a wr handle of a HW RD buffer */
        /* Difference in pointers needs to be divided by number of memory locations */
        return ((uintptr_t)write_ptr - (uintptr_t)cbuffer->base_addr)/sizeof(unsigned int);
    }
}

/****************************************************************************
 *
 * cbuffer_set_read_offset - sets the offset into the buffer of the read pointer.
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      offset    - read offset for the supplied cbuffer, in words
 *
 * Notes. Should not be called on pure SW buffers.
 *        Use with caution, offset should not exceed buffer_size.
 */
void cbuffer_set_read_offset(tCbuffer *cbuffer, unsigned int offset)
{

    patch_fn_shared(hydra_cbuff_offset);

    PL_ASSERT(offset < (cbuffer->size));

    if( (cbuffer->descriptor) == BUF_DESC_SW_BUFFER)
    {
        /* pure SW buffers shouldn't use offsets. */
        panic_diatribe(PANIC_AUDIO_BUFFER_DESC_INVALID, cbuffer->descriptor);
    }
    else
    {
        if(!BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            /* not mmu rd handle, must be SW rd pointer */
            int  *new_rd_ptr;

            /* Offset in words converted to number of memory locations */
            new_rd_ptr = (int*)((uintptr_t)cbuffer->base_addr + (uintptr_t)offset*sizeof(int));
            /* aux should not be modified, so we can ignore that) */
            /* OBSOLETE cbuffer_set_read_address, but we know it's SW handle */
            cbuffer->read_ptr = new_rd_ptr;
        }
        else
        {
            /* must be mmu or remote */
            mmu_handle handle;
            if (BUF_DESC_BUFFER_TYPE_MMU(cbuffer->descriptor))
            {
                handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);
            }
            else
            {
                /* otherwise must be remote */
                mmu_handle_unpack(cbuffer->read_ptr, &handle);
            }
            mmu_buffer_set_handle_offset(handle, SAMPLES_TO_BAC_OFFSET(offset, mmu_buffer_get_flags(handle)));
        }
    }
}


/****************************************************************************
 *
 * cbuffer_set_write_offset - sets the offset into the buffer of the write pointer.
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      offset    - write offset for the supplied cbuffer, in words
 *
 * Notes. Should not be called on pure SW buffers.
 *        Use with caution, offset should not exceed buffer_size.
 */
void cbuffer_set_write_offset(tCbuffer *cbuffer, unsigned int offset)
{
    patch_fn_shared(hydra_cbuff_offset);

    PL_ASSERT(offset < (cbuffer->size));

    if( (cbuffer->descriptor) == BUF_DESC_SW_BUFFER )
    {
        /* pure SW buffers shouldn't use offsets. */
        panic_diatribe(PANIC_AUDIO_BUFFER_DESC_INVALID, cbuffer->descriptor);
    }
    else
    {
        if(!BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
        {
            /* not mmu wr handle, must be SW wr pointer */
            int  *new_wr_ptr;

            /* Offset in words converted to number of memory locations */
            new_wr_ptr = (int*)((uintptr_t)cbuffer->base_addr + (uintptr_t)offset*sizeof(int));
            /* aux should not be modified, so we can ignore that) */
            /* OBSOLETE cbuffer_set_write_address, but we know it's SW handle */
            cbuffer->write_ptr = new_wr_ptr;
        }
        else
        {
            /* must be mmu or remote */
            mmu_handle handle;
            if (BUF_DESC_BUFFER_TYPE_MMU(cbuffer->descriptor))
            {
                handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);
            }
            else
            {
                /* otherwise must be remote */
                mmu_handle_unpack(cbuffer->write_ptr, &handle);
            }
            mmu_buffer_set_handle_offset(handle, SAMPLES_TO_BAC_OFFSET(offset, mmu_buffer_get_flags(handle)));
        }
    }
}

/****************************************************************************
 *
 * cbuffer_set_read_sample_size - Sets the sample size for the read mmu handle
 *
 * Input arguments:
 *      cbuffer     - pointer to a cbuffer structure
 *      sample_size - enum indicating the size of data stored in buffer
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_read_sample_size(tCbuffer *cbuffer, buffer_sample_size sample_size)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_sample);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set sample size on a local MMU handle */
        return FALSE;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

    /* Set up the sample size for the read handle */
    flags = mmu_buffer_get_flags(handle);
    MMU_BUF_SAMP_SZ_SET(flags, sample_size);
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a read handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same sample size for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        MMU_BUF_SAMP_SZ_SET(flags, sample_size);
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_write_sample_size - Sets the sample size for the write mmu handle
 *
 * Input arguments:
 *      cbuffer     - pointer to a cbuffer structure
 *      sample_size - enum indicating the size of data stored in buffer
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_write_sample_size(tCbuffer *cbuffer, buffer_sample_size sample_size)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_sample);

    /* Write handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set sample size on a local MMU handle */
        return FALSE;
    }

    /* Get the write handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

    /* Set up the sample size for the write handle */
    flags = mmu_buffer_get_flags(handle);
    MMU_BUF_SAMP_SZ_SET(flags, sample_size);
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a write handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same sample size for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        MMU_BUF_SAMP_SZ_SET(flags, sample_size);
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_read_sign_extend - Sets the sign extend for the read mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_read_sign_extend(tCbuffer *cbuffer, bool set)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set sign extend on a local MMU handle */
        return FALSE;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

    /* Set up the sign extend for the read handle */
    flags = mmu_buffer_get_flags(handle);
    if (set)
    {
        MMU_BUF_SE_SET(flags);
    }
    else
    {
        MMU_BUF_SE_UNSET(flags);
    }
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a read handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same sign extend for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_SE_SET(flags);
        }
        else
        {
            MMU_BUF_SE_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_write_sign_extend - Sets the sign extend for the write mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_write_sign_extend(tCbuffer *cbuffer, bool set)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_modify);

    /* Write handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set sign extend on a local MMU handle */
        return FALSE;
    }

    /* Get the write handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

    /* Set up the sign extend for the write handle */
    flags = mmu_buffer_get_flags(handle);
    if (set)
    {
        MMU_BUF_SE_SET(flags);
    }
    else
    {
        MMU_BUF_SE_UNSET(flags);
    }
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a write handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same sign extend for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_SE_SET(flags);
        }
        else
        {
            MMU_BUF_SE_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_read_packing - Sets the packing for the read mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_read_packing(tCbuffer *cbuffer, bool set)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_sample);

    /* Read handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set packing on a local MMU handle */
        return FALSE;
    }

    /* Get the read handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->read_ptr);

    /* Set up the packing for the read handle */
    flags = mmu_buffer_get_flags(handle);
    if (set)
    {
        MMU_BUF_PKG_EN_SET(flags);
    }
    else
    {
        MMU_BUF_PKG_EN_UNSET(flags);
    }
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a read handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_READ)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same packing for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_PKG_EN_SET(flags);
        }
        else
        {
            MMU_BUF_PKG_EN_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * cbuffer_set_write_packing - Sets the packing for the write mmu handle
 *
 * Input arguments:
 *      cbuffer   - pointer to a cbuffer structure
 *      set       - bool indicating set or clear TRUE = set, False = clear
 * Return value
 *      TRUE if successful, FALSE otherwise.
 *
 */
bool cbuffer_set_write_packing(tCbuffer *cbuffer, bool set)
{
    unsigned flags;
    mmu_handle handle;

    patch_fn_shared(hydra_cbuff_sample);

    /* Write handle should be local MMU */
    if (BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) ||
        !BUF_DESC_WR_PTR_TYPE_MMU(cbuffer->descriptor))
    {
        /* Can only set packing on a local MMU handle */
        return FALSE;
    }

    /* Get the write handle */
    handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->write_ptr);

    /* Set up the packing for the write handle */
    flags = mmu_buffer_get_flags(handle);
    if (set)
    {
        MMU_BUF_PKG_EN_SET(flags);
    }
    else
    {
        MMU_BUF_PKG_EN_UNSET(flags);
    }
    mmu_buffer_set_flags(handle, flags);

    /* If the aux handle is in use as a write handle */
    if (cbuffer_aux_in_use(cbuffer) == BUFF_AUX_PTR_TYPE_WRITE)
    {
        /* Get the aux handle */
        handle = mmu_buffer_get_handle_from_ptr((audio_buf_handle_struc *)cbuffer->aux_ptr);

        /* Set up the same packing for the aux handle */
        flags = mmu_buffer_get_flags(handle);
        if (set)
        {
            MMU_BUF_PKG_EN_SET(flags);
        }
        else
        {
            MMU_BUF_PKG_EN_UNSET(flags);
        }
        mmu_buffer_set_flags(handle, flags);
    }

    /* Succeeded */
    return TRUE;
}


unsigned cbuffer_get_mmu_flags(const tCbuffer *cb, bool read_flags)
{
    return mmu_buffer_get_flags(
                mmu_buffer_get_handle_from_ptr(
                    (audio_buf_handle_struc *)(read_flags ? cb->read_ptr : cb->write_ptr)));
}
