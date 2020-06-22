/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_operator_client.c
 * \ingroup  opmgr
 *
 * Operator Manager for Operator Clients. <br>
 * This file contains the operator manager functionalities for operator clients.
 * This contains both APIs used by the Framework (and thus not exposed to the
 * customers) and APIs exposed to the operator client developer, which can be
 * the customer itself. <br>
 */

/****************************************************************************
Include Files
*/

#include "opmgr_private.h"
#include "opmgr_op_client_framework.h"
#include "opmgr_op_client_interface.h"
#include "opmsg_prim.h"

/****************************************************************************
Private Type Declarations
*/

typedef struct OP_CLIENT_OWNED
{
    INT_OP_ID op_id;
    struct OP_CLIENT_OWNED *next;
} OP_CLIENT_OWNED;

typedef struct OP_CLIENT
{
    bool valid;
    INT_OP_ID op_id;
    CONNECTION_PEER client_id;
    unsigned num_owned_ops;
    OP_CLIENT_OWNED *owned_ops;
} OP_CLIENT;

typedef struct OP_CLIENT_UNSOLICITED_MESSAGE
{
    OPMSG_HEADER header;
    EXT_OP_ID src_op_id;
    unsigned payload[OPMGR_ANY_SIZE];
} OP_CLIENT_UNSOLICITED_MESSAGE;

typedef struct OP_CLIENT_MSG_RESPONSE
{
    OPMSG_HEADER header;
    EXT_OP_ID src_op_id;
    unsigned status; /* Serialized STATUS_KYMERA */
    unsigned payload[OPMGR_ANY_SIZE];
} OP_CLIENT_MSG_RESPONSE;

typedef struct OP_CLIENT_CMD_RESPONSE
{
    OPMSG_HEADER header;
    unsigned count;
    unsigned status; /* Serialized STATUS_KYMERA */
    unsigned err_code;
} OP_CLIENT_CMD_RESPONSE;

/****************************************************************************
Private Constant Declarations
*/
#define MAX_NUM_OP_CLIENT    4
/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/** Table of all the created mappings between a client operator and a creator id
 */
static OP_CLIENT *op_client_db[MAX_NUM_OP_CLIENT];

/****************************************************************************
Private Function Declarations
*/

static bool op_client_create_owned_ops(OP_CLIENT *client,
                                       CONNECTION_PEER client_id,
                                       unsigned num_owned_ops,
                                       EXT_OP_ID *owned_op_array);
static bool op_client_remove_delegated_op(CONNECTION_PEER client_id,
                                          INT_OP_ID op_id);
static OP_CLIENT *op_client_with_op_id(INT_OP_ID op_id);
static OP_CLIENT *op_client_with_client_id(CONNECTION_PEER client_id);
static OP_CLIENT *op_client_new(INT_OP_ID op_id);
static void op_client_delete(OP_CLIENT *client);
static void op_client_notify_promotion(CONNECTION_LINK con_id,
                                       OP_CLIENT *client);
static bool op_client_empty_cback(CONNECTION_LINK con_id,
                                  STATUS_KYMERA status,
                                  unsigned ext_op_id,
                                  unsigned num_resp_params,
                                  unsigned *resp_params);
static bool op_client_send_message_cback(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         unsigned ext_op_id,
                                         unsigned num_resp_params,
                                         unsigned *resp_params);
static bool op_client_operator_action_cback(CONNECTION_LINK con_id,
                                            STATUS_KYMERA status,
                                            unsigned count,
                                            unsigned err_code);
static OPERATOR_DATA* op_client_get_client_op_data(CONNECTION_PEER con_id);
/****************************************************************************
Public Function Definitions
To be used within the Framework. Must not be exposed to the operator client
codespace.
*/

/**
 * \brief Delegates some operators to an operator client.
 *        The new owner is promoted to be an operator client (if not already).
 *        This API will be called to serve a request from the application client.
 *        If needed, creates a new entry in the operator client DB, generating
 *        a new client id, saved in the con_id. This will be used to set this
 *        operator as owner of the operators specified in owned_op_array .
 *        This API also checks that the client has the rights to perform this
 *        operation, i.e. it is the owner of the operator to promote as well as
 *        the operators it is delegating.
 *
 *        Note: To check the ownership, we consider that this API should be
 *        called to handle a request from Apps. At this point, input
 *        con_id equals client_id, since there will be no info on the rcv id or
 *        processor id.
 *
 * \param con_id Connection ID of this call.
 * \param ext_op_id External operator ID, as seen by the application client.
 * \param num_owned_ops Number of operators owned by this operator client.
 * \param owned_op_array Array of the external operator ids to take ownership of.
 *
 * \return TRUE if the client has successfully been promoted.
 */
bool opmgr_op_client_delegate(CONNECTION_LINK con_id,
                              unsigned ext_op_id,
                              unsigned num_owned_ops,
                              unsigned *owned_op_array)
{
    OP_CLIENT *new_client;
    unsigned i;
    OPERATOR_DATA *op_data;
    EXT_OP_ID *op_array;
    CONNECTION_PEER cur_client_id = GET_CON_ID_OWNER_CLIENT_ID(con_id);
    INT_OP_ID op_id = EXT_TO_INT_OPID((EXT_OP_ID)ext_op_id);

    patch_fn_shared(op_client);

    if (num_owned_ops == 0)
    {
        /* We should delegate at least one operator. */
        return FALSE;
    }

    op_data = get_anycore_op_data_from_id(op_id);
    if (op_data == NULL)
    {
        /* Operator not found. */
        return FALSE;
    }

    /* Client con_id should be the very same as the con_id saved in the
     * operator's op_data. Note that this means that only operators in P0 will
     * pass this test and thus be promoted.
     */
    if (cur_client_id != GET_CON_ID_OWNER_CLIENT_ID(op_data->con_id))
    {
        /* The client is trying to promote an operator that someone else owns.
         * Not allowed. */
        return FALSE;
    }

    new_client = op_client_with_op_id(op_id);
    if (new_client == NULL)
    {
        new_client = op_client_new(op_id);
        if (new_client == NULL)
        {
            /* No space left (or something went wrong). */
            return FALSE;
        }
    }

    /* Allocate an array of operator ids and use it to avoid mixing pointers.
     * Memory is freed in this function. */
    op_array = xpnewn(num_owned_ops, EXT_OP_ID);
    if (op_array == NULL)
    {
        /* Release all memory and fail. */
        op_client_delete(new_client);
        return FALSE;
    }
    for (i = 0; i < num_owned_ops; i++)
    {
        op_array[i] = (EXT_OP_ID)owned_op_array[i];
    }

    /*
     * Create array with the id for each operator.
     */
    if (!op_client_create_owned_ops(new_client, cur_client_id,
                                    num_owned_ops, op_array))
    {
        /* Release all memory and fail. */
        op_client_delete(new_client);
        return FALSE;
    }

    /* Set ownership of delegated operators to the operator new_client */
    opmgr_set_creator_id(new_client->client_id, num_owned_ops, op_array);

    /* Notify client of the delegated operators. */
    op_client_notify_promotion(con_id, new_client);

    pfree(op_array);
    return TRUE;
}


/**
 * \brief Cancels delegation of an operator to an operator client.
 *        This API will be called to serve a request from the application client.
 *        Effectively resets the ownership of the specified operators 
 *        destroys the entry in the delegated list of the operator client.
 *        This API also checks that the client has the rights to perform this
 *        operation, i.e. it is the owner of the promoted operator.
 *        If the last delegated operator is removed, the operator client is
 *        unpromoted. 
 *
 * \param con_id Connection ID of this call.
 * \param num_ops Number of operators in the following array.
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
                                           unsigned *op_array)
{
    unsigned i, j;
    EXT_OP_ID *tmp_array;
    CONNECTION_PEER original_client_id = GET_CON_ID_OWNER_CLIENT_ID(con_id);
    EXT_OP_ID ext_op_id;

    patch_fn_shared(op_client);

    tmp_array = xpnewn(num_ops, EXT_OP_ID);
    if (tmp_array == NULL)
    {
        /* No memory. Fail the request. */
        return 0;
    }

    /* Cancel delegation of each operator. */
    for (i = 0, j = 0; i < num_ops; i++)
    {
        ext_op_id = (EXT_OP_ID)op_array[i];
        /* Remove from lists of delegated operators. */
        if (op_client_remove_delegated_op(original_client_id,
                                          EXT_TO_INT_OPID(ext_op_id)))
        {
            /* Only the found operators will change ownership. */
            tmp_array[j] = ext_op_id;
            j++;
        }
    }

    /* Set back ownership of the operators that were recognized as delegated. */
    opmgr_set_creator_id(original_client_id, j, tmp_array);

    pdelete(tmp_array);

    return j;
}

/**
 * \brief Forward an unsolicited message to the operator client that owns the
 *        sender operator. The sender's connection id actually identifies the
 *        operator client.
 *
 * \param reversed_con_id Connection ID for the sender operator (reversed).
 * \param msg_length Length of the unsolicited message.
 * \param msg Unsolicited message.
 *
 * \return TRUE if the message was successfully forwarded.
 */
bool opmgr_op_client_unsolicited_message(CONNECTION_LINK reversed_con_id,
                                         unsigned msg_length,
                                         unsigned *msg)
{
    /* The operator client is the receiver of this message. */
    CONNECTION_PEER client_id = GET_CON_ID_RECV_ID(reversed_con_id);
    OPERATOR_DATA *dst_op;
    tRoutingInfo new_rinfo;
    OP_UNSOLICITED_MSG *message = (OP_UNSOLICITED_MSG *)msg;
    OP_CLIENT_UNSOLICITED_MESSAGE *client_msg;
    unsigned len;

    patch_fn_shared(op_client);

    /* Check if sender is owned by this operator client. */
    if (message->client_id != client_id)
    {
        return FALSE;
    }

    /* Lookup the client operator */
    dst_op = op_client_get_client_op_data(client_id);
    if (dst_op == NULL)
    {
        /* Invalid connection id. The client might have been unpromoted.
         * Fail and ignore */
        return FALSE;
    }

    new_rinfo.src_id = client_id;

    /* The destination id is the opid (op client is always in P0) */
    new_rinfo.dest_id = dst_op->id;

    /* Allocate space for the client_msg.
     * Memory freed once the receiver's opmgr_task_handler processes the msg. */
    len = message->length + CLIENT_UNSOLICITED_MESSAGE_SIZE;
    client_msg = (OP_CLIENT_UNSOLICITED_MESSAGE *) xpnewn(len, unsigned);
    if (client_msg == NULL)
    {
        return FALSE;
    }

    /* fill in the client_msg fields */
    len = message->length + CLIENT_UNSOLICITED_MESSAGE_SIZE_EXTRA;
    client_msg->header.cmd_header.client_id = client_id;
    client_msg->header.cmd_header.length = len;
    client_msg->header.msg_id = message->msg_id | OPMSG_OP_CLIENT_REPLY_ID_MASK;
    /* The operator id will have been switched to an external op id from the
     * adaptor. No need to convert it here. */
    client_msg->src_op_id = (EXT_OP_ID)message->op_id;

    if (message->length != 0)
    {
        memcpy(client_msg->payload, message->payload, message->length*sizeof(unsigned));
    }

    /* Save the callback so we can use it when we get the response.
     * Retrieved in message_resp_handler. */
    if (!opmgr_store_in_progress_task(new_rinfo.src_id, dst_op->id,
                                        (void *)op_client_empty_cback))
    {
        /* If we couldn't save the in progress task data then panic */
        panic_diatribe(PANIC_AUDIO_OP_CLIENT_CALLBACK_FAILED, client_id);
    }

    put_message_with_routing(mkqid(dst_op->task_id, 1), OPCMD_MESSAGE, client_msg, &new_rinfo);

    return TRUE;
}

/****************************************************************************
Public Function Definitions
To be used for the development of an operator client.
*/

/**
 * \brief Send a message to one of the owned operators.
 *        All of the target operators must be owned by this client otherwise
 *        the function will fail.
 *
 * \param op_data OPERATOR_DATA of the requesting operator client
 * \param target_op_id External operator id of the target operator
 * \param msg_length Length of the message to send
 * \param msg Contents of the message to send
 *
 * \return TRUE if the message has been successfully sent
 */
bool opmgr_op_client_send_message(OPERATOR_DATA *op_data,
                                  OPERATOR_ID target_op_id,
                                  unsigned msg_length,
                                  unsigned *msg)
{
    OP_CLIENT *client = op_client_with_op_id((INT_OP_ID)op_data->id);
    CONNECTION_LINK con_id;

    if (client == NULL)
    {
        return FALSE;
    }

    con_id = PACK_CON_ID(client->client_id, 0);
    opmgr_operator_message(con_id, target_op_id,
            msg_length, msg, op_client_send_message_cback);

    return TRUE;
}

/**
 * \brief Send a "start operator" request to a list of the owned operators.
 *        This will fail if the target operators are not owned by this client.
 *
 * \param op_data OPERATOR_DATA of the requesting operator client
 * \param num_ops Number of operators to start
 * \param op_list List of operators to start
 *
 * \return TRUE if the request has been successfully forwarded to opmgr
 */
bool opmgr_op_client_start_operator(OPERATOR_DATA *op_data,
                                    unsigned num_ops,
                                    OPERATOR_ID *op_list)
{
    OP_CLIENT *client = op_client_with_op_id((INT_OP_ID)op_data->id);
    CONNECTION_LINK con_id;

    if (client == NULL)
    {
        return FALSE;
    }

    con_id = PACK_CON_ID(client->client_id, 0);
    opmgr_start_operator(con_id, num_ops, op_list, op_client_operator_action_cback);

    return TRUE;
}


/**
 * \brief Send a "stop operator" request to a list of the owned operators.
 *        This will fail if the target operators are not owned by this client.
 *
 * \param op_data OPERATOR_DATA of the requesting operator client
 * \param num_ops Number of operators to stop
 * \param op_list List of operators to stop
 *
 * \return TRUE if the request has been successfully forwarded to opmgr
 */
bool opmgr_op_client_stop_operator(OPERATOR_DATA *op_data,
                                   unsigned num_ops,
                                   OPERATOR_ID *op_list)
{
    OP_CLIENT *client =  op_client_with_op_id((INT_OP_ID)op_data->id);
    CONNECTION_LINK con_id;

    if (client == NULL)
    {
        return FALSE;
    }

    con_id = PACK_CON_ID(client->client_id, 0);
    opmgr_stop_operator(con_id, num_ops, op_list, op_client_operator_action_cback);

    return TRUE;
}


/**
 * \brief Send a "reset operator" request to a list of the owned operators.
 *        This will fail if the target operators are not owned by this client.
 *
 * \param op_data OPERATOR_DATA of the requesting operator client
 * \param num_ops Number of operators to reset
 * \param op_list List of operators to reset
 *
 * \return TRUE if the request has been successfully forwarded to opmgr
 */
bool opmgr_op_client_reset_operator(OPERATOR_DATA *op_data,
                                    unsigned num_ops,
                                    OPERATOR_ID *op_list)
{
    OP_CLIENT *client = op_client_with_op_id((INT_OP_ID)op_data->id);
    CONNECTION_LINK con_id;

    if (client == NULL)
    {
        return FALSE;
    }

    con_id = PACK_CON_ID(client->client_id, 0);
    opmgr_reset_operator(con_id, num_ops, op_list, op_client_operator_action_cback);

    return TRUE;
}


/****************************************************************************
Private Function Definitions
*/

/**
 * \brief Create array with the (OPERATOR_DATA *) for each owned operator.
 *
 * \param client Operator client.
 * \param client_id Application client id: current owner of the operators.
 * \param num_owned_ops Number of operators owned by this operator client.
 * \param owned_op_array Array of the external operator ids to take ownership of.
 *
 * \return TRUE if the array has successfully been created.
 */
static bool op_client_create_owned_ops(OP_CLIENT *client,
                                       CONNECTION_PEER client_id,
                                       unsigned num_owned_ops,
                                       EXT_OP_ID *owned_op_array)
{
    unsigned i;
    OPERATOR_DATA *op_data;
    OP_CLIENT_OWNED **plist = &client->owned_ops;

    /* We want to add newly owned ops at the end of the list.
     * Traverse the list and get a pointer to its end. */
    while ((*plist) != NULL)
    {
        plist = &((*plist)->next);
    }

    for (i = 0; i < num_owned_ops; i++)
    {
        OP_CLIENT_OWNED * op;

        /* Allocate necessary memory. Freed in op_client_destoy. */
        op = xzpnew(OP_CLIENT_OWNED);
        if (op == NULL)
        {
            /* No memory. */
            return FALSE;
        }
        op->op_id = EXT_TO_INT_OPID(owned_op_array[i]);
        op->next = NULL;
        (*plist) = op;
        plist = &op->next;

        op_data = get_anycore_op_data_from_id(op->op_id);
        if (op_data == NULL)
        {
            /* An operator from the list was not found. */
            return FALSE;
        }
        /* Note that the operator may be running in the other processor. */
        if (client_id != GET_CON_ID_OWNER_CLIENT_ID(op_data->con_id))
        {
            /* The client is trying to delegate ownership of an operator it
             * doesn't own. */
            return FALSE;
        }
    }

    client->num_owned_ops += num_owned_ops;

    return TRUE;
}

/**
 * \brief Removes a delegated operator from an operator client's entry. If the
 *        operator client has no other delegated operators, delete the entry
 *        and unpromote the operator.
 *
 * \param client_id Application client id: owner of the operator client
 * \param op_id Internal operator ID
 *
 * \return Pointer to operator client entry.
 */
static bool op_client_remove_delegated_op(CONNECTION_PEER client_id, INT_OP_ID op_id)
{
    OP_CLIENT * client;
    OPERATOR_DATA *op_data;
    unsigned i;

    /* This patch point can also be used to patch interface APIs. */
    patch_fn_shared(op_client);

    /* Go through all the mappings and search con_id */
    for (i = 0; i < MAX_NUM_OP_CLIENT; i++)
    {
       client = op_client_db[i];
        if ((client != NULL) && client->valid)
        {
           OP_CLIENT_OWNED **pop = &client->owned_ops;
           while ((*pop) != NULL && (*pop)->op_id != op_id)
           {
               pop = &(*pop)->next;
           }
           if ((*pop) != NULL)
           {
               /* We found it. */
               op_data = get_anycore_op_data_from_id(client->op_id);
               if (client_id != GET_CON_ID_OWNER_CLIENT_ID(op_data->con_id))
               {
                   /* The client is trying to unpromote an operator that
                    * someone else owns. Not allowed. */
                   return FALSE;
               }
               else
               {
                   /* Remove it. */
                   OP_CLIENT_OWNED *op = *pop;
                   *pop = op->next;
                   pdelete(op);

                   client->num_owned_ops --;
                   if (client->owned_ops == NULL)
                   {
                       /* This client has no delegated operators now.
                        * Delete it.*/
                       op_client_delete(client);
                   }
                   return TRUE;
               }
           }
        }
    }
    /* Not found. */
    return FALSE;
}

/**
 * \brief Finds an operator client entry from the DB with specified op id.
 *
 * \param op_id Internal Operator ID
 *
 * \return Pointer to operator client entry.
 */
static OP_CLIENT *op_client_with_op_id(INT_OP_ID op_id)
{
    unsigned i;

    /* This patch point can also be used to patch interface APIs. */
    patch_fn_shared(op_client);

    /* Go through all the mappings and search op_id */
    for (i = 0; i < MAX_NUM_OP_CLIENT; i++)
    {
       if ((op_client_db[i] != NULL) && (op_client_db[i]->op_id == op_id))
       {
           if (op_client_db[i]->valid)
           {
               return op_client_db[i];
           }
           else
           {
               /*
                * Some race condition. The element is being created,
                * but is not ready yet. Can't really use it yet.
                */
               return NULL;
           }
       }
    }
    return NULL;
}

/**
 * \brief Finds an operator client entry from the DB with specified
 *        operator client id.
 *
 * \param client_id Operator Client ID
 *
 * \return Pointer to operator client entry.
 */
static OP_CLIENT *op_client_with_client_id(CONNECTION_PEER client_id)
{
    /* This patch point can also be used to patch callbacks. */
    patch_fn_shared(op_client);

    if (GET_SEND_RECV_ID_IS_SPECIAL_CLIENT(client_id))
    {
        unsigned idx = GET_SEND_RECV_ID_CLIENT_INDEX(client_id);
        /* This is actually the index of the entry in the db. */
        if (idx < MAX_NUM_OP_CLIENT)
        {
            OP_CLIENT *client = op_client_db[idx];
            if ((client != NULL) && client->valid)
            {
                return client;
            }
        }
    }
    return NULL;
}

/**
 * \brief Create new entry in the operator client DB for op_id.
 *        Fail if exists.
 *
 * \param op_id Internal Operator ID
 *
 * \return Pointer to operator client entry.
 */
static OP_CLIENT *op_client_new(INT_OP_ID op_id)
{
    unsigned idx = MAX_NUM_OP_CLIENT;
    unsigned i;

    for (i = 0; i < MAX_NUM_OP_CLIENT; i++)
    {
        /* Find an empty slot for a new operator client. */
        if (op_client_db[i] == NULL)
        {
            /* Found one. Save it if we haven't already. */
            if (idx == MAX_NUM_OP_CLIENT)
            {
                idx = i;
            }
        }
        else if (op_client_db[i]->op_id == op_id)
        {
            /* op_client already created. Must be destroyed and created again
             * if needed.
             */
            return NULL;
        }
    }

    if (idx >= MAX_NUM_OP_CLIENT)
    {
        /* Too many operator clients. */
        return NULL;
    }

    /* Memory will be freed in op_client_delete. */
    op_client_db[idx] = xzpnew(OP_CLIENT);
    if (op_client_db[idx] == NULL)
    {
        /* Malloc failed! */
        return NULL;
    }
    op_client_db[idx]->client_id = MAKE_SPECIAL_CLIENT_ID(idx);
    op_client_db[idx]->op_id = op_id;
    /* Any other field should be initialized before this line. */
    op_client_db[idx]->valid = TRUE;

    return op_client_db[idx];
}

/**
 * \brief Delete entry from the operator client DB.
 *
 * \param client Pointer to operator client entry.
 */
static void op_client_delete(OP_CLIENT *client)
{
    unsigned i = 0;

    if (client != NULL)
    {
        for (i = 0; i < MAX_NUM_OP_CLIENT; i++)
        {
            if (op_client_db[i] == client)
            {
                OP_CLIENT_OWNED *opi, *opx;
                op_client_db[i] = NULL;

                /* Now free the list of operators. */
                opi = client->owned_ops;
                while (opi != NULL)
                {
                    opx = opi;
                    opi = opi->next;
                    pdelete(opx);
                }
                /* Any other de-initialization should be done before this line. */
                pdelete(client);
                /* Client found and deleted. */
                return;
            }
        }
        /* client had just been found in op_client_db before this call.
         * If don't find it now, something has gone wrong. */
        L2_DBG_MSG1("Pointer to client addr 0x%x is not valid.", client);
#ifdef OP_CLIENT_DEBUG
        panic_diatribe(PANIC_AUDIO_OP_CLIENT_CORRUPTED, client);
#endif
    }
}

/**
 * \brief Send a message to the promoted operator with the list of the
 *        delegated operators.
 *
 * \param con_id Connection ID of this call.
 * \param client Pointer to operator client entry.
 */
static void op_client_notify_promotion(CONNECTION_LINK con_id,
                                       OP_CLIENT *client)
{
    unsigned i;
    unsigned *operators;
    OP_CLIENT_OWNED * op;
    unsigned len = client->num_owned_ops + OPMSG_OP_CLIENT_DELEGATED_OPERATORS_OPERATORS_WORD_OFFSET;
    /* Memory freed here. */
    unsigned *msg = pnewn(len, unsigned);

    OP_CLIENT_MSG_FIELD_SET(msg, OPMSG_OP_CLIENT_DELEGATED_OPERATORS,
                            MESSAGE_ID, OPMSG_OP_CLIENT_REPLY_ID_DELEGATED_OPERATORS);

    operators = OP_CLIENT_MSG_FIELD_POINTER_GET(msg, OPMSG_OP_CLIENT_DELEGATED_OPERATORS,
                                                OPERATORS);
    op = client->owned_ops;
    for (i=0; i < client->num_owned_ops && op != NULL; i++)
    {
        operators[i] = INT_TO_EXT_OPID(op->op_id);
        op = op->next;
    }
    if (i < client->num_owned_ops)
    {
        /* Something went wrong here! Op list got misaligned. */
        len -= (client->num_owned_ops - i);
        L2_DBG_MSG2("Operator client was supposed to have %d delegated operators, %d found instead.",
                    client->num_owned_ops, i);
    }
    opmgr_operator_message(con_id, INT_TO_EXT_OPID(client->op_id),
                            len, (unsigned *)msg,
                            op_client_empty_cback);
    pfree(msg);
}

/**
 * \brief Empty callback for messages we send to an operator client.
 *        The callback is empty because no one should handle any response.
 */
static bool op_client_empty_cback(CONNECTION_LINK con_id,
                                  STATUS_KYMERA status,
                                  unsigned ext_op_id,
                                  unsigned num_resp_params,
                                  unsigned *resp_params)
{
    return TRUE;
}

/**
 * \brief Callback to notify operator client about the result of an operator
 *        message it sent to one of its owned operators. This callback is
 *        actually called by the opmgr task: to notify the operator client,
 *        we will send to it a special operator message.
 */
static bool op_client_send_message_cback(CONNECTION_LINK reversed_con_id,
                                         STATUS_KYMERA status,
                                         unsigned ext_op_id,
                                         unsigned length,
                                         unsigned *resp_payload)
{
    CONNECTION_PEER client_id = GET_EXT_CON_ID_RECV_ID(reversed_con_id);
    OPERATOR_DATA *dst_op;
    tRoutingInfo new_rinfo;
    OP_CLIENT_MSG_RESPONSE *client_msg;
    unsigned len;

    /* Lookup the client operator */
    dst_op = op_client_get_client_op_data(client_id);
    if (dst_op == NULL)
    {
        /* Invalid connection id. The client might have been unpromoted.
         * Fail and ignore */
        return FALSE;
    }
    new_rinfo.src_id = client_id;

    /* The destination id is the opid (op client is always in P0) */
    new_rinfo.dest_id = dst_op->id;

    /* Allocate space for the client_msg, panic if no memory.
     * Memory freed once the receiver's opmgr_task_handler processes the msg.*/
    len = length + CLIENT_MSG_RESPONSE_SIZE;
    client_msg = (OP_CLIENT_MSG_RESPONSE *) pnewn(len, unsigned);

    /* fill in the client_msg fields */
    len = length + CLIENT_MSG_RESPONSE_SIZE_EXTRA;
    client_msg->header.cmd_header.client_id = client_id;
    client_msg->header.cmd_header.length = len;
    client_msg->header.msg_id = OPMSG_OP_CLIENT_REPLY_ID_MESSAGE_RESPONSE;
    client_msg->src_op_id = (EXT_OP_ID)ext_op_id;
    client_msg->status = status;

    if (length != 0)
    {
        memcpy(client_msg->payload, resp_payload, length*sizeof(unsigned));
    }

    /* Save the callback so we can use it when we get the response.
     * Retrieved in message_resp_handler. */
    if (!opmgr_store_in_progress_task(new_rinfo.src_id, dst_op->id,
                                        (void *)op_client_empty_cback))
    {
        /* If we couldn't save the in progress task data then panic */
        panic_diatribe(PANIC_AUDIO_OP_CLIENT_CALLBACK_FAILED, client_id);
    }

    put_message_with_routing(mkqid(dst_op->task_id, 1), OPCMD_MESSAGE, client_msg, &new_rinfo);

    return TRUE;
}

/**
 * \brief Callback to notify operator client about the result of an operator
 *        start/stop it sent to one of its owned operators. This callback is
 *        actually called by the opmgr task: to notify the operator client,
 *        we will send to it a special operator message.
 */
static bool op_client_operator_action_cback(CONNECTION_LINK reversed_con_id,
                                            STATUS_KYMERA status,
                                            unsigned count,
                                            unsigned err_code)
{
    CONNECTION_PEER client_id = GET_EXT_CON_ID_RECV_ID(reversed_con_id);
    OPERATOR_DATA *dst_op;
    tRoutingInfo new_rinfo;
    OP_CLIENT_CMD_RESPONSE *client_msg;
    unsigned len;

    /* Lookup the client operator */
    dst_op = op_client_get_client_op_data(client_id);
    if (dst_op == NULL)
    {
        /* Invalid connection id. The client might have been unpromoted.
         * Fail and ignore */
        return FALSE;
    }

    new_rinfo.src_id = client_id;

    /* The destination id is the opid (op client is always in P0) */
    new_rinfo.dest_id = dst_op->id;

    /* Allocate space for the client_msg, panic if no memory.
     * Memory freed once the receiver's opmgr_task_handler processes the msg. */
    client_msg = (OP_CLIENT_CMD_RESPONSE *) pnewn(CLIENT_CMD_RESPONSE_SIZE, unsigned);

    /* fill in the client_msg fields */
    len = OPMSG_OP_CLIENT_COMMAND_RESPONSE_WORD_SIZE;
    client_msg->header.cmd_header.client_id = client_id;
    client_msg->header.cmd_header.length = len;
    client_msg->header.msg_id = OPMSG_OP_CLIENT_REPLY_ID_COMMAND_RESPONSE;
    client_msg->count = count;
    client_msg->status = status;
    client_msg->err_code = err_code;

    /* Save the callback so we can use it when we get the response.
     * Retrieved in message_resp_handler. */
    if (!opmgr_store_in_progress_task(new_rinfo.src_id, dst_op->id,
                                        (void *)op_client_empty_cback))
    {
        /* If we couldn't save the in progress task data then panic */
        panic_diatribe(PANIC_AUDIO_OP_CLIENT_CALLBACK_FAILED, client_id);
    }

    put_message_with_routing(mkqid(dst_op->task_id, 1), OPCMD_MESSAGE, client_msg, &new_rinfo);

    return TRUE;
}

/**
 * \brief Get the operator data of the operator client with given client_id.
 *        This is groups common operations from the message handling APIs.
 *        It return NULL in case of a recoverable or transient error.
 *        In case of a non recoverable error, it panics.
 */
static OPERATOR_DATA* op_client_get_client_op_data(CONNECTION_PEER client_id)
{
    OP_CLIENT *client;
    OPERATOR_DATA* dst_op;

    client = op_client_with_client_id(client_id);
    if (client == NULL)
    {
        /* Invalid connection id. The client might have been unpromoted.
         * Fail or ignore */
        return NULL;
    }

    dst_op = get_anycore_op_data_from_id(client->op_id);
    if (dst_op == NULL)
    {
        /* The saved data for this client is not valid.
         * This is unexpected and should never happen.
         * A possible cause is that the operator client has been destroyed
         * without having cancelled the delegation first. */
        EXT_OP_ID ext_op_id = INT_TO_EXT_OPID(client->op_id);
        L2_DBG_MSG1("Client with op id 0x%04x is not a valid operator", ext_op_id);
#ifdef OP_CLIENT_DEBUG
        panic_diatribe(PANIC_AUDIO_OP_CLIENT_CORRUPTED, ext_op_id);
#endif
    }
    return dst_op;
}
