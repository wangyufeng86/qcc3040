/**
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
* \file  aec_reference_latency.c
* \ingroup  capabilities
*
*  AEC Reference
*
*/

/****************************************************************************
Include Files
*/

#include "pmalloc/pl_malloc.h"
#include "aec_reference_latency.h"

/****************************************************************************
Private Function Declarations
*/
static cbops_op* create_latency_op(unsigned idx, aec_latency_common *common, void *function_vector);

/****************************************************************************
Public Function Definitions
*/

/**
 * create_mic_latency_op
 * \brief creates cbops latency operator for mic graph
 *
 * \param idx index of mic master channel
 * \param common Pointer to common aec_latency_common latency structure
 * \return pointer to created cbops latency operator, NULL if fails to create.
 */
cbops_op* create_mic_latency_op(unsigned idx, aec_latency_common *common)
{
    return create_latency_op(idx, common, cbops_mic_latency_table);
}

/**
 * create_speaker_latency_op
 * \brief creates cbops latency operator for speaker graph
 *
 * \param idx index of speaker master channel
 * \param common Pointer to common aec_latency_common latency structure
 * \return pointer to created cbops latency operator, NULL if fails to create.
 */
cbops_op* create_speaker_latency_op(unsigned idx, aec_latency_common *common)
{
    return create_latency_op(idx, common, cbops_speaker_latency_table);
}

/****************************************************************************
Priavte Function Definitions
*/

/**
 * create_latency_op
 * \brief creates cbops latency operator
 *
 * \param idx index of mic master channel
 * \param common Pointer to common aec_latency_common latency structure
 * \function_vector cbops function for the operator
 * \return pointer to created cbops latency operator, NULL if fails to create.
 */
static cbops_op* create_latency_op(unsigned idx, aec_latency_common *common, void *function_vector)
{

    patch_fn_shared(aec_reference);
    // cbop param struct size (header plus cbop-specific parameters, no buffer table)
    cbops_op *op = (cbops_op*)xzpmalloc(sizeof_cbops_op(latency_op, 0, 0));

    if(op)
    {
        latency_op *params = (latency_op *)cbops_populate_param_hdr(op,
                                                                    0,    /* number of inputs */
                                                                    0,    /* number of outputs */
                                                                    NULL, /* input index table */
                                                                    NULL);/* output index table */
        /* Setup Operator func table */
        op->function_vector = function_vector;

        /* Setup cbop param struct header info */

        params->index  = idx;
        params->common = common;
    }

    return(op);
}

