/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
 ****************************************************************************/
/**
 * \file  cbops_insert_op.c
 * \ingroup cbops
 *
 * This file contains functions for cbops multi-channel sidetone mixing operator. *
 */

/****************************************************************************
Include Files
*/
#include "pmalloc/pl_malloc.h"
#include "cbops_c.h"
#include <string.h>
#include "pl_assert.h"
#include "platform/pl_intrinsics.h"
/****************************************************************************
Public Function Definitions
*/

/**
 * create_sidetone_mix_op
 * \brief creates single channel sidetone mix operator
 * \param input_idx cbops buffer index for main input channel
 * \param output_idx cbops buffer index for main output channel
 * \param st_in_idx cbops buffer index for sidetone channel
 * \param threshold threshold for sidetone latency
 * \return pointer to operator
 */
cbops_op* create_sidetone_mix_op(unsigned input_idx,
                                 unsigned output_idx,
                                 unsigned st_in_idx,
                                 unsigned threshold)
{
    cbops_op *op = create_multichan_sidetone_mix_op_base(1,           /* nr_channels */
                                                         &input_idx,  /* *input_idxs */
                                                         &output_idx, /* *output_idxs */
                                                         1,           /* nr_st_channels */
                                                         &st_in_idx,  /* *st_in_idxs */
                                                         threshold);  /* threshold */
    if(NULL != op)
    {
        /* main channel[0]  will be mixed into sidetone channel[0] */
        cbops_sidetone_mix_map_channel(op, 0, 0);
    }

    return op;
}
/**
 * create_multichan_sidetone_mix_op_base
 * \brief
 *    create multi channel sidetone mix operator
 *    NOTE: This function however wont map any main channel to a particular
 *          sidetone input, after creation use cbops_sidetone_mix_map_channel
 *          to tell the operator which sidetone buffer should mix into a main
 *          channel.
 *
 * \param nr_channels number of main channels
 *
 * \param input_idxs array of cbops buffer indexes for main input channels
 *
 * \param output_idxs array of cbops buffer indexes for main output channels
 *   NOTES:
 *        1- This operator can work in-place, i.e. same input_idxs and output_idxs
 *        2- each input channel is copied to its corresponding output,
 *           after mixing with a sidetone input if configured.
 *
 * \param nr_st_channels number of sidetone channels, practically this shall not
 *   be needed to exceed nr_channels.
 *
 * \param st_in_idxs array of cbops buffer indexes for sidetone channels
 *   NOTES:
 *        1- each sidetone channel can be mixed into any number of main channels.
 *        2- each main channel can be mixed into only one or none sidetone channel.
 *
 * \param threshold threshold for sidetone latency
 * \return pointer to operator
 */
cbops_op* create_multichan_sidetone_mix_op_base(unsigned nr_channels,
                                                unsigned *input_idxs,
                                                unsigned *output_idxs,
                                                unsigned nr_st_channels,
                                                unsigned *st_in_idxs,
                                                unsigned threshold)
{
    /* Expect at least one sidetone channel*/
    if(nr_st_channels == 0 || /* At least one sidetone input channel */
       nr_channels == 0    || /* At least one main channel */
       nr_st_channels > CBOPS_SIDETONE_MIX_MAX_ST_CHANNELS)
    {
        /* something is wrong with the caller */
        return NULL;
    }

    cbops_op *op = (cbops_op*)xzpmalloc(sizeof_cbops_op(cbops_multichan_sidetone_mix_op, nr_channels, nr_channels)
                                        + (nr_channels + nr_st_channels - 1)*sizeof(unsigned));
    if(op != NULL)
    {
        cbops_multichan_sidetone_mix_op  *params;

        op->function_vector    = cbops_multichan_sidetone_mix_table;

        /* Setup cbops param struct header info */
        params = (cbops_multichan_sidetone_mix_op*)cbops_populate_param_hdr(op, nr_channels, nr_channels, input_idxs, output_idxs);

        /* Only config param which is threshold which is for all of the
         * sidetone channels
         */
        params->max_samples = threshold;

        /* number of main channels */
        params->nr_channels = nr_channels;

        /* number of sidetone channels */
        params->nr_st_channels = nr_st_channels;

        /* write cbops buffer indexes for sidetone buffers */
        unsigned *params_st_in_idxs = &params->st_idxs[0];
        memcpy(params_st_in_idxs, st_in_idxs, sizeof(unsigned)*nr_st_channels);

        /* initialise sidetone mixing mapping */
        unsigned i;
        unsigned *params_st_map_idxs = &params->st_idxs[nr_st_channels];
        for(i = 0; i < nr_channels; ++i)
        {
            /* initialise to no sidetone mixing, this needs to be
             * configured later to do any sidetone mixing
             */
            params_st_map_idxs[i] = CBOPS_BUFFER_NOT_SUPPLIED;
        }
    }

    return(op);
}

/**
 * cbops_sidetone_mix_map_channel
 * \brief configures a main channel of the operator to use a sidetone channel for mixing
 * \param op cbops multichannel sidetone mix operator (input)
 * \param input_channel main channel number (0 to (nr_channels-1))
 * \param use_st_channel use this sidetone channel (0 to (nr_st_channels-1))
 */
void cbops_sidetone_mix_map_channel(cbops_op *op, unsigned input_channel, unsigned use_st_channel)
{
    if(NULL != op)
    {
        cbops_multichan_sidetone_mix_op  *params = CBOPS_PARAM_PTR(op, cbops_multichan_sidetone_mix_op);
        PL_ASSERT(use_st_channel < params->nr_st_channels);
        PL_ASSERT(input_channel < params->nr_channels);

        /* get sidetone indexes */
        unsigned *st_in_idxs = &params->st_idxs[0];

        /* get mapping table indexes */
        unsigned *st_map_idxs = &params->st_idxs[params->nr_st_channels];

        /* configure this channel to use sidetone mixing from use_st_channel */
        st_map_idxs[input_channel] = st_in_idxs[use_st_channel];
    }
}
/**
 * cbops_sidetone_mix_map_one_to_all
 * \brief configures a sidetone channel to be mixed into all main channels
 * \param op cbops multichannel sidetone mix operator (input)
 * \param use_st_channel use this sidetone channel to mix into all channels
 */
void cbops_sidetone_mix_map_one_to_all(cbops_op *op, unsigned use_st_channel)
{
    if(NULL != op)
    {
        unsigned i;
        cbops_multichan_sidetone_mix_op  *params = CBOPS_PARAM_PTR(op, cbops_multichan_sidetone_mix_op);
        PL_ASSERT(use_st_channel < params->nr_st_channels);
        for(i = 0; i < params->nr_channels; ++i)
        {
            cbops_sidetone_mix_map_channel(op, i, use_st_channel);
        }
    }
}

/**
 * cbops_sidetone_mix_map_one_to_one
 * \brief configures one to one sidetone mixing, i.e each main channel
 *        will be mixed into same-number sidetone channel.
 * \param op cbops multichannel sidetone mix operator (input)
 */
void cbops_sidetone_mix_map_one_to_one(cbops_op *op)
{
    if(NULL != op)
    {
        unsigned i;
        cbops_multichan_sidetone_mix_op  *params = CBOPS_PARAM_PTR(op, cbops_multichan_sidetone_mix_op);
        for(i = 0; i < params->nr_channels; ++i)
        {
            cbops_sidetone_mix_map_channel(op, i, i);
        }
    }
}

/**
 * create_multichan_sidetone_mix_op
 * \brief create multi channel sidetone mix operator
 *  Note:  This is same as create_multichan_sidetone_mix_op_base just needs index
 *    of first channel.
 *
 * \param nr_channels number of main channels *
 * \param first_input_idx buffer index for first main input channel, others
 *        are expected to be consecutive.
 * \param first_output_idx buffer indexer for first main output channel, others
 *        are expected to be consecutive.
 * \param nr_st_channels number of sidetone channels *
 * \param first_st_in_idx buffer index for first sidetone channel, others
 *        are expected to be consecutive.
 *
 * \param threshold threshold for sidetone latency
 * \return pointer to operator
 */
cbops_op* create_multichan_sidetone_mix_op(unsigned nr_channels,
                                           unsigned first_input_idx,
                                           unsigned first_output_idx,
                                           unsigned nr_st_channels,
                                           unsigned first_st_in_idx,
                                           unsigned threshold
                                           )
{
    /* We have first index only, to create we need an array of indexes,
     * create buffer indexes using maximum index.
     */
    unsigned max_idx = pl_max(first_input_idx, first_output_idx)+nr_channels;
    max_idx = pl_max(max_idx, nr_st_channels + first_st_in_idx);
    unsigned *idxs = create_default_indexes(max_idx);
    if(NULL == idxs)
    {
        return NULL;
    }

    /* we have everything to create the operator */
    cbops_op* op = create_multichan_sidetone_mix_op_base(nr_channels,
                                                         &idxs[first_input_idx],
                                                         &idxs[first_output_idx],
                                                         nr_st_channels,
                                                         &idxs[first_st_in_idx],
                                                         threshold);
    pfree(idxs);
    return op;
}
