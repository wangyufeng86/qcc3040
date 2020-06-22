/****************************************************************************
 * Copyright (c) 2014 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file cbops_sidetone_mix_op.h
 *
 * \ingroup cbops
 *
 * mix sidetone with the output.   Provide latency control
 * and synchronisation between signals
 */

#ifndef CBOPS_SIDETONE_MIX_OP_H
#define CBOPS_SIDETONE_MIX_OP_H
/****************************************************************************
Include Files
*/

#include "buffer/cbuffer_c.h"

/****************************************************************************
Constants
*/
/* maximum number of sidetone channels that is supported,
 * This is only for sanity usage check, the operator structure
 * supports any number.
 */
#define CBOPS_SIDETONE_MIX_MAX_ST_CHANNELS 8

/****************************************************************************
Public Type Declarations
*/

typedef struct multichan_st_mix{
    /* Channel-independent params, with multi channel sidetone mixing
     * this operator expects all sidetone channels are synchronised from
     * write side, the operator in turn will keep the buffers synced from
     * read side.
     */
    unsigned    max_samples;        /* Config: maximum samples is expected to mix per iteration,
                                     * is used for latency control in sidetone path. If the operator
                                     * see more than max twice of this value value left in the buffer it
                                     * will discard the extra to limit the latency in sidetone path.
                                     */

    unsigned    num_inserts;         /* Status(for info only): Total number of invented sidetone
                                     * samples so far. The operator invents sidetone sample if main
                                     * channels have data but sidetone channels don't have enough data.
                                     */

    unsigned    num_drops;          /* Status (For info only): Total number of dropped samples. The
                                     * operator will discard samples from sidetone buffers if it sees
                                     * too many samples left in the buffers after mixing, this
                                     * is to control the latency in sidetone path.
                                     */

    unsigned    nr_channels;        /* number of main channels, each input channel is copied to its
                                     * corresponding output after mixing with sidetone.
                                     */

    unsigned    nr_st_channels;     /* number of sidetone inputs, each input can be configured to
                                     * be mixed with one of the sidetone channels.
                                     */

    /* Buffer indexes, size = nr_st_channels(N) + nr_channels(M)
     *
     *    First CBOPS indexes for sidetone buffers, these must be synchronised
     *    from producer side and they all need to be valid buffers.
     *
     *    SIDETONE_INDX_1  <-- Buffer index for first sidetone input
     *    SIDETONE_INDX_2  <-- Buffer index for second sidetone input
     *    ...
     *    SIDETONE_INDX_N  <-- Buffer index for last sidetone input
     *
     *    And then the mapping table, telling which sidetone channel should be mixed with each
     *    main channel:
     *    MAIN_SIDETONE_INDX_1 <--- (sidetone index to be used for first main channel)
     *    MAIN_SIDETONE_INDX_2 <--- (sidetone index to be used for second main channel)
     *    ...
     *    MAIN_SIDETONE_INDX_M <--- (sidetone index to be used for last main channel)
     *
     *    NOTE: individual main channels can choose not to have sidetone by mapping to
     *    CBOPS_BUFFER_NOT_SUPPLIED.
     */
    unsigned    st_idxs[1];         /* allocated size = nr_st_channels + nr_channels */

}cbops_multichan_sidetone_mix_op;

/****************************************************************************
Public Variable Definitions
*/

/** The address of the function vector table. This is aliased in ASM */
extern unsigned cbops_multichan_sidetone_mix_table[];


#endif /* CBOPS_SIDETONE_MIX_OP_H */
