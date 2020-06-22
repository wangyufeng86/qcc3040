/*!
\copyright  Copyright (c) 2017 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Definitions of latency.

For TWS standard, latencies are measured from arrival of audio in the
audio subsystem to the synchronised rendering by TWS master and TWS
slave.
*/

#ifndef EARBUD_LATENCY_H
#define EARBUD_LATENCY_H

#include <rtime.h>
#include <hydra_macros.h>

/*! The TWS standard target latency in milli-seconds. */
#if defined INCLUDE_MIRRORING || defined INCLUDE_STEREO
#define TWS_STANDARD_LATENCY_MS 200
#else
#define TWS_STANDARD_LATENCY_MS 300
#endif

/*! The TWS standard target latency in micro-seconds. */
#define TWS_STANDARD_LATENCY_US (1000 * TWS_STANDARD_LATENCY_MS)

/*! The TWS slave latency in milli-seconds. */
#define TWS_SLAVE_LATENCY_MS 150

/*! The maximum allowed latency, above which the TTP generator in the RTP decoder
    will reset. */
#define TWS_STANDARD_LATENCY_MAX_MS (TWS_STANDARD_LATENCY_MS + 300)

/*! Buffering in the TWS standard graph is split between the (pre) source sync and
 *  pre-decoder buffers. The buffer after source sync is treated as "headroom",
 *  since it is comparatively small (4 * kick period). So for SBC/AAC, the entire
 *  TTP latency is split over these two buffers. AptX will have some extra buffer
 *  space since the decoding is split across two operators - there will be an
 *  extra buffer between the demux operator and aptX mono decoder. */
#define PCM_LATENCY_BUFFER_MS MAX(50, (TWS_STANDARD_LATENCY_MS - 50))
/*! The difference between the maximum latency and the PCM buffer latency is the
    amount of buffering required pre-decoder. It must be possible for the buffer
    to accommodate the maximum allowed latency. */
#define PRE_DECODER_BUFFER_MS MAX(50, (TWS_STANDARD_LATENCY_MAX_MS - PCM_LATENCY_BUFFER_MS))

/*! The minimum buffer to use before the forwarding path in the chain in milli-seconds */
#define PRE_FORWARDING_BUFFER_MIN_MS (50)

#endif /* EARBUD_LATENCY_H */
