/****************************************************************************
 * Copyright (c) 2013 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  opmgr_kip.c
 * \ingroup  opmgr
 *
 * Operator Manager KIP-related parts. <br>
 */

/****************************************************************************
Include Files
*/

#include "opmgr_private.h"
#include "kip_msg_prim.h"
#include "adaptor/kip/kip_msg_adaptor.h"
#include "patch.h"

/****************************************************************************
Variable definitions
*/

OPMGR_CREATE_REQ_KEYS create_req_keys;

/****************************************************************************
Function Definitions
*/

/* Free the memory allocated in create_req_keys. */
void opmgr_kip_free_req_keys(void)
{
    if (create_req_keys.num_keys > 0)
    {
        pdelete(create_req_keys.ex_info);
        create_req_keys.ex_info = NULL;
        create_req_keys.num_keys = 0;
    }
}

/* Send 'list' type command via KIP: stop, reset, start, destroy */
bool opmgr_kip_build_send_list_cmd(CONNECTION_LINK con_id,
                                   unsigned num_ops,
                                   unsigned* op_list,
                                   KIP_MSG_ID kip_msg_id,
                                   void* callback)
{
    bool success = FALSE;
    unsigned length;
    uint16* msg_data;

    length = KIP_MSG_OPLIST_CMD_REQ_OP_LIST_WORD_OFFSET + num_ops;
    msg_data = xpnewn(length, uint16);
    if (msg_data != NULL)
    {
        msg_data[KIP_MSG_OPLIST_CMD_REQ_CON_ID_WORD_OFFSET] = (uint16)con_id;
        msg_data[KIP_MSG_OPLIST_CMD_REQ_COUNT_WORD_OFFSET] = (uint16)num_ops;

        unsigned opcount;
        for(opcount=0; opcount < num_ops; opcount++)
        {
            msg_data[KIP_MSG_OPLIST_CMD_REQ_OP_LIST_WORD_OFFSET + opcount] = (uint16)op_list[opcount];
        }

        success = kip_adaptor_send_message(con_id, kip_msg_id, length,
                                           msg_data, callback);

        pfree(msg_data);
    }

    return success;
}

/**
 * \brief    Send create operator REQ to adaptor
 *
 * \param    con_id  Connection id
 * \param    cap_id  Capability id
 * \param    op_id  The created operator id
 * \param    create_req_keys  the required key information (such as
 *                            priority, processor id) for creating this operator
 * \param    callback  The callback function of create operator request
 *
 */
bool opmgr_kip_build_send_create_op_req(CONNECTION_LINK con_id,
                                        unsigned cap_id,
                                        unsigned op_id,
                                        OPMGR_CREATE_REQ_KEYS *create_req_keys,
                                        void* callback)
{
    bool success = FALSE;
    unsigned length;
    uint16* msg_data;

    patch_fn_shared(kip);

    /* Send KIP message to create it on remote processor. The KIP response will lead to
     * the API callback being called, so use some housekeeping there to call the callback when response comes back.
     */
    length = KIP_MSG_CREATE_OPERATOR_REQ_INFO_WORD_OFFSET +
             create_req_keys->num_keys * sizeof(OPERATOR_CREATE_EX_INFO)/sizeof(uint16);
    msg_data = xzpnewn(length, uint16);
    if (msg_data != NULL)
    {
        msg_data[KIP_MSG_CREATE_OPERATOR_REQ_CON_ID_WORD_OFFSET]        = (uint16)con_id;
        msg_data[KIP_MSG_CREATE_OPERATOR_REQ_CAPABILITY_ID_WORD_OFFSET] = (uint16)cap_id;
        msg_data[KIP_MSG_CREATE_OPERATOR_REQ_OP_ID_WORD_OFFSET]         = (uint16)op_id;
        msg_data[KIP_MSG_CREATE_OPERATOR_REQ_NUM_KEYS_WORD_OFFSET] = (uint16)create_req_keys->num_keys;

        /* Copy the keys */
        OPERATOR_CREATE_EX_INFO* msg_ex_info = (OPERATOR_CREATE_EX_INFO*)&msg_data[KIP_MSG_CREATE_OPERATOR_REQ_INFO_WORD_OFFSET];
        unsigned count;

        for(count=0; count < create_req_keys->num_keys; count++)
        {
            msg_ex_info[count].key = create_req_keys->ex_info[count].key;
            msg_ex_info[count].value = create_req_keys->ex_info[count].value;
        }

        success = kip_adaptor_send_message(con_id, KIP_MSG_ID_CREATE_OPERATOR_REQ,
                                           length, msg_data, callback);

        pfree(msg_data);
    }

    return success;
}

/**
 * \brief    send operator message to KIP
 *
 * \param    con_id      Connection id
 * \param    op_id       The created operator id
 * \param    num_params  Length of parameters in message
 * \param    *params     Pointer to the Parameters
 * \param    callback    The callback function of operator message
 *
 */
bool opmgr_kip_build_send_opmsg(CONNECTION_LINK con_id,
                                unsigned op_id,
                                unsigned num_params,
                                unsigned* params,
                                void* callback)
{
    bool success = FALSE;
    unsigned length;
    uint16* kip_msg;

    patch_fn_shared(kip);

    length = KIP_MSG_OPERATOR_MESSAGE_REQ_OP_MESSAGE_WORD_OFFSET + num_params;
    kip_msg = xpnewn(length, uint16);
    if (kip_msg != NULL)
    {
        /* Conid already has remote processor ID added by ACCMD adaptor */
        kip_msg[KIP_MSG_OPERATOR_MESSAGE_REQ_CON_ID_WORD_OFFSET] = (uint16)con_id;
        kip_msg[KIP_MSG_OPERATOR_MESSAGE_REQ_OPID_WORD_OFFSET]   = (uint16)op_id;

        adaptor_pack_list_to_uint16(&kip_msg[KIP_MSG_OPERATOR_MESSAGE_REQ_OP_MESSAGE_WORD_OFFSET], params, num_params);

        /* Callback provided for return/response path, so a response from Pn will lead to calling this
         * external API callback to respond to client.
         */
        success = kip_adaptor_send_message(con_id, KIP_MSG_ID_OPERATOR_MSG_REQ,
                                           length, kip_msg, callback);

        pfree(kip_msg);
    }

    return success;
}

/***************** Request handler functions *********************/

/**
 * \brief    Handle the create_operator_ex request from KIP adaptor
 *
 * \param    con_id      Connection id
 * \param    cap_id      Capability id
 * \param    op_id       The created operator id
 * \param    num_keys    The number of keys
 * \param    *info       Key information
 * \param    callback    The callback function of create operator for KIP
 */
void opmgr_kip_create_operator_ex_req_handler(CONNECTION_LINK con_id,
                                              CAP_ID cap_id,
                                              unsigned int op_id,
                                              unsigned int num_keys,
                                              OPERATOR_CREATE_EX_INFO *info,
                                              OP_CREATE_CBACK callback)
{
    opmgr_create_operator_ex(con_id, cap_id, op_id, num_keys, info, callback);
}
/**
 * \brief    Handle the operator message request from kip adaptor
 *
 * \param    con_id      Connection id
 * \param    op_id       The created operator id
 * \param    num_params  Length of parameters in operator message
 * \param    *params     Parameters in the operator message
 * \param    callback    The callback function of operator message
 */
void opmgr_kip_operator_message_req_handler(CONNECTION_LINK con_id,
                                            STATUS_KYMERA status,
                                            unsigned op_id,
                                            unsigned num_param,
                                            unsigned *params,
                                            OP_MSG_CBACK callback)
{
    patch_fn_shared(kip);

    if (status == STATUS_OK)
    {
        opmgr_operator_message(con_id, op_id, num_param, params, callback);
    }
    else
    {
        callback(con_id, status, 0, 0, NULL);
    }
}

/***************** Response handler functions *********************/

/**
 * \brief    Handle the create operator response from kip
 *
 * \param    con_id  Connection id
 * \param    status  Status of the request
 * \param    op_id  The created operator id
 * \param    callback  The callback function of create operator
 */
void opmgr_kip_create_resp_handler(CONNECTION_LINK con_id,
                                   STATUS_KYMERA status,
                                   unsigned op_id,
                                   OP_CREATE_CBACK callback)
{
    patch_fn_shared(kip);

    PL_ASSERT(NULL != callback);

    if (status != STATUS_OK)
    {
        /* Remove the given operator's data structure from the remote operator list */
        remove_op_data_from_list(EXT_TO_INT_OPID(op_id), &remote_oplist_head);
    }

    callback(con_id, status, op_id);
}

/**
 * \brief    Handle the std list operator (start/stop/reset/destroy) response from kip
 *
 * \param    con_id    Connection id
 * \param    status    Status of the request
 * \param    count     Number of operators handled successfully
 * \param    err_code  Error code upon operator error
 * \param    callback  The callback function of create operator
 */
void opmgr_kip_stdlist_resp_handler(CONNECTION_LINK con_id,
                                    STATUS_KYMERA status,
                                    unsigned count,
                                    unsigned err_code,
                                    OP_STD_LIST_CBACK callback)
{
    patch_fn_shared(kip);

    if (callback != NULL)
    {
        callback(con_id, status, count, err_code);
    }
    else
    {
        PL_PRINT_P0(TR_DUALCORE, "No context for KIP_MSG_ID_X_OPERATOR_RES. (X = DESTROY or START or STOP or RESET)\n");
    }
}

/**
 * \brief    Handle the operator message response from kip
 *           This is used in the KIP message handler on P0, where it
 *           receives operator messages responses from P1 that must
 *           be forwarded to the originator (apps0 typically).
 *
 * \param    con_id           Connection id
 * \param    status           Status of the request
 * \param    op_id            The created operator id
 * \param    num_resp_params  Length of parameters in response message
 * \param    *resp_params     Parameters in response message
 * \param    callback         The callback function of operator message from kip
 */
void opmgr_kip_operator_msg_resp_handler(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         unsigned op_id,
                                         unsigned num_resp_params,
                                         unsigned *resp_params,
                                         void *context)
{
    patch_fn_shared(kip);

    OP_MSG_CBACK callback = (OP_MSG_CBACK)context;

    if (callback != NULL)
    {
        /* Just forward the whole response, unalterated */
        callback(con_id, status, op_id, num_resp_params, resp_params);
    }
    else
    {
        PL_PRINT_P0(TR_DUALCORE, "No context for KIP_MSG_ID_OPERATOR_MSG_RES.\n");
    }
}

/**
 * \brief    Send the unsolicited message to KIP
 *
 * \param    con_id          Connection id
 * \param    *msg_from_op    Unsolicited messsage from the operator
 */
bool opmgr_kip_unsolicited_message(CONNECTION_LINK con_id,
                                   OP_UNSOLICITED_MSG *msg)
{
    bool success = FALSE;
    unsigned kip_len;
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ* kip_msg;
    uint16* payload;

    patch_fn_shared(kip);

    /* Allocate the KIP message. */
    kip_len = KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MESSAGE_WORD_OFFSET +
              msg->length;
    kip_msg = (KIP_MSG_MESSAGE_FROM_OPERATOR_REQ*) xpnewn(kip_len, uint16);
    if (kip_msg == NULL)
    {
        return FALSE;
    }

    /* Serialize the message. */
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_EXT_OP_ID_SET(kip_msg, msg->op_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_CLIENT_ID_SET(kip_msg, msg->client_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MSG_ID_SET(kip_msg, msg->msg_id);
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_LENGTH_SET(kip_msg, msg->length);
    payload = ((uint16 *) kip_msg) +
              KIP_MSG_MESSAGE_FROM_OPERATOR_REQ_MESSAGE_WORD_OFFSET;
    adaptor_pack_list_to_uint16(payload, (unsigned*) &msg->payload[0], msg->length);

    /* And send it to the primary processor. */
    success = kip_adaptor_send_message(REVERSE_CONNECTION_ID(con_id),
                                       KIP_MSG_ID_UNSOLICITED_FROM_OP_REQ,
                                       kip_len, (uint16*) kip_msg, NULL);
    pfree(kip_msg);

    return success;
}
