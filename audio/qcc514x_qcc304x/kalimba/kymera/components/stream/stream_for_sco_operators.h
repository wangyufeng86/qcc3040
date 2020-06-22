/****************************************************************************
 * Copyright (c) 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
#ifndef STREAM_FOR_SCO_OPERATORS_H
#define STREAM_FOR_SCO_OPERATORS_H

#include "stream/stream_common.h"

/**
 * \brief resets SCO RX metadata buffer offset
 *
 * \param *ep pointer to the conneted endpoint to sco_rx endpoint
 */
extern void stream_sco_reset_sco_metadata_buffer_offset(ENDPOINT *ep);

/**
 * \brief resets ISO RX metadata buffer offset
 *
 * \param *ep pointer to the conneted endpoint to iso_rx endpoint
 */
extern void stream_iso_reset_sco_metadata_buffer_offset(ENDPOINT *ep);

#endif /* STREAM_FOR_SCO_OPERATORS_H */
