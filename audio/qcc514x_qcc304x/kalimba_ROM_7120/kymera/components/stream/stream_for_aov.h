/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup stream Stream Subsystem
 *
 * \file  stream_for_aov.h
 *
 * stream header file.
 * This file contains stream functions that are only publicly available to
 * aov/lp_preserve
 */

#ifndef STREAM_FOR_AOV_H
#define STREAM_FOR_AOV_H

#include "stream_common.h"
#include "adaptor/connection_id.h"

/**
 * \brief Accessor for getting the connection id of an endpoint.
 *
 * \param ep The endpoint to get the connection id of.
 *
 * \return The connection id of the ep.
 */
extern CONNECTION_LINK stream_con_id_from_ep(ENDPOINT *ep);

/**
 * \brief Sets the connection id for all endpoints in a list.
 *        The endpoints in the list MUST be running on the primary core.
 *        Note as in other stream interface functions the argument con_id
 *        truly means just the creator (or sender id).
 *
 * \param ep        
 * \param num_eps  
 * \param con_id
 *
 */
extern void stream_set_endpoint_connection_id(ENDPOINT **ep,
                                              unsigned num_eps,
                                              CONNECTION_LINK con_id);
#endif /* STREAM_FOR_AOV_H */

