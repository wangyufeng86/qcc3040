/****************************************************************************
 * Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup override Override module
 * \ingroup opmgr
 *
 * \file  opmgr_endpoint_override.h
 * \ingroup override
 *
 * \brief
 * Operator Manager, Override module public header file.
 * This file contains public override functions and types.
 */

#ifndef OPMGR_ENDPOINT_OVERRIDE_H
#define OPMGR_ENDPOINT_OVERRIDE_H

#include "cbops_mgr/cbops_mgr.h"
#include "opmgr_for_ops.h"

/****************************************************************************
Public Macro Declarations
*/
/** Empty flag. */
#define EMPTY_FLAG (0)

/****************************************************************************
Public Function Declarations
*/

/**
 * \brief  Function is used for allocating the necessary memory for the cbops parameters
 * containing cbops operators indicated by the cbops_flags.
 *
 * \param  flags_to_add - flags indicating which cbops operators will be added.
 * \param  flags_to_remove - flags indicating which cbops operators will be removed.
 *
 * \return Pointer to cbops parameters.
 */
extern CBOPS_PARAMETERS* create_cbops_parameters(uint32 flags_to_add, uint32 flags_to_remove);

/**
 * \brief  Function to free memory allocated for cbops parameters.
 *
 * \param parameters - pointer to cbops parameters.
 */
extern void free_cbops_parameters(CBOPS_PARAMETERS* parameters);

/**
 *  Function for passing all the cbops parameters to a cbops manager.
 *
 * \param parameters - pointer to cbops parameters.
 * \param cbops_mgr - pointer to the cbops manager who will receive the cbops parameters
 * \param source - needed for cbops manager.
 * \param sink - needed for cbops manager.
 *
 * \return boolean indicating if the operation was successful.
 */
extern bool opmgr_override_pass_cbops_parameters(CBOPS_PARAMETERS* parameters, cbops_mgr *cbops_mgr, tCbuffer *source, tCbuffer *sink);

/**
 * \brief  Function to set the shift amount for the cbops parameters.
 *
 * \param  parameters - pointer to cbops parameters.
 * \param  shift_amount - shift amount in integers.
 *
 * \return Indicates if the operation was successful.
 */
extern bool cbops_parameters_set_shift_amount(CBOPS_PARAMETERS* parameters, int shift_amount);

/**
 * \brief  Function to check if dc remove is in operation.
 *
 * \param  cbops_parameters - pointer to cbops parameters
 *
 * \return returns True if DC remove is present False otherwise.
 */
extern bool get_dc_remove_from_cbops_parameters(CBOPS_PARAMETERS* cbops_parameters);

/**
 * \brief  Function to check if U law algorithm is in use
 *
 * \param  cbops_parameters - pointer to cbops parameters
 *
 * \return returns true if the U law algorithm is present false otherwise.
 */
extern bool get_u_law_from_cbops_parameters(CBOPS_PARAMETERS* cbops_parameters);

/**
 * \brief  Function to check if A law algorithm is in use
 *
 * \param  cbops_parameters - pointer to cbops parameters
 *
 * \return returns true if the A law algorithm is present false otherwise.
 */
extern bool get_a_law_from_cbops_parameters(CBOPS_PARAMETERS* cbops_parameters);

/**
 * \brief  Function to get the ratematching rate for the overridden endpoint.
 *
 * \param  ep_hdl - Endpoint handle
 * \param  value - Pointer which contain the value of the ratematching ability.
 *
 * \return Success or failure.
 */
extern bool get_override_ep_ratematch_rate(OVERRIDE_EP_HANDLE ep_hdl, uint32* value);

/**
 * \brief Finds the endpoint whose clock source is seen at the kymera side
 * boundary of an endpoint.
 *
 * \param ep_hdl  The handle of the endpoint whose boundary the clock source is requested of
 *
 * \return The handle of the endpoint whose clock source is present at the boundary. This may
 * be ep_hdl.
 */
extern OVERRIDE_EP_HANDLE override_get_clk_src_of_endpoint(OVERRIDE_EP_HANDLE ep_hdl);

/**
 * \brief computes the rate adjustment between endpoints
 *
 * \param ep_src Handle of the source endpoint
 * \param src_rate Rate of source if not enacting endpoint
 * \param ep_sink Handle of sink the endpoint
 * \param sink_rate Rate of sink if not enacting endpoint
 *
 * \return The rate adjustment, If endpoints invalid then zero.
 */
extern unsigned override_get_rate_adjustment(OVERRIDE_EP_HANDLE ep_src,unsigned src_rate,OVERRIDE_EP_HANDLE ep_sink,unsigned sink_rate);

#endif /* OPMGR_ENDPOINT_OVERRIDE_H */
