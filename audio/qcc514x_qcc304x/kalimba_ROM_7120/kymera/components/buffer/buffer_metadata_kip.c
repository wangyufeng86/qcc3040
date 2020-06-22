/****************************************************************************
 * Copyright (c) 2016 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup buffer Buffer Subsystem
 * \file  buffer_metadata_kip.c
 *
 * \ingroup buffer
 *
 * Buffer metadata private functions related to dual core
 *
 */

#include "buffer_private.h"
#include "audio_log/audio_log.h"
#include "stream/stream_kip.h"

#if !defined(UNIT_TEST_BUILD)
/**
 * The arguments for SIZE_IN_WORDS, which is sizeof(metadata_tag)
 * or the size of metadata_tag->xdata, are a multiple of a word.
 * For sizeof(metadata_tag) that is enforced with a compile time
 * assert, for xdata this is enforced by the private data implementation.
 */
#define SIZE_IN_WORDS(x)    ( (x)/sizeof(uintptr_t) )

/*************************************************************************************
  Function definitions for metadata cross cores
*/

bool buff_metadata_kip_tag_to_buff(tCbuffer *cbuffer, metadata_tag* tag)
{
    patch_fn_shared(kip_metadata);

    if (NULL == tag)
    {
        return FALSE;
    }

    /*
     * If operator has no metadata support, cbuffer passed may be NULL. The
     * calling function (buff_metadata_append) checks for that so not necessary
     * to do that here.
     */
    PL_ASSERT(cbuffer != NULL);

    /*
     * Enforce sizeof(metadata_tag) to be a multiple of a word.
     * Otherwise it gets harder to stream a tag over a KIP cbuffer.
     */
    COMPILE_TIME_ASSERT((sizeof(metadata_tag) % (sizeof(int)))==0, sizeof_metadata_tag_not_multiple_of_word);

    unsigned tag_len = SIZE_IN_WORDS(sizeof(metadata_tag));

    unsigned privdata_len = 0;
    if (NULL != tag->xdata)
    {
        privdata_len = buff_metadata_get_priv_data_length(tag);
        PL_ASSERT( (privdata_len % (sizeof(int)))==0 );
        privdata_len = SIZE_IN_WORDS(privdata_len);
    }

    unsigned total_len = tag_len + privdata_len;

    /*
     * 1 + total_len to cover total_len
     */
    if (cbuffer_calc_amount_space_in_words(cbuffer) < 1 + total_len)
    {
        /*
         * This used to be a fault, but following the changes in B-253660, only
         * data that corresponds to the metadata that was successfully pushed will
         * be transferred.
         * Log the event however.
         */
        L2_DBG_MSG1("Metadata KIP buffer not enough space tag len %d", (1 + total_len));
        return FALSE;
    }

    /**
     * Data is pushed into the buffer in the following order:
     *     1. Total tag length (including private data)
     *     3. Tag structure data
     *     4. Tag's private data (if any)
     */

    cbuffer_write(cbuffer, (int*)&total_len, 1);

    /**
     * We need to set the tag->next and tag->xdata to NULL before pushing the tag
     * into the buffer to avoid the other core accessing this core's memory. It's
     * important to restore these pointers before returning from this function.
     */
    metadata_tag* next_tag = tag->next;
    metadata_priv_data* priv_data = tag->xdata;

    tag->next  = NULL;
    tag->xdata = NULL;
    cbuffer_write(cbuffer, (int*)tag, tag_len);

    /* Restore local pointers */
    tag->next  = next_tag;
    tag->xdata = priv_data;

    /**
     * EOF tag's private data is modified when it crosses cores, so it's
     * important that we do that before pushing the private data to the shared
     * cbuffer.
     */
    if (METADATA_STREAM_END(tag))
    {
        /**
         * NB the order in which we call functions below matters. It's important
         * to call metadata_handle_eof_tag_copy() first because the second method
         * sets the "ref" field to NULL.
         */
        metadata_handle_eof_tag_copy(tag, TRUE);
        buff_metadata_kip_prepare_eof_before_append(tag);
    }

    if (NULL != tag->xdata)
    {
        cbuffer_write(cbuffer, (int*)tag->xdata, privdata_len);
    }

    return TRUE;
}

metadata_tag* buff_metadata_kip_tag_from_buff(tCbuffer *cbuffer)
{
    patch_fn_shared(kip_metadata);

    unsigned available_data;
    tCbuffer temp_cbuffer;
    unsigned tag_size = SIZE_IN_WORDS(sizeof(metadata_tag));
    unsigned priv_data_size = 0;
    unsigned total_tag_size = 0;
    metadata_tag* tag_to_return = NULL;

    /*
     * If operator has no metadata support, cbuffer passed may be NULL. The
     * calling functions (buff_metadata_kip_remove, buff_metadata- kip_peek)
     * check for that so not necessary to do that here.
     */
    PL_ASSERT(cbuffer != NULL);

    /*
     * Enforce sizeof(metadata_tag) to be a multiple of a word.
     * Otherwise it gets harder to stream a tag over a KIP cbuffer.
     */
    COMPILE_TIME_ASSERT((sizeof(metadata_tag) % (sizeof(int)))==0, sizeof_metadata_tag_not_multiple_of_word);

    available_data = cbuffer_calc_amount_data_in_words(cbuffer);
    temp_cbuffer = *cbuffer;

    /*
     * 1 + tag_size to cover total_tag_size
     */
    if (available_data < 1 + tag_size)
    {
        /** There's not enough in the buffer to read one tag. */
        return NULL;
    }

    /**
     * NB if we've got here, we have enough data in the buffer for one tag but
     * we still don't know if the private data of the tag (if any) has arrived
     * in the buffer.
     */

    tag_to_return = buff_metadata_new_tag();
    if (NULL == tag_to_return)
    {
        return NULL;
    }

    /**
     * Data is read from the shared buffer in the following order:
     *     1. Total tag length (including private data)
     *     3. Tag structure data
     *     4. Tag's private data (if any)
     */

    cbuffer_read(&temp_cbuffer, (int*)&total_tag_size, 1);

    cbuffer_read(&temp_cbuffer, (int*)tag_to_return, tag_size);

    priv_data_size = total_tag_size - tag_size;

    if (priv_data_size > 0)
    {

        if (available_data < 1 + tag_size + priv_data_size)
        {
            buff_metadata_delete_tag(tag_to_return, FALSE);
            return NULL;
        }

        metadata_priv_data* priv_data = xpmalloc(priv_data_size * sizeof(uintptr_t));

        if (NULL != priv_data)
        {
            cbuffer_read(&temp_cbuffer, (int*)(priv_data), priv_data_size);
            tag_to_return->xdata = priv_data;
        }
        else
        {
            /**
             * It is important to keep tags moving so we will lose the private
             * data here and return the tag without private data. This matches
             * the behaviour in single-core.
             */
            cbuffer_advance_read_ptr(&temp_cbuffer, priv_data_size);
        }
    }

    /* Sync the read pointer */
    cbuffer->read_ptr = temp_cbuffer.read_ptr;

    return tag_to_return;
}

#endif /* !defined(UNIT_TEST_BUILD) */
