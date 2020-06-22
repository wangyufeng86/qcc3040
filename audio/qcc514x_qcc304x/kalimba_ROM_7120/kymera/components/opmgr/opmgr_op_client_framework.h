/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_op_client_framework.h
 * \ingroup  opmgr
 *
 * Operator Manager for Operator Clients. <br>
 * This file contains the operator manager functionalities for operator clients
 * used by the Framework (and thus not exposed to the customers). <br>
 */

#ifndef OPMGR_OPERATOR_CLIENT_FRAMEWORK_H
#define OPMGR_OPERATOR_CLIENT_FRAMEWORK_H

/****************************************************************************
Include Files
*/

#include "types.h"
#include "adaptor/connection_id.h"

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Public Constant Declarations
*/

/****************************************************************************
Public Macro Declarations
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/
/**
 * \brief Delegates some operators to an operator client.
 *
 * \param con_id Connection ID of this call.

 * \param ext_op_id External operator ID, as seen by the application client.

 * \param num_owned_ops Number of operators owned by this operator client.

 * \param owned_op_array Array of the external operator ids to take ownership of.
 *
 * \return TRUE if the client has successfully been promoted.
 *         FALSE if there is an error. Error causes can be:
 *         - operator doesn't exist
 *         - requesting client doesn't own the operator
 *         - maximum number of operator clients has been reached;
 *         - no more memory
 *         - no operators delegated
 *         - any of the delegated operators does not exist
 *         - any of the delegated operators is not owned by this client
 */
bool opmgr_op_client_delegate(CONNECTION_LINK con_id,
                              unsigned ext_op_id,
                              unsigned num_owned_ops,
                              unsigned *owned_op_array);

/**
 * \brief Cancels delegation of an operator to an operator client.
 *
 * \param con_id Connection ID of this call.
 *
 * \param num_ops Number of operators in the following array.
 *
 * \param op_array Array of the operators to cancel delegation.
 *
 * \return Number of operators that have been successfully cancelled
 *         the delegation.
 *         Error causes can be:
 *         - no memory
 *         - requesting client doesn't own the operator
 *         - at least for one of the operators we failed cancelling delegation
 */
unsigned opmgr_op_client_cancel_delegation(CONNECTION_LINK con_id,
                                           unsigned num_ops,
                                           unsigned *op_array);

/**
 * \brief Forward an unsolicited message to the operator client that owns the
 *        sender operator.
 *
 * \param con_id Connection ID for the sender operator.
 *
 * \param msg_length Length of the unsolicited message.
 *
 * \param msg Unsolicited message.
 *
 * \return TRUE if the message was successfully forwarded.
 */
bool opmgr_op_client_unsolicited_message(CONNECTION_LINK con_id,
                                         unsigned msg_length,
                                         unsigned *msg);

#endif /* OPMGR_OPERATOR_CLIENT_FRAMEWORK_H */

