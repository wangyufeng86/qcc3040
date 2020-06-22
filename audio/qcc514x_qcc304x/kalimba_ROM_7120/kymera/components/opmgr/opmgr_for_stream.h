/****************************************************************************
 * Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup opmgr Operator Manager
 * \file  opmgr_for_stream.h
 *
 * Operator Manager header file. <br>
 * This file contains opmgr functions and types that are only exposed to stream. <br>
 *
 */

#ifndef OPMGR_FOR_STREAM_H
#define OPMGR_FOR_STREAM_H

/****************************************************************************
Include Files
*/
#include "sched_oxygen/sched_oxygen.h"
#include "opmsg_prim.h"
#include "stream/stream_common.h"

/****************************************************************************
Public Function Declarations
*/
/**
 * \brief Get the task_id of the underlying operator of an operator endpoint.
 *
 * \note This is provided to facilitate kicking the operator without performing
 * any slow lookups.
 *
 * \return The task id of the underlying operator.
 */
extern BGINT_TASK opmgr_get_op_task_from_epid(unsigned opidep);

/**
 * \brief This function is used at connect, to cache information about the thing
 * on the other side of the connection if it wants to be kicked. If the other
 * side of the connection doesn't wish to receive kicks then it is expected that
 * this function is not called.
 *
 * \param endpoint_id The ID of the endpoint associated with the operator
 * terminal which is being connected and is required to propagate kicks.
 *
 * \return TRUE the table was successfully updated. FALSE the table update failed
 * likely reason is insufficient RAM.
 */
extern bool opmgr_kick_prop_table_add(unsigned endpoint_id, unsigned ep_id_to_kick);

/**
 * \brief Removes a connection from an operator's kick propagation table
 * when it no longer needs to be kicked.
 *
 * \param endpoint_id The ID of the endpoint to remove from the table.
 */
extern void opmgr_kick_prop_table_remove(unsigned endpoint_id);

/**
 * \brief Get the linked list of terminals.
 *        This hides the OPERATOR_DATA layout from stream.
 */
extern ENDPOINT** opmgr_get_terminal_list(unsigned opidep);

/**
 * \brief  Sends a get configuration message to the operator.
 *
 * \param  endpoint_id - The operator endpoint ID for whom the message is for.
 *
 * \param  key   - Configuration key which specify the type of the get configuration.
 *
 * \param  result - Pointer which will be populated with the asked configuration value.
 *
 * \return TRUE if the message was successfully sent, FALSE otherwise
 *
 */
extern bool opmgr_get_config_msg_to_operator(unsigned int endpoint_id,
                                             unsigned int key,
                                             OPMSG_GET_CONFIG_RESULT *result);

/**
 * \brief  Gets the generic configuration from the operator.
 *
 * \param  endpoint_id - The operator endpoint ID for whom the message is for.
 *
 * \param  key - Configuration key which specify the type of the get configuration.
 *
 * \param  result - Pointer which will be populated with the asked configuration value.
 *
 * \return TRUE if the message was successfully sent, FALSE otherwise
 *
 */
bool opmgr_get_generic_value_from_operator(unsigned int endpoint_id,
                                           OPMSG_CONFIGURATION_KEYS key,
                                           uint32 *result);

/**
 * \brief  Gets the terminal details from the operator.
 *
 * \param  endpoint_id - The operator endpoint ID for whom the message is for.
 *
 * \param  result - Pointer which will be populated with the asked configuration value.
 *
 */
void opmgr_get_terminal_details_from_operator(unsigned int endpoint_id,
                                              uint32 *result);

/**
 * \brief  Gets the terminal ratematch measurement from the operator.
 *
 * \param  endpoint_id - The operator endpoint ID for whom the message is for.
 *
 * \param  sp_deviation - Pointer which will be populated with the asked configuration value.
 *
 * \param  measurement - Pointer which will be populated with the asked configuration value.
 *
 * \return TRUE if the message was successfully sent, FALSE otherwise
 *
 */
bool opmgr_get_ratematch_measure_from_operator(unsigned int endpoint_id,
                                               int *sp_deviation,
                                               RATE_RELATIVE_RATE *measurement);
#endif /* OPMGR_FOR_STREAM_H */

