/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file cbuffer_copy_ex.c
 * \ingroup buffer
 *
 *  cbuffer_ex octet handling extension functions.
 *
 *  TODO currently only 16-bit and 32-bit big endian packing without any shift.
 *  Mainly used by the a2dp and the file endpoint to copy encoded data
 *  from one cbuffer to another.
 */

#ifdef INSTALL_CBUFFER_EX

#include "buffer_private.h"

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
 * cbuffer_copy_ex - copies from a 16-bit unpacked/32-bit packed buffer to a
 *                   16-bit unpacked/32-bit packed buffer
 *
 * Input arguments:
 *      dst                 - destination buffer
 *      src                 - source buffer
 *      num_octets          - number of octets to be copied
 *
 * Return value
 *      The number of copied octets. (NOTE: the ASM implementation does not
 *      return the copied octets)
 */
unsigned cbuffer_copy_ex(tCbuffer * dst, tCbuffer *src, unsigned num_octets)
{
    unsigned src_data, dest_space, octets_to_copy, r_octet_offset, w_octet_offset;
    unsigned us_octets_source = cbuffer_get_usable_octets(src);
    unsigned us_octets_dest = cbuffer_get_usable_octets(dst);

    patch_fn_shared(cbuffer_copy_ex);

    /* Only 2 or 4 usable octets are supported. */
    PL_ASSERT((us_octets_source == 2) || (us_octets_source == 4));
    PL_ASSERT((us_octets_dest == 2) || (us_octets_dest == 4));

    src_data = cbuffer_calc_amount_data_ex(src);
    dest_space = cbuffer_calc_amount_space_ex(dst);

    octets_to_copy = MIN(src_data, num_octets);
    octets_to_copy = MIN(octets_to_copy, dest_space);

    if (octets_to_copy == 0)
    {
        return 0;
    }

    /* Warning, hydra/mmu_buff is crescendo d01 specific, detect changes. */
    COMPILE_TIME_ASSERT( BAC_BUFFER_SAMPLE_8_BIT == 0, Assumption_about_ENUM_8bit_in_mmu_buff_ex_asm_violated);
    COMPILE_TIME_ASSERT( BAC_BUFFER_SAMPLE_16_BIT == 1, Assumption_about_ENUM_16bit_in_mmu_buff_ex_asm_violated);
    /* This is only one example of changes in io_defs > d01 that will break
       our assumptions TODO could make more use of these */
    COMPILE_TIME_ASSERT( BAC_BUFFER_SAMPLE_SIZE_MSB - BAC_BUFFER_SIZE_LSB < 32 &&
                         BAC_BUFFER_SAMPLE_SIZE_LSB - BAC_BUFFER_SIZE_LSB > 0,
                         Assumptions_about_POSN_in_mmu_buff_asm_violated);

    cbuffer_get_read_address_ex(src, &r_octet_offset);
    cbuffer_get_write_address_ex(dst, &w_octet_offset);

    if(us_octets_source == USABLE_OCTETS_16BIT)
    {
        if(us_octets_dest == USABLE_OCTETS_16BIT)
        {
            /* the unaligned version is MIPS heavier, so keep the aligned variant separate */
            if (r_octet_offset == w_octet_offset)
            {
                cbuffer_copy_aligned_16bit_be_zero_shift_ex(dst, src, octets_to_copy);
            }
            else
            {
               cbuffer_copy_unaligned_16bit_be_zero_shift_ex(dst, src, octets_to_copy);
            }
        }
        else
        {
            /* pack */
            /* the unaligned version is MIPS heavier, so keep the aligned variant separate */
            if (r_octet_offset == (w_octet_offset&1))   /* for dst take lsb */
            {
                /* this is the most efficient implementation, all aligned to word offsets */
                cbuffer_pack_aligned_ex(dst, src, octets_to_copy);
            }
            else
            {
                /* TODO not in phase is still slow (in-phase is assumed in practice) */
                cbuffer_pack_unaligned_ex(dst, src, octets_to_copy);
            }
        }
    }
    else
    {
        /* src is 32-bit */
        if(us_octets_dest == USABLE_OCTETS_16BIT)
        {
            /* unpack */
            /* the unaligned version is MIPS heavier, so keep the aligned variant separate */
            if ((r_octet_offset&1) == w_octet_offset)   /* for src take lsb */
            {
                /* this is the most efficient implementation, all aligned to word offsets */
                cbuffer_unpack_aligned_ex(dst, src, octets_to_copy);
            }
            else
            {
                /* TODO not in phase is still slow (in-phase is assumed in practice) */
                cbuffer_unpack_unaligned_ex(dst, src, octets_to_copy);
            }
        }
        else
        {
            /* dst 32-bit as well, simple copy */
            /* the unaligned version is MIPS heavier, so keep the aligned variant separate */
            if (r_octet_offset == w_octet_offset)
            {
                cbuffer_copy_aligned_32bit_be_ex(dst, src, octets_to_copy);
            }
            else
            {
                cbuffer_copy_unaligned_32bit_be_ex(dst, src, octets_to_copy);
            }
        }
    }

    /* TODO currently the asm implementation doesn't supply octets_copied */
    return octets_to_copy;
}

/****************************************************************************
 * cbuffer_get_read_offset_ex - returns the read offset of the read pointer wrt
 *                              to the base
 *
 * Input arguments:
 *      cbuffer             - the buffer whose rd offset is to be returned
 *
 * Return value
 *      The read offset in octets.
 */
unsigned int cbuffer_get_read_offset_ex(tCbuffer *cbuffer)
{
    int *read_ptr;

    patch_fn_shared(cbuffer);

    read_ptr = cbuffer->read_ptr;

    PL_ASSERT(!BUF_DESC_IS_REMOTE_MMU(cbuffer->descriptor) &&
              !BUF_DESC_RD_PTR_TYPE_MMU(cbuffer->descriptor));

    return ((uintptr_t)read_ptr - (uintptr_t)cbuffer->base_addr);
}

/****************************************************************************
 * cbuffer_get_size_ex - returns the size of the buffer in valid octets.
 *
 * Input arguments:
 *      cbuffer             - the buffer
 *
 * Return value
 *      The size of the buffer in valid octets.
 */
unsigned cbuffer_get_size_ex(tCbuffer *cb)
{
    unsigned usable_octets = cbuffer_get_usable_octets(cb);

    if(usable_octets == NR_OF_OCTETS_IN_WORD(USE_16BIT_PER_WORD))
    {
        /* 16-bit packed */
        return cbuffer_get_size_in_octets(cb) / 2;
    }
    else
    {
        /* 32-bit packed */
        return cbuffer_get_size_in_octets(cb);
    }
}

#endif /* INSTALL_CBUFFER_EX */
