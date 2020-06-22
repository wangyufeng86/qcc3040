/****************************************************************************
 * Copyright (c) 2011 - 2017 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_kip.c
 * \ingroup stream
 *
 * Private Stream KIP elements. <br>
 *
 */

/****************************************************************************
Include Files
*/

#include "types.h"
#include "proc/proc.h"
#include "ipc/ipc.h"
#include "kip_mgr/kip_mgr.h"
#include "kip/kip_msg_adaptor.h"
#include "stream_kip.h"
#include "stream/stream_private.h"
#include "buffer/buffer_metadata.h"
#include "patch.h"

#include "buffer_metadata_kip.h"

/****************************************************************************
Private Type Declarations
*/

/**
 * KIP state is maintained only if there are multiple IPC interactions and/or
 * multiple KIP messages involved to complete once client request over KIP, that
 * requires preserving context.
 */
typedef enum
{
    STREAM_KIP_STATE_NONE = 0,
    STREAM_KIP_STATE_CONNECT,
    STREAM_KIP_STATE_DISCONNECT
} STREAM_KIP_STATE;

typedef struct
{
    STREAM_KIP_STATE        state;
    union
    {
        void                                 *any_info;
        STREAM_KIP_CONNECT_INFO              *connect_info;
        STREAM_KIP_TRANSFORM_DISCONNECT_INFO *disconnect_info;
    } context;
} STREAM_KIP_STATE_INFO;

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Private Macro Declarations
*/
#define STREAM_KIP_STATE_IN_IDLE(x) (stream_kip_state.state == STREAM_KIP_STATE_NONE)
#define STREAM_KIP_STATE_IN_CONNECT(x) (stream_kip_state.state == STREAM_KIP_STATE_CONNECT)
#define STREAM_KIP_STATE_IN_DISCONNECT(x) (stream_kip_state.state == STREAM_KIP_STATE_DISCONNECT)


static bool stream_kip_get_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                            PROC_ID_NUM proc_id);
static bool stream_kip_set_ep_meta_info(STREAM_KIP_CONNECT_INFO *state,
                                        bool support_metadata);

#define KIP_METADATA_BUFFER_SIZE (128)

/**
 * Always use the highest channel number in IPC for metadata. -1 because
 * channel numbers start from zero.
 */
#define META_DATA_CHANNEL_NUM (IPC_MAX_DATA_CHANNELS-1)
#define DATA_CHANNEL_IS_META(id) (((id) & META_DATA_CHANNEL_NUM) == META_DATA_CHANNEL_NUM)
#define BOTH_CHANNELS_ARE_ACTIVATED(s) ((s)->data_channel_is_activated && (s)->metadata_channel_is_activated)


/****************************************************************************
Private Variable Definitions
*/

/*
 * There are some instances, there may be multiple KIP messages
 * or IPC activities to complete one client request. especially
 * while activating or deactivating the data channels which
 * requires to store local context and continue the sequence.
 * No other kip messages are entertained during that time.
 */
STREAM_KIP_STATE_INFO stream_kip_state = {STREAM_KIP_STATE_NONE, {NULL}};

/**
 * This keeps information about the remote transforms. On P0, this list contains
 * a copy of P1 transforms. On P1, it contains all the transform created on P1.
 * All P0 related transform local transforms are maintained in its transform_list.
 */
STREAM_KIP_TRANSFORM_INFO *kip_transform_list = NULL;

/****************************************************************************
Private Function Declarations
*/

/**
 * \brief Set KIP state to 'not active'.
 *
 * \param free_context - Flag that indicates whether to free state context
 */
static void stream_kip_state_to_none(bool free_context)
{
    if ((stream_kip_state.context.any_info != NULL) && free_context)
    {
        /* Free the context only requested */
        pfree(stream_kip_state.context.any_info);
    }

    stream_kip_state.context.any_info = NULL;
    stream_kip_state.state            = STREAM_KIP_STATE_NONE;
}

/**
 * \brief Destroy endpoints of a given state.
 *
 * \param state         - KIP connection state information
 */
static void stream_kip_destroy_endpoint_ids(STREAM_KIP_CONNECT_INFO *state)
{
    if (state != NULL)
    {
        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
        if ((state->ep_location == STREAM_EP_REMOTE_SINK) &&
            (state->data_channel_id != 0))
        {
            ipc_destroy_data_channel(state->data_channel_id);
        }
    }
}

/**
 * \brief Set KIP state to connected.
 *
 * \param info          - Context to remember for this state
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_state_to_connect(STREAM_KIP_CONNECT_INFO *info)
{
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state.context.connect_info = info;
        stream_kip_state.state = STREAM_KIP_STATE_CONNECT;
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get KIP connect state info.
 *
 * \return Returns state's connect info.
 */
static inline STREAM_KIP_CONNECT_INFO *stream_kip_state_get_connect_info(void)
{
    return stream_kip_state.context.connect_info;
}

/**
 * \brief Set KIP state to disconnected.
 *
 * \param info          - Context to remember for this state
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_state_to_disconnect(STREAM_KIP_TRANSFORM_DISCONNECT_INFO *info)
{
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        stream_kip_state.context.disconnect_info = info;
        stream_kip_state.state = STREAM_KIP_STATE_DISCONNECT;
        return TRUE;
    }
    return FALSE;
}

/**
 * \brief Get KIP disconnect state info.
 *
 * \return Returns state's disconnect info.
 */
static inline STREAM_KIP_TRANSFORM_DISCONNECT_INFO *stream_kip_state_get_disconnect_info(void)
{
    return stream_kip_state.context.disconnect_info;
}

/**
 * \brief Update KIP connection state information.
 *
 * \param state            - KIP connect state information
 * \param channel_id       - The data channel id for this state
 * \param buffer_size      - The buffer size for this state
 * \param flags            - The flags for this state
 */
static inline void stream_kip_update_buffer_info(STREAM_KIP_CONNECT_INFO *state,
                                                 unsigned channel_id,
                                                 unsigned buffer_size,
                                                 unsigned flags)
{
    union _parter_buffer_config_flags
    {
       struct BUFFER_CONFIG_FLAGS bcf;
       unsigned flags;
    } parter_buffer_config_flags;

    parter_buffer_config_flags.flags = flags;
    state->data_channel_id  = (uint16)channel_id;
    state->connect_info.buffer_info.buffer_size = buffer_size;

    /* Update the flag for supports_metadata only */
    state->connect_info.buffer_info.flags.supports_metadata
                            = parter_buffer_config_flags.bcf.supports_metadata;

    /* Currently most flags are ignored and shadow endpoints uses pre-defined
     * flags. Only the 'source_wants_kicks' and 'sink_wants_kicks' flags need
     * to be copied, for adc->P1 and P1->dac transfers.
     * If any other flags need to be configured in the stream shadow
     * endpoint, it needs to call the config() handler to do so.
     *
     */
    UNUSED(parter_buffer_config_flags);
}

/**
 * \brief Get KIP endpoint from connect state
 *
 * \param state            - KIP connect state information
 *
 * \return Pointer to KIP connection state's remote endpoint
 */
static ENDPOINT* stream_kip_get_kip_endpoint_from_state(STREAM_KIP_CONNECT_INFO *state)
{
    ENDPOINT *ep = NULL;

    if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
    {
        ep = stream_endpoint_from_extern_id(state->source_id);
    }
    else if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        ep = stream_endpoint_from_extern_id(state->sink_id);
    }

    return ep;
}

/**
 * \brief Get the non-KIP endpoint from connect state
 *
 * \param state            - KIP connect state information
 *
 * \return Pointer to KIP connection state's local endpoint
 */
static inline ENDPOINT* stream_kip_get_local_endpoint_from_state(STREAM_KIP_CONNECT_INFO *state)
{
    ENDPOINT *ep;

    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        ep = stream_endpoint_from_extern_id(state->source_id);
    }
    else
    {
        ep = stream_endpoint_from_extern_id(state->sink_id);
    }

    return ep;
}

/**
 * \brief Get buffer and activate data channel. This function is being
 *        called when the endpoint is EP_REMOTE_SINK.
 *
 * \param state            - Connect state information
 * \param proc_id          - Processor ID
 *
 * \return TRUE on success
 */
static bool kip_get_buffer_and_activate_channels(STREAM_KIP_CONNECT_INFO *state,
                                                 PROC_ID_NUM proc_id)
{
    bool result;
    uint32 source_data_format;
    ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
    ENDPOINT *sink_ep   = stream_endpoint_from_extern_id(state->sink_id);

    /* Get the data format for the data channel from the source*/
    if (!stream_get_endpoint_config(source_ep,
                                    EP_DATA_FORMAT,
                                    &source_data_format))
    {
        return FALSE;
    }

    /* Configure the required buffer size */
    sink_ep->functions->configure(sink_ep,
                                  EP_SET_SHADOW_BUFFER_SIZE,
                                  state->connect_info.buffer_info.buffer_size);

    /* Configure the data format */
    sink_ep->functions->configure(sink_ep,
                                  EP_DATA_FORMAT,
                                  source_data_format);

    /* Get the connect buffer and activate data channel */
    result = (stream_connect_get_buffer(source_ep, sink_ep, &state->connect_info) &&
            (IPC_SUCCESS == ipc_activate_data_channel(state->data_channel_id,
                                                      proc_id,
                                                      state->connect_info.buffer_info.buffer, TRUE,
                                                      2, &source_data_format)));

    if (state->connect_info.buffer_info.flags.supports_metadata && result)
    {
        if (!stream_kip_get_metadata_channel(state, proc_id))
        {
#ifdef STREAM_CONNECT_FAULT_CODE
            stream_connect_fault = SC_KIP_GET_META_CHANNEL_FAILED;
            L1_DBG_MSG("stream_connect_fault: Failed to get_metadata_channel");
#endif /* STREAM_CONNECT_FAULT_CODE */
            /* Disable metadata support */
            stream_kip_set_ep_meta_info(state, FALSE);
        }
    }

    return result;
}

/**
 * \brief Internal function to the local endpoint buffer details.
 *
 * \param state            - KIP connect state information
 * \param buf_details      - Pointer to return the buffer details
 *
 * \return TRUE on successfully retrieving the buffer details
 */
static bool stream_kip_get_local_ep_buffer_details(STREAM_KIP_CONNECT_INFO *state,
                                                   BUFFER_DETAILS *buf_details)
{
    ENDPOINT *ep;
    ep = stream_kip_get_local_endpoint_from_state(state);

    STREAM_KIP_ASSERT(ep != NULL);

    /* Get the local endpoint buffer details */
    return ep->functions->buffer_details(ep, buf_details);
}

/**
 * \brief Internal function to get buffer info for remote information,
 *        the buffer info is stored in the KIP connect info object.
 *
 * \param state            - KIP connect state information
 *
 * \return TRUE on successfully retrieving the buffer info
 */
static bool stream_kip_ep_get_buffer_info(STREAM_KIP_CONNECT_INFO *state)
{
    BUFFER_DETAILS buffer_details;
    unsigned buffer_size;
    STREAM_TRANSFORM_BUFFER_INFO* info = &state->connect_info.buffer_info;

    /* Get the local endpoint buffer details */
    if (!stream_kip_get_local_ep_buffer_details(state, &buffer_details))
    {
        return FALSE;
    }

    /*
     * Any flags that requires sharing with the remote
     * must be set here.
     */

    /* set the buffer size */
    buffer_size = get_buf_size(&buffer_details);

    info->flags.supports_metadata = buffer_details.supports_metadata;

    if (buffer_size > info->buffer_size)
    {
        info->buffer_size = buffer_size;
    }

    return TRUE;
}

/**
 * \brief Find an existing transform created using the same terminal group belongs
 *        to the provided sink and source.
 *
 * \param source id  - The source endpoint id
 * \param sink id    - The sink endpoint id
 *
 * \return Any KIP transform info already present with the same base endpoint ids
 */
static STREAM_KIP_TRANSFORM_INFO *stream_kip_get_created_transform(unsigned source_id, unsigned sink_id)
{
    STREAM_KIP_TRANSFORM_INFO *kip_tr = kip_transform_list;

    /* figure out what should be the best base id for search */
    bool source_id_is_base = STREAM_EP_IS_REALEP_ID(sink_id) ?
                                        TRUE : STREAM_EP_IS_OPEP_ID(source_id);

    patch_fn_shared(stream_kip);

    while (kip_tr != NULL)
    {
        /* Either sink or source id must be a KIP endpoint to compare. Otherwise next transform */
        if (STREAM_EP_IS_SHADOW_ID(kip_tr->source_id) || STREAM_EP_IS_SHADOW_ID(kip_tr->sink_id))
        {
            if (source_id_is_base)
            {
                /* Check the base ids. if the source ids are matching and either both sink ids are
                 * real endpoints or both sink id bases are matching, then we found it.
                 */
                if ((GET_BASE_EPID_FROM_EPID(kip_tr->source_id) ==  GET_BASE_EPID_FROM_EPID(source_id))  &&
                    ((STREAM_EP_IS_REALEP_ID(sink_id) && kip_tr->real_sink_ep) ||
                    (GET_BASE_EPID_FROM_EPID(kip_tr->sink_id) == GET_BASE_EPID_FROM_EPID(sink_id))))
                {
                    /* This is another channel connecting same operators */
                    break;
                }
            }
            else
            {
                /* Check the base ids. if the sink base ids are matching and either both source ids are
                 * real endpoints or both source id bases are matching, then we found it.
                 */
                if ((GET_BASE_EPID_FROM_EPID(kip_tr->sink_id) ==  GET_BASE_EPID_FROM_EPID(sink_id))  &&
                    ((STREAM_EP_IS_REALEP_ID(source_id) && kip_tr->real_source_ep) ||
                    (GET_BASE_EPID_FROM_EPID(kip_tr->source_id) == GET_BASE_EPID_FROM_EPID(source_id))))
                {
                    /* This is another channel connecting same operators */
                    break;
                }
            }
        }
        kip_tr = kip_tr->next;
    }

    return kip_tr;
}

/**
 * \brief Internal function to get a used port by another endpoint of the
 *        of the same operator. It searches the kip transform list to find it.
 *
 * \param source id    - The source id
 * \param sink id      - sink id
 *
 * \return 0 if not found, otherwise port id (> 0).
 */
static uint16 stream_kip_get_used_port(unsigned source_id, unsigned sink_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = NULL;
    uint16 port_id = 0;

    tr = stream_kip_get_created_transform(source_id, sink_id);

    if (tr != NULL)
    {
        /* we got the details */
        port_id = ipc_get_data_channelid_port(tr->data_channel_id);
    }

    return port_id;
}

/**
 * \brief Update the metadata information of a KIP endpoint
 *
 * \param state            - Connect state information
 * \param support_metadata - Flag to indicate whether to support metadata
 *
 * \return  TRUE on success
 */
static bool stream_kip_set_ep_meta_info(STREAM_KIP_CONNECT_INFO *state,
                                        bool support_metadata)
{
    ENDPOINT *kip_ep = stream_kip_get_kip_endpoint_from_state(state);

    STREAM_KIP_ASSERT(kip_ep != NULL);

    /* update the meta data channel id in KIP endpoint */
    return (kip_ep->functions->configure(kip_ep, EP_METADATA_SUPPORT,
                                         support_metadata));
}

/**
 * \brief Check if both endpoints support metadata
 *
 * \param state       - Connect state information
 *
 * \return  TRUE on success
 */
static bool stream_kip_endpoints_support_metadata(STREAM_KIP_CONNECT_INFO *state)
{
    BUFFER_DETAILS buffer_details;
    ENDPOINT *local_ep = stream_kip_get_local_endpoint_from_state(state);

    patch_fn_shared(stream_kip);

    if (!local_ep->functions->buffer_details(local_ep, &buffer_details))
    {
        return FALSE;
    }

    if ((state->connect_info.buffer_info.flags.supports_metadata) &&
        (buffer_details.supports_metadata))
    {
        /* Update the metadata information for the KIP endpoint */
        if (!stream_kip_set_ep_meta_info(state, buffer_details.supports_metadata))
        {
            return FALSE;
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * \brief Update the metadata buffer information based on previous connections
 *
 * \param state       - State information
 */
static void stream_kip_update_metadata_buffer(STREAM_KIP_CONNECT_INFO *state)
{
    patch_fn_shared(stream_kip);

    ENDPOINT *cur_kip_ep = stream_kip_get_kip_endpoint_from_state(state);

    STREAM_KIP_ASSERT(cur_kip_ep != NULL);

    /* Check if we have got a synchronised connection in the same base of source ep and sink ep */
    STREAM_KIP_TRANSFORM_INFO *kip_tr = stream_kip_get_created_transform(state->source_id, state->sink_id);

    if (kip_tr != NULL)
    {
        cur_kip_ep->functions->configure(cur_kip_ep, EP_METADATA_CHANNEL_ID, state->meta_channel_id);
    }
}

/**
 * \brief Get buffer and activate data channel
 *
 * \param state       - State information
 * \param proc_id     - Processor ID
 *
 * \return TRUE on success
 */
static bool stream_kip_activate_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                                 PROC_ID_NUM proc_id)
{
    uint32 source_data_format;

    ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
    ENDPOINT *sink_ep   = stream_endpoint_from_extern_id(state->sink_id);
    ENDPOINT *kip_ep    = STREAM_EP_GET_SHADOW_EP(source_ep, sink_ep);

    patch_fn_shared(stream_kip);

    /* Get the data format for the data channel from the source*/
    if (!stream_get_endpoint_config(source_ep, EP_DATA_FORMAT, &source_data_format))
    {
        return FALSE;
    }

    sink_ep->functions->configure(sink_ep, EP_DATA_FORMAT, source_data_format);

    /*
     * In addition to tags we need to synchronise a few more values across
     * cores. We have therefore extended the tCbuffer structure with a few extra
     * fields that are only needed by the KIP layer. This is allocated here in
     * KIP so to the rest of the system this structure is an ordinary cbuffer.
     *
     * The reason we're creating a buffer first and then delete it after a copy
     * is limitations in cbuffer API which always allocates the cbuffer
     * structure internally and doesn't allow us to allocate the structure and
     * pass it as a pointer.
     */
    tCbuffer *shared_metadata_buf = cbuffer_create_shared_with_malloc(KIP_METADATA_BUFFER_SIZE,
                                                                      BUF_DESC_SW_BUFFER);

    KIP_METADATA_BUFFER* cbuffer_extra = xzppnew(KIP_METADATA_BUFFER,
                                                 MALLOC_PREFERENCE_SHARED);

    if ((cbuffer_extra == NULL) || (shared_metadata_buf) == NULL)
    {
        return FALSE;
    }

    /* Swap the buffer with the extended version */
    cbuffer_extra->parent = *shared_metadata_buf;
    cbuffer_destroy_struct(shared_metadata_buf);
    shared_metadata_buf = &(cbuffer_extra->parent);

    stream_shadow_set_shared_metadata_buffer(kip_ep, shared_metadata_buf);

    return (IPC_SUCCESS == ipc_activate_data_channel(state->meta_channel_id,
                                                     proc_id,
                                                     shared_metadata_buf, FALSE,
                                                     2, &source_data_format));
}

/**
 * \brief Internal function to retrieve timing information for partner
 *        shadow endpoint on other core from this processor's endpoint.
 *
 * \param state       - Current KIP state or context to use
 */
static void stream_kip_get_timing(STREAM_KIP_CONNECT_INFO *state)
{
    uint16 epid;
    bool   loc_clocked;
    bool   wants_kicks;

    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        epid = state->source_id;
    }
    else
    {
        epid = state->sink_id;
    }

    if (STREAM_EP_IS_OPEP_ID(epid))
    {
        opmgr_get_sched_info(epid, &state->tinfo_block_size, &state->tinfo_period, &loc_clocked, &wants_kicks);
    }
    else
    {
        /*
         * Real endpoint (shadow endpoint is connected to
         * either operator endpoint or a real endpoint)
         */
        state->tinfo_block_size = stream_shadow_get_block_size_default();
        state->tinfo_period     = stream_shadow_get_period_default();
    }
}

/**
 * \brief Internal function to update timing information from partner
 *        endpoint on other core to this processor's shadow endpoint.
 *
 * \param state       - Current KIP state or context to use
 */
static void stream_kip_set_timing(STREAM_KIP_CONNECT_INFO *state)
{
    uint16 epid;

    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        epid = state->sink_id;
    }
    else
    {
        epid = state->source_id;
    }

    ENDPOINT *ep = stream_endpoint_from_extern_id(epid);
    if (ep != NULL)
    {
        ep->functions->configure(ep, EP_SET_SHADOW_TINFO_BLOCKSIZE, state->tinfo_block_size);
        ep->functions->configure(ep, EP_SET_SHADOW_TINFO_PERIOD   , state->tinfo_period);
    }
}

/**
 * \brief Internal function to create endpoints and get buffer details
 *        while creating the kip endpoints
 *
 * \param packed_con_id  - Packed connection id
 * \param state          - Current KIP state or context to use if and
 *                         when a KIP reply comes back from P0.
 *
 * \return  TRUE on success
 */
static bool stream_kip_create_eps_for_connect(CONNECTION_LINK packed_con_id,
                                              STREAM_KIP_CONNECT_INFO *state)
{
    patch_fn_shared(stream_kip);

    bool result = TRUE;

    /* create data channel if local source */
    if (state->ep_location == STREAM_EP_REMOTE_SINK)
    {
        uint16 ch_id = state->data_channel_id;

        /* ch_id may contain the proposed channel id from the remote. If it was
         * not proposed, generate one
         */
        if (ch_id == 0)
        {
            ch_id = (uint16)((STREAM_EP_IS_OPEP_ID(state->source_id)) ?
                           GET_TERMINAL_FROM_OPIDEP(state->source_id) :
              get_hardware_channel(stream_endpoint_from_extern_id(state->source_id)));
        }

        /* Find out if the same operator is being already connect via KIP
         * query the the KIP transform list. If it is already being created
         * use the port ID.
         */
        uint16 port_id =  stream_kip_get_used_port(state->source_id,
                                                   state->sink_id);

        state->data_channel_id = 0;
        result = (ipc_create_data_channel(port_id, ch_id,
                                     IPC_DATA_CHANNEL_WRITE,
                                     &state->data_channel_id) == IPC_SUCCESS);
    }
    else
    if (state->data_channel_id == 0)
    {
        state->data_channel_id = (uint16)((STREAM_EP_IS_OPEP_ID(state->sink_id)) ?
                                        GET_TERMINAL_FROM_OPIDEP(state->sink_id) :
             get_hardware_channel(stream_endpoint_from_extern_id(state->sink_id)));
    }

    if ((result) &&
        (stream_create_endpoint(state->source_id, packed_con_id) != NULL) &&
        (stream_create_endpoint(state->sink_id, packed_con_id)   != NULL)    )
    {
        /* set the context as connect for the multi sequence connect */
        stream_kip_state_to_connect(state);

        /* set timing information from other core */
        stream_kip_set_timing(state);

        /* get timing information to send to other core */
        stream_kip_get_timing(state);
    }
    else
    {
        /* Failed to create the endpoints */
        result = FALSE;

        /* clean up */
        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
        ipc_destroy_data_channel(state->data_channel_id);
    }

    return result;
}

/**
 * \brief Go through the transform id list, stop receiving kicks and
 *        deactivate ready. When the remote deactivate the KIP endpoints,
 *        just accept it.
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - List of transform ids
 */
static void stream_kip_transform_deactivate_ready(unsigned count,
                                                  unsigned *tr_id_list)
{
    unsigned i;

    for (i = 0; (i < count); i++)
    {
        STREAM_KIP_TRANSFORM_INFO *tr;
        unsigned id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);

        tr = stream_kip_transform_info_from_id(id);

        if ((tr != NULL) && (ipc_get_data_channelid_port(tr->data_channel_id) != 0))
        {
            /* Disable the transform on KIP. This will stop the kicks through KIP*/
            tr->enabled = FALSE;
            if (ipc_get_data_channelid_dir(tr->data_channel_id) == IPC_DATA_CHANNEL_READ)
            {
                tCbuffer *buff = ipc_data_channel_get_cbuffer(tr->data_channel_id);
                /* Decremented the reference count and release the metadata information
                 * associated to this buffer */
                buff_metadata_release(buff);
            }
        }
    }
}

/**
 * \brief pack and send a remote transform disconnect request
 *
 * \param con_id  - Connection id
 * \param count   - Number of transform ids in the list to disconnect. This must
 *                  not be 0.
 * \param tr_list - The transform list
 * \param context - This is void because DESTROY as well use this function
 *                  to disconnect the transform.
 *
 * \return  TRUE on success
 */
static bool stream_kip_send_transform_disconnect(CONNECTION_LINK packed_con_id,
                                                 unsigned count,
                                                 unsigned *tr_list,
                                                 void     *state)
{
    bool result = FALSE;
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ *req = NULL;

    patch_fn_shared(stream_kip);

    /* Mark the transforms for deactivate, stop receiving kicks and
     * ready for deactivate
     */
    stream_kip_transform_deactivate_ready(count, tr_list);

    /* We will end up in odd double words in crescendo
     * and not use one uint16 location
     */
    req = xpmalloc(sizeof(KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ) +
                   count * sizeof(unsigned));
    if (req == NULL)
    {
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, count * sizeof(unsigned));
    }
    else
    {
        KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_COUNT_SET(req, count);

        /* Copy the transform ids without packing to uint16.
         * This avoids unpacking at the remote
         */
        /* TODO: This should be adaptor_pack_list_to_uint16. */
        memcpy(&req->_data[KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_TRANSFORM_IDS_WORD_OFFSET], tr_list,
                              count * sizeof(unsigned));

        result = kip_adaptor_send_message(packed_con_id,
                                          KIP_MSG_ID_STREAM_TRANSFORM_DISCONNECT_REQ,
                                          KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ_TRANSFORM_IDS_WORD_OFFSET +
                                          count * sizeof(unsigned)/sizeof(uint16),
                                          (uint16*) req, state);

        /* Free the request */
        pfree(req);
    }

    return result;
}

/**
 * \brief pack and send a kip_transform_list remove entry request
 *
 * \param packed_con_id  - Packed connection id
 * \param count          - Number of transform ids in the list to remove.
 *                         This must not be 0.
 * \param tr_list        - The transform id list
 * \param state          - Current KIP state or context to use if and
 *                         when a KIP reply comes back from P0.
 *
 * \return  TRUE on success
 */
static bool stream_kip_transform_list_remove_entry(CONNECTION_LINK packed_con_id,
                                                   unsigned count,
                                                   unsigned *tr_list,
                                                   void     *state)
{
    bool result = FALSE;
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ *req = NULL;

    patch_fn_shared(stream_kip);

    /* We will end up in odd double words in crescendo
     * and not use one uint16 location
     */
    req = xpmalloc(sizeof(KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ) +
                              count * sizeof(unsigned));

    if (req == NULL)
    {
        fault_diatribe(FAULT_AUDIO_INSUFFICIENT_MEMORY, count * sizeof(unsigned));
    }
    else
    {
        KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_COUNT_SET(req, count);

        /* Copy the transform ids without packing to uint16.
         * This avoids unpacking at the remote
         */
        /* TODO: This should be adaptor_pack_list_to_uint16. */
        memcpy(&req->_data[KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_TRANSFORM_IDS_WORD_OFFSET],
                              tr_list, count * sizeof(unsigned));

        result = kip_adaptor_send_message(packed_con_id,
                                          KIP_MSG_ID_TRANSFORM_LIST_REMOVE_ENTRY_REQ,
                                          KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ_TRANSFORM_IDS_WORD_OFFSET +
                                          count * sizeof(unsigned)/sizeof(uint16),
                                          (uint16*) req, state);

        /* Free the request */
        pfree(req);
    }

    return result;
}

/**
 * \brief Find next P0 only transform
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - The transform id list
 *
 * \return Index of first P0-only transfers in the KIP transfer info list
 */
static unsigned stream_kip_find_p0_only_transform_start(unsigned count,
                                                        unsigned *tr_id_list)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        unsigned id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        if (stream_kip_transform_info_from_id(id) == NULL)
        {
            break;
        }
    }
    return i;
}

/**
 * \brief Pack and send a remote destroy endpoint request
 *
 * \param con_id           - Connection id
 * \param remote_source_id - The remote source endpoint id
 * \param remote_sink_id   - The remote sink endpoint id
 * \param state            - This is void because DESTROY as well use this
 *                           function to disconnect the transform.
 */
static void stream_kip_destroy_endpoints(CONNECTION_LINK packed_con_id,
                                         unsigned remote_source_id,
                                         unsigned remote_sink_id,
                                         STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ req;

    patch_fn_shared(stream_kip);

    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_SINK_ID_SET(&req, remote_sink_id);

    if (!kip_adaptor_send_message(packed_con_id,
                                  KIP_MSG_ID_STREAM_DESTROY_ENDPOINTS_REQ,
                                  KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ_WORD_SIZE,
                                  (uint16*) &req, (void*) state))
    {
        stream_kip_destroy_endpoints_response_handler(packed_con_id,
                                                      STATUS_CMD_FAILED,
                                                      state);
    }
}

/**
 * \brief Send a request to the remote core to set the state as
 *        activated with an existing metadata
 *
 * \param packed_con_id    - Packed send/receive connection ID
 * \param meta_channel_id  - The existing metadata data channel id
 *
 * \return TRUE if successfully sent response, FALSE if not
 */
static bool stream_kip_send_metadata_channel_activated_req(CONNECTION_LINK packed_con_id,
                                                           uint16 meta_channel_id)
{
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ req;

    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ_CHANNEL_ID_SET(&req, meta_channel_id);

    return kip_adaptor_send_message(packed_con_id,
                                    KIP_MSG_ID_METADATA_CHANNEL_ACTIVATED_REQ,
                                    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ_WORD_SIZE,
                                    (uint16*) &req, NULL);
}

/**
 * \brief Send a response back after setting the state as activated with
 *        an existing metadata data channel
 *
 * \param packed_con_id    - Packed send/receive connection ID
 * \param status           - Status
 * \param meta_channel_id  - The existing metadata data channel id
 *
 * \return TRUE if successfully sent response, FALSE if not
 */
static bool stream_kip_send_metadata_channel_activated_resp(CONNECTION_LINK packed_con_id,
                                                            STATUS_KYMERA status,
                                                            uint16 meta_channel_id)
{
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES resp;

    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_STATUS_SET(&resp, status);
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_CHANNEL_ID_SET(&resp, meta_channel_id);

    return kip_adaptor_send_message(packed_con_id,
                                    KIP_MSG_ID_METADATA_CHANNEL_ACTIVATED_RES,
                                    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES_WORD_SIZE,
                                    (uint16*) &resp, NULL);
}

/**
 * \brief Gets the metadata channel from an existing connection or creates
 *        and activates a new channel for metadata if no connection already
 *        exists.
 *
 * \param state       - The state information to be populated
 * \param proc_id     - The processor ID
 *
 * \return TRUE if successful, FALSE if not
 */
static bool stream_kip_get_metadata_channel(STREAM_KIP_CONNECT_INFO *state,
                                            PROC_ID_NUM proc_id)
{
    patch_fn_shared(stream_kip);

    /* Update the KIP endpoint with metadata information */
    stream_kip_set_ep_meta_info(state, TRUE);

    /* Find the existing meta data channel associated with the same port */
    STREAM_KIP_TRANSFORM_INFO *tr = stream_kip_get_created_transform(state->source_id, state->sink_id);

    L4_DBG_MSG("stream_kip_get_metadata_channel - Getting metadata channel");
    if (tr == NULL)
    {
        /* No existing connection, create and activate a channel */

        uint16 meta_channel_id = (state->meta_channel_id == 0) ?
                              META_DATA_CHANNEL_NUM : state->meta_channel_id;


        uint16 port_id = ipc_get_data_channelid_port(state->data_channel_id);

        if (IPC_SUCCESS != ipc_create_data_channel(port_id,
                                                   meta_channel_id,
                                                   IPC_DATA_CHANNEL_WRITE,
                                                   &state->meta_channel_id))
        {
            ipc_destroy_data_channel(state->meta_channel_id);
            return FALSE;
        }

        if (!stream_kip_activate_metadata_channel(state, proc_id))
        {
            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                                         STREAM_GET_SHADOW_EP_ID(state->source_id),
                                         STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                                         state);

            return FALSE;
        }
    }
    else
    {
        /* Some connection already exists, use the existing metadata channel */
        state->meta_channel_id = (state->data_channel_id) | META_DATA_CHANNEL_NUM;

        /*
         * Now we only need to set the state to activated for the metadata data
         * channel. Send a request to the remote core first in order to follow
         * the same sequence as a metadata data channel activation.
         */
        stream_kip_send_metadata_channel_activated_req(state->packed_con_id,
                                (uint16)ipc_invert_chanid_dir(state->meta_channel_id));
    }

    L4_DBG_MSG("stream_kip_get_metadata_channel - send metadata channel activation request");
    return TRUE;

}

/**
 * \brief Remove all remote transforms in the list
 *
 * \param count       - Number of transform ids in the list
 * \param tr_id_list  - The transform id list
 *
 * \return Number of remote-only transfers removed from the
 *         KIP transfer info list
 */
static unsigned stream_kip_remove_px_only_transforms(unsigned count,
                                                     unsigned *tr_id_list)
{
    unsigned i;

    patch_fn_shared(stream_kip);

    for (i = 0; i < count; i++)
    {
        unsigned id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        STREAM_KIP_TRANSFORM_INFO *tr = stream_kip_transform_info_from_id(id);

        if (tr != NULL)
        {
            /* Store the endpoint IDs so that we can tidy up ratematching
             * after the disconnect. */
            unsigned source_id = tr->source_id;
            unsigned sink_id = tr->sink_id;
            /* Break if the transform is a KIP transform */
            if (STREAM_EP_IS_SHADOW_ID(source_id) ||
                STREAM_EP_IS_SHADOW_ID(sink_id))
            {
                break;
            }

            /* remove from the transform list */
            stream_kip_remove_transform_info(tr);

            /* The external ID is stored in the transform but ratematching code
             * expects internal_id so convert it */
            TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(source_id);
            cease_ratematching(source_id);
            TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(sink_id);
            cease_ratematching(sink_id);

            set_system_event(SYS_EVENT_EP_DISCONNECT);
        }
    }

    return i;
}

/**
 * \brief Do a check on the callback response before calling the final
 *        callback. This function decides to continue next iteration
 *        of disconnect if all transforms are not disconnected.
 *
 * \param id          - Connection id
 * \param status      - Status
 * \param count       - Total disconnected count
 *
 * \return  TRUE on success
 */
static bool stream_kip_disconnect_callback_handler(CONNECTION_LINK con_proc_id,
                                                   STATUS_KYMERA status,
                                                   unsigned count)
{
    bool loop = FALSE;
    unsigned *tr_list;

    patch_fn_shared(stream_kip);

    /* Context MUST not be NULL at this point. No validation required */
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state = stream_kip_state_get_disconnect_info();

    if (status == STATUS_OK)
    {

        /* clean up remote only transforms from the list */
        if (state->remote_success_count > count)
        {
            unsigned px_tr_count;
            tr_list = ((unsigned*)state->tr_list + count);
            px_tr_count = stream_kip_remove_px_only_transforms(
                                  state->remote_success_count - count,
                                  tr_list);
            count += px_tr_count;
        }

        /* update the total success count */
        state->success_count = (uint16)count;

        if (state->count > count)
        {
            unsigned local_remain = 0;
            unsigned remaining = state->count - count;

            /* There are still more to disconnect */
            tr_list = &state->tr_list[count];

            /* If we have disconnected more remote ones, then start
             * with start with local ones
             */
            if (state->remote_success_count > count)
            {
                local_remain = state->remote_success_count - count;

                if (state->remote_success_count < state->count)
                {
                    /*
                     * Find the next boundary whether Px transforms starts. So we
                     * can disconnect local P0 chunk together with remaining KIP
                     * transforms
                     */
                    local_remain += stream_kip_find_px_transform_start (
                                    state->count - state->remote_success_count,
                                    &state->tr_list[state->remote_success_count]);
                }
            }
            else
            {
                local_remain = stream_kip_find_px_transform_start(remaining,
                                                                   tr_list);
            }

            if (local_remain > 0)
            {
                /* P0 list to process */
                stream_if_part_transform_disconnect(con_proc_id,
                                                     local_remain,
                                                     tr_list,
                                                     state->success_count,
                                            stream_kip_disconnect_callback_handler);
                loop = TRUE;
            }
            else
            {
                /* P1 list to process */
                 count = stream_kip_find_p0_only_transform_start(remaining, tr_list);

                 loop= stream_kip_send_transform_disconnect(
                                                  REVERSE_CONNECTION_ID(con_proc_id),
                                                  count,
                                                  tr_list,
                                                  state);
            }
        }
    }
    else
    {
        if (state->remote_success_count > count)
        {
            /* We are in a irrecoverable situation where P1
             * KIP transforms are disconnected but P0 KIP
             * transforms are hanging around while disconnect
             * failed.
             */
             fault_diatribe(FAULT_AUDIO_STREAM_TRANSFORM_DISCONNNECT_ERROR,
                            state->remote_success_count);
        }
    }

    /* no more looping between P0 and P1. Exit now */
    if (!loop)
    {
        if (state->disc_cb_flag)
        {
            state->callback.disc_cb(UNPACK_REVERSE_CONID(con_proc_id), status,
                            state->tr_list[0],
                            *((unsigned*)state->tr_list + 1));
        }
        else
        {
            state->callback.tr_disc_cb(UNPACK_REVERSE_CONID(con_proc_id),
                                        status, count);
        }

        /* Free the global context and unblock KIP */
        stream_kip_state_to_none(TRUE);
    }

    return TRUE;
}

/**
 * \brief Do a check on the callback response before calling the
 *        final callback. This function release the resources
 *        in case of error
 *
 * \param con_id       - The connection id
 * \param status       - Status
 * \param transform_id - Transform
 *
 * \return  TRUE on success
 */
static bool stream_kip_connect_callback_handler(CONNECTION_LINK con_id,
                                                STATUS_KYMERA status,
                                                unsigned transform_id)
{
    STREAM_KIP_CONNECT_INFO *state;

    /* This must be in connect state */
    state = stream_kip_state_get_connect_info();

    STREAM_KIP_ASSERT(state != NULL);

    if ((status != STATUS_OK) && STREAM_KIP_STATE_IN_CONNECT())
    {
        /* failed to establish stream connection at P0
         * Send a disconnect request
         */
        if (stream_kip_send_transform_disconnect(state->packed_con_id, 1,
                                              &transform_id, (void*)state))
        {
            /* wait for the remote return */
            return TRUE;
        }
    }

    /* original accmd callback */
    state->callback(UNPACK_REVERSE_CONID(con_id), status, transform_id);

    /* Unblock KIP if it was blocked*/
    stream_kip_state_to_none(TRUE);

    return TRUE;
}

/**
 * \brief Internal function to handle the kip disconnect response. This
 *        function pointer will be provided as a callback function
 *        to streams to send transform disconnect response through KIP.
 *
 * \param con_id      - The connection id
 * \param status      - Status of the request
 * \param count       - The number of transforms got disconnected.
 *
 * \return TRUE on success
 */
static bool stream_kip_send_transform_disconnect_resp(CONNECTION_LINK con_id,
                                                      STATUS_KYMERA status,
                                                      unsigned count)
{
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES resp;

    if ((count == 0) && (status == STATUS_OK))
    {
        status = STATUS_CMD_FAILED;
    }

    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_STATUS_SET(&resp, status);
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_COUNT_SET(&resp, count);

    /* Free the global context and unblock KIP only
     * if it was in disconnect
     */
    if (STREAM_KIP_STATE_IN_DISCONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    return kip_adaptor_send_message(con_id,
                                    KIP_MSG_ID_STREAM_TRANSFORM_DISCONNECT_RES,
                                    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES_WORD_SIZE,
                                    (uint16*) &resp, NULL);
}

/**
 * \brief Internal function to handle the KIP P0 transform list remove
 *        entry response. This function pointer will be provided as a
 *        callback function to streams to send KIP P0 transform list
 *        remove response through KIP. This function is only provided
 *        for P0 (to handle requests from Px).
 *
 * \param con_id      - The connection id
 * \param status      - Status of the request
 * \param count       - The number of transforms that were removed.
 *
 * \return TRUE on success
 */
static bool stream_kip_transform_list_remove_entry_resp(CONNECTION_LINK con_id,
                                                        STATUS_KYMERA status,
                                                        unsigned count)
{
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES resp;

    if ((count == 0) && (status == STATUS_OK))
    {
        status = STATUS_CMD_FAILED;
    }

    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_STATUS_SET(&resp, status);
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_COUNT_SET(&resp, count);

    /* Free the global context and unblock KIP only
     * if it was in disconnect
     */
    if (STREAM_KIP_STATE_IN_DISCONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    return kip_adaptor_send_message(con_id,
                                    KIP_MSG_ID_TRANSFORM_LIST_REMOVE_ENTRY_RES,
                                    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES_WORD_SIZE,
                                    (uint16*) &resp, NULL);
}

/**
 * \brief Internal function to send the kip connect response. This
 *        function pointer will be provided as a callback function
 *        to streams to send stream connect response through KIP.
 *
 * \param con_id       - The connection id
 * \param status       - Status of the request
 * \param transform_id - Transform
 *
 * \return TRUE on success
 */
static bool stream_kip_send_connect_resp(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         unsigned transform_id)
{
    KIP_MSG_STREAM_CONNECT_RES resp;
    STREAM_KIP_TRANSFORM_INFO *tr = NULL;

    patch_fn_shared(stream_kip);

#ifdef STREAM_CONNECT_FAULT_CODE
    stream_connect_fault = SC_KIP_CONNECTION_FAILED;
    L1_DBG_MSG1("stream_kip_send_connect_resp status = %X",status);
#endif /* STREAM_CONNECT_FAULT_CODE */

    tr = stream_kip_transform_info_from_id(
                    STREAM_TRANSFORM_GET_INT_ID(transform_id));

    if (tr != NULL)
    {
        if (status != STATUS_OK)
        {
            stream_kip_remove_transform_info(tr);
        }
        else
        {
            if (STREAM_EP_IS_SHADOW_ID(tr->sink_id))
            {
                endpoint_shadow_state *state;
                tCbuffer *shared_buffer;
                ENDPOINT *shadow_ep = stream_endpoint_from_extern_id(tr->sink_id);

                /* If connecting to a shadow endpoint */
                state = &shadow_ep->state.shadow;

                /* propagate potential usable_octets TO the shared_buffer */
                shared_buffer = ipc_data_channel_get_cbuffer(state->channel_id);
                cbuffer_set_usable_octets(shared_buffer, cbuffer_get_usable_octets(state->buffer));
            }

            tr->enabled = TRUE;
        }
    }

    KIP_MSG_STREAM_CONNECT_RES_STATUS_SET(&resp, status);
    KIP_MSG_STREAM_CONNECT_RES_TRANSFORM_ID_SET(&resp, transform_id);

    /* Add to KIP transform list */
    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        stream_kip_state_to_none(TRUE);
    }

    return kip_adaptor_send_message(con_id, KIP_MSG_ID_STREAM_CONNECT_RES,
                                    KIP_MSG_STREAM_CONNECT_RES_WORD_SIZE,
                                    (uint16*) &resp, NULL);
}

/**
 * \brief Internal function to handle the kip create endpoint response
 *
 * \param con_id      - The connection id
 * \param status      - Status of the request
 *
 * \return TRUE on success
 */
static bool stream_kip_send_create_endpoints_resp(CONNECTION_LINK packed_con_id,
                                                  STATUS_KYMERA status)
{
    KIP_MSG_STREAM_CREATE_ENDPOINTS_RES resp;
    STREAM_KIP_CONNECT_INFO *state = NULL;

    patch_fn_shared(stream_kip);

    if (status != STATUS_OK)
    {
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_STATUS_SET(&resp, status);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_CHANNEL_ID_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_BUFFER_SIZE_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_TINFO_BLOCK_SIZE_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_TINFO_PERIOD_SET(&resp, 0);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_FLAGS_SET(&resp, 0);
    }
    else
    {
        STREAM_TRANSFORM_BUFFER_INFO *buffer_info;
        BUFFER_DETAILS buffer_details;
        unsigned buffer_size;

        state = stream_kip_state_get_connect_info();

        STREAM_KIP_ASSERT(state != NULL);

        buffer_info= &(state->connect_info.buffer_info);
        /* Get the local endpoint buffer details to retrieve the size */
        stream_kip_get_local_ep_buffer_details(state, &buffer_details);
        buffer_size = get_buf_size(&buffer_details);

        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_STATUS_SET(&resp, status);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_CHANNEL_ID_SET(&resp,
                                                           state->data_channel_id);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_BUFFER_SIZE_SET(&resp,
                                                            buffer_size);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_TINFO_BLOCK_SIZE_SET(&resp,
                                                                 state->tinfo_block_size);
        KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_FLAGS_SET(&resp,
                                                      (*((uint16*)&buffer_info->flags)));
    }

    /* Now send the message */
    return kip_adaptor_send_message(packed_con_id,
                                    KIP_MSG_ID_STREAM_CREATE_ENDPOINTS_RES,
                                    KIP_MSG_STREAM_CREATE_ENDPOINTS_RES_WORD_SIZE,
                                    (uint16*) &resp, (void*) state);
}

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Get the (external) transform id associated with a given endpoint, if any.
 *
 * \param endpoint [IN]  - The KIP endpoint
 * \param tr_id    [OUT] - The (external) transform id
 *
 * \return TRUE if a valid transform (id) was found, FALSE if not.
 */
bool stream_transform_id_from_endpoint(ENDPOINT *endpoint, unsigned *tr_id)
{
    TRANSFORM *tr = stream_transform_from_endpoint(endpoint);

    if (tr == NULL)
    {
        return FALSE;
    }

    *tr_id = STREAM_TRANSFORM_GET_EXT_ID(tr->id);

    return TRUE;
}

/**
 * \brief Request to P0 to remove entry from kip_transform_list
 *        (where P0 keeps a copy/entry/id of each transform on Px).
 *
 * \param tr_id       - The external transform id
 * \param proc_id     - The processor ID
 *
 * \return            TRUE if successful
 */
bool stream_kip_cleanup_endpoint_transform(unsigned tr_id,
                                           PROC_ID_NUM proc_id)
{
    CONNECTION_LINK con_id;

    con_id = PACK_CONID_PROCID(PROC_PROCESSOR_0, proc_id);

    (void) stream_kip_transform_list_remove_entry(con_id, 1, &tr_id, NULL);

    return TRUE;
}

/**
 * \brief Disconnect the transform associated with a KIP endpoint
 *
 * \param endpoint    - The KIP endpoint
 * \param proc_id     - The processor_id
 *
 * \return            TRUE if success, FALSE otherwise
 */
bool stream_kip_disconnect_endpoint(ENDPOINT *endpoint, PROC_ID_NUM proc_id)
{
    TRANSFORM *tr = stream_transform_from_endpoint(endpoint);
    unsigned tr_id = 0;
    CONNECTION_LINK con_id;

    if (tr == NULL)
    {
        return FALSE;
    }

    tr_id  = STREAM_TRANSFORM_GET_EXT_ID(tr->id);
    con_id = PACK_CONID_PROCID(PROC_PROCESSOR_0, proc_id);

    if (stream_kip_send_transform_disconnect(con_id, 1, &tr_id, NULL));

    /* Now remove it from kip transform info list */
    stream_kip_remove_transform_info_by_id(tr_id);

    return TRUE;
}

/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it.
 *
 * \param con_id      - The packed connection id
 * \param source_id   - The source id at the local side
 * \param sink_id     - The sink id at the  local side
 * \param ep_location - Location of endpoints
 * \param block_size  - Timing info 'block_size' of shadowed operator
 * \param period      - Timing info 'period' of shadowed operator
 * \param callback    - The callback to be called after handling the response
 *
 * \return  A connect information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record_ex(CONNECTION_LINK con_id,
                                                                  unsigned source_id,
                                                                  unsigned sink_id,
                                                                  STREAM_EP_LOCATION ep_location,
                                                                  unsigned block_size,
                                                                  unsigned period,
                                                                  bool (*callback)(CONNECTION_LINK con_id,
                                                                                   STATUS_KYMERA status,
                                                                                   unsigned transform_id))
{
    STREAM_KIP_CONNECT_INFO *ep_connect_info= xzpnew(STREAM_KIP_CONNECT_INFO);

    if (ep_connect_info == NULL)
    {
        return NULL;
    }

    ep_connect_info->packed_con_id     = (uint16)con_id;
    ep_connect_info->source_id         = (uint16)source_id;
    ep_connect_info->sink_id           = (uint16)sink_id;
    ep_connect_info->ep_location       = ep_location;
    ep_connect_info->callback          = callback;
    ep_connect_info->tinfo_block_size  = block_size;
    ep_connect_info->tinfo_period      = period;

    return ep_connect_info;
}

/**
 * \brief Create a connect info record during connection state and partially
 *        initialise it. This is different from the above in that parameters
 *        'block_size' and 'period' are replaced by default values for a
 *        shadow endpoint. This is useful at the start of the shadow
 *        endpoint connection setup sequence.
 *
 * \param con_id      - The packed connection id
 * \param source_id   - The source id at the local side
 * \param sink_id     - The sink id at the  local side
 * \param ep_location - Location of endpoints
 * \param callback    - The callback to be called after handling the response
 *
 * \return  A connection information record or NULL
 */
STREAM_KIP_CONNECT_INFO *stream_kip_create_connect_info_record(CONNECTION_LINK con_id,
                                                               unsigned source_id,
                                                               unsigned sink_id,
                                                               STREAM_EP_LOCATION ep_location,
                                                               bool (*callback)(CONNECTION_LINK con_id,
                                                                                STATUS_KYMERA status,
                                                                                unsigned transform_id))
{
    return stream_kip_create_connect_info_record_ex(con_id, source_id, sink_id,
               ep_location, stream_shadow_get_block_size_default(),
               stream_shadow_get_period_default(), callback);
}

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param id          - KIP transform info id
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_id(unsigned id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->id != id))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_epid(unsigned epid)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->source_id != epid) && (tr->sink_id != epid))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Helper function to find the ID of a remote endpoint
 *        connected to a known endpoint.
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return The ID of the endpoint connected to endpoint with ID epid.
 *         0 if not found.
 */
unsigned stream_kip_connected_to_epid(unsigned epid)
{
    STREAM_KIP_TRANSFORM_INFO *tr = stream_kip_transform_info_from_epid(epid);

    if ((epid & STREAM_EP_SINK_BIT) == STREAM_EP_SINK_BIT)
    {
        return tr->source_id;
    }
    else
    {
        return tr->sink_id;
    }
}

/**
 * \brief Helper function to create new remote transform info entry
 *        and add it to the respective list.
 *
 * \param id           - Internal transform id
 * \param processor_id - Remote processor id
 * \param source_id    - Remote source id
 * \param sink_id      - Remote sink id
 * \param id           - Data channel id
 *
 * \return             Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_add_transform_info(unsigned id,
                                PROC_ID_NUM processor_id,
                                unsigned source_id,
                                unsigned sink_id,
                                uint16 data_chan_id)
{
    STREAM_KIP_TRANSFORM_INFO* tr_info = xpnew(STREAM_KIP_TRANSFORM_INFO);

    if (tr_info != NULL)
    {
        tr_info->next = kip_transform_list;
        kip_transform_list = tr_info;

        tr_info->id = id;
        tr_info->processor_id = proc_serialize(processor_id);
        tr_info->data_channel_id = data_chan_id;
        tr_info->source_id = source_id;
        tr_info->sink_id = sink_id;
        tr_info->enabled = FALSE;
    }

    return tr_info;
}

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param id - Internal transform id of transform to be removed
 */
void stream_kip_remove_transform_info_by_id(unsigned tr_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr, **tr_p = &kip_transform_list;

    while ((tr = *tr_p) != NULL)
    {
        /* Is this the one we're looking for? */
        if (tr->id == tr_id)
        {
            /* Remove entry from list and free it */
            *tr_p = tr->next;
            pfree(tr);

            return;
        }

        tr_p = &tr->next;
    }
}

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param transform - Transform to be removed
 */
void stream_kip_remove_transform_info(STREAM_KIP_TRANSFORM_INFO* transform)
{
    STREAM_KIP_TRANSFORM_INFO *tr, **tr_p = &kip_transform_list;

    while ((tr = *tr_p) != NULL)
    {
        /* Is this the one we're looking for? */
        if (tr == transform)
        {
            /* Remove entry from list and free it */
            *tr_p = tr->next;
            pfree(tr);

            return;
        }

        tr_p = &tr->next;
    }
}

/**
 * \brief Helper function to retrieve entry in remote transform
 *        info list based on data channel ID.
 *
 * \param data_chan_id - data channel id
 *
 * \return             Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_chanid(uint16 data_chan_id)
{
    STREAM_KIP_TRANSFORM_INFO *tr = kip_transform_list;

    while ((tr != NULL) && (tr->data_channel_id != data_chan_id))
    {
        tr = tr->next;
    }

    return tr;
}

/**
 * \brief Create disconnect info record
 *
 * \prama con_id      - The connection id
 * \param count       - Number of transforms in the list
 * \param ep_disc_cb  - Flag as to which callback to call
 *                      (state->callback.disc_cb or .tr_disc_cb)
 * \param transforms  - The transform list
 * \param callback    - The callback
 *
 * \return            Pointer to an allocated disconnect info object
 */
STREAM_KIP_TRANSFORM_DISCONNECT_INFO *stream_kip_create_disconnect_info_record(CONNECTION_LINK con_id,
                                                                               unsigned count,
                                                                               bool ep_disc_cb,
                                                                               unsigned *transforms,
                                                                               STREAM_KIP_TRANSFORM_DISCONNECT_CB callback)
{
    STREAM_KIP_TRANSFORM_DISCONNECT_INFO *ep_disconnect_info=
              xpmalloc(sizeof(STREAM_KIP_TRANSFORM_DISCONNECT_INFO) +
                                (count * sizeof(unsigned)));

    if (ep_disconnect_info == NULL)
    {
        return NULL;
    }

    ep_disconnect_info->packed_con_id = (uint16)con_id;
    ep_disconnect_info->count = (uint16)count;
    ep_disconnect_info->disc_cb_flag = ep_disc_cb;
    ep_disconnect_info->success_count = 0;
    ep_disconnect_info->remote_success_count = 0;
    ep_disconnect_info->callback = callback;

    memcpy(&ep_disconnect_info->tr_list[0], transforms, count * sizeof(unsigned));

    return ep_disconnect_info;
}

/**
 * \brief Find the first Px transform offset from the kip_transform_list
 *
 * \param count       - The total number of transforms in the list
 * \param tr_id_list  - The transform list
 *
 * \return            The first Px transform offset from the list
 */
unsigned stream_kip_find_px_transform_start(unsigned count, unsigned *tr_id_list)
{
    unsigned i;

    for (i = 0; i < count; i++)
    {
        unsigned id = STREAM_TRANSFORM_GET_INT_ID(tr_id_list[i]);
        if (stream_kip_transform_info_from_id(id) != NULL)
        {
            break;
        }
    }

    return i;
}

/**
 * \brief Handling the incoming destroy endpoint response.
 *
 * \param con_id      - The connection id
 * \param status      - Status
 * \param state_info  - The connection state information.
 */
void stream_kip_destroy_endpoints_response_handler(CONNECTION_LINK con_id,
                                                   STATUS_KYMERA status,
                                                   STREAM_KIP_CONNECT_INFO *state)
{
    if (status != STATUS_OK)
    {
        /* P1 may end up in a wrong state */
        fault_diatribe(FAULT_AUDIO_STREAM_ENDPOINT_DESTROY_ERROR , 0);
    }

    if ((STREAM_KIP_STATE_IN_CONNECT()               ) &&
        (stream_kip_state_get_connect_info() == state)    )
    {
        /* attempt to destroy the endpoints if exists */
        stream_kip_destroy_endpoint_ids(state);

        state->callback(UNPACK_REVERSE_CONID(con_id), STATUS_CMD_FAILED, 0);
        stream_kip_state_to_none(TRUE);
    }
}

/**
 * \brief Handling the incoming create endpoint response.
 *
 * \param con_id      - The connection id
 * \param status      - Status
 * \param channel_id  - The data channel id. This must not be 0.
 * \param buffer_size - Negotiated buffer size for the connection
 * \param block_size  - The block size for ep timing info
 * \param period      - The period for ep timing info
 * \param flags       - The buffer related flags
 * \param state_info  - The connection state information.
 */
void stream_kip_create_endpoints_response_handler(CONNECTION_LINK con_id,
                                                  STATUS_KYMERA status,
                                                  unsigned channel_id,
                                                  unsigned buffer_size,
                                                  unsigned block_size,
                                                  unsigned period,
                                                  unsigned flags,
                                                  STREAM_KIP_CONNECT_INFO *state)
{
    bool result = FALSE;

    patch_fn_shared(stream_kip);

    STREAM_KIP_ASSERT(state != NULL);

    /* Record returned timing information from shadowed operator */
    state->tinfo_block_size = block_size;
    state->tinfo_period     = period;

    if ((status != STATUS_OK) || !STREAM_KIP_STATE_IN_CONNECT() ||
        (channel_id == 0) || (ipc_get_data_channelid_port(state->data_channel_id) != 0 &&
        (ipc_invert_chanid_dir(channel_id) != state->data_channel_id)))
    {
        stream_kip_destroy_endpoint_ids(state);

        state->callback(UNPACK_REVERSE_CONID(con_id), STATUS_CMD_FAILED, 0);
        stream_kip_state_to_none(TRUE);
    }
    else
    {
        /* set timing information from other core */
        stream_kip_set_timing(state);

        if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
        {

            /* update the state buffer details. */
            stream_kip_update_buffer_info(state,
                                          ipc_invert_chanid_dir(channel_id),
                                          buffer_size, flags);

            result = stream_kip_connect_endpoints(state->packed_con_id,
                            STREAM_EP_ID_FROM_SHADOW_ID(state->source_id),
                            STREAM_GET_SHADOW_EP_ID(state->sink_id),
                            state);
        }
        else
        {
            /* update the state buffer details. no channel id changes */
            stream_kip_update_buffer_info(state, state->data_channel_id,
                                          buffer_size, flags);

            PROC_ID_NUM proc_id = GET_RECV_PROC_ID(state->packed_con_id);
            result = kip_get_buffer_and_activate_channels(state, proc_id);
        }

        if (!result)
        {

#ifdef STREAM_CONNECT_FAULT_CODE
            stream_connect_fault = SC_KIP_GET_DATA_CHANNEL_FAILED;
            L1_DBG_MSG("stream_connect_fault: Failed to get_data_channel");
#endif /* STREAM_CONNECT_FAULT_CODE */

            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                            STREAM_GET_SHADOW_EP_ID(state->source_id),
                            STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                            state);
        }
    }
}

/**
 * \brief Handle the transform disconnect response from the remote
 *
 * \param con_id     - The connection id
 * \param status     - Status of the request
 * \param count      - The number of disconnected transforms
 * \param state      - The disconnect state
 */
void stream_kip_transform_disconnect_response_handler(CONNECTION_LINK con_id,
                                                      STATUS_KYMERA status,
                                                      unsigned count,
                                                      STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state)
{
    patch_fn_shared(stream_kip);

    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        /* This is a disconnect due to a connection failure */
        stream_kip_connect_callback_handler(con_id, status, 0);
    }
    else
    if ((STREAM_KIP_STATE_IN_DISCONNECT()               ) &&
        (stream_kip_state_get_disconnect_info() == state)    )
    {
        STREAM_KIP_ASSERT(state != NULL);

        state->remote_success_count = (uint16)(state->success_count + count);

        stream_kip_disconnect_callback_handler(con_id, status,
                                               state->success_count);
    }
    else
    {
        /*
         * Ignore the response. This happens while cleaning
         * up transforms during endpoint close especially
         * destroying operators.
         */
    }

    return;
}

/**
 * \brief Handle the connect resp from the secondary core
 *
 * \param con_id       - The connection id
 * \param status       - Status of the request
 * \param transform_id - Transform id returned
 * \param state        - The connect state
 */
void stream_kip_connect_response_handler(CONNECTION_LINK con_id,
                                         STATUS_KYMERA status,
                                         unsigned transform_id,
                                         STREAM_KIP_CONNECT_INFO *state)
{
    STREAM_KIP_ASSERT(state != NULL);

    patch_fn_shared(stream_kip);

    if (status == STATUS_OK)
    {
        /* The external ID is stored in the transform but ratematching code
         * expects internal_id so convert it */
        unsigned source_id = state->source_id;
        TOGGLE_EP_ID_BETWEEN_INT_AND_EXT(source_id);
        if (!cease_ratematching(source_id))
        {
            status = STATUS_CMD_FAILED;
        }
        else
        if (STREAM_TRANSFORM_GET_INT_ID(transform_id) != state->tr_id)
        {
            /* Unexpected. P1 is not playing by the rules! */
            fault_diatribe(FAULT_AUDIO_MULTICORE_CONNECT_INVALID_STATE, transform_id);

            status = STATUS_CMD_FAILED;
        }
    }

    if (status != STATUS_OK)
    {

#ifdef STREAM_CONNECT_FAULT_CODE
        stream_connect_fault = SC_KIP_CONNECTION_FAILED;
        L1_DBG_MSG1("stream_kip_connect_resp_handler = %X",status);
#endif /* STREAM_CONNECT_FAULT_CODE */

        /* Deactivating the data channel as well remove the transform info */
        if (state->meta_channel_id != 0)
        {
            bool data_channel_status;
            data_channel_status = stream_kip_data_channel_deactivate_ipc(state->meta_channel_id);
            STREAM_KIP_ASSERT(data_channel_status);
        }
        if (!(stream_kip_data_channel_deactivate(state->data_channel_id)))
        {
            /* remove the transform if deactivation fails */
            stream_kip_remove_transform_info_by_id(transform_id);
        }

        stream_destroy_endpoint_id(state->source_id);
        stream_destroy_endpoint_id(state->sink_id);
    }
    else
    {
        /*  update the transform info list with status */
        STREAM_KIP_TRANSFORM_INFO* kip_tr;

        /* get the preserved remote transform info */
        kip_tr = stream_kip_transform_info_from_id(state->tr_id);

        /* activate the transform */
        kip_tr->enabled = TRUE;

        /* Mark whether the source or sink endpoint is a real endpoint at P0 */
        kip_tr->real_source_ep = STREAM_EP_IS_REALEP_ID(state->source_id);
        kip_tr->real_sink_ep = STREAM_EP_IS_REALEP_ID(state->sink_id);

        STREAM_KIP_ASSERT(kip_tr != NULL);

        if (state->ep_location != STREAM_EP_REMOTE_ALL)
        {
            stream_if_transform_connect(UNPACK_REVERSE_CONID(con_id),
                                        state->source_id,
                                        state->sink_id,
                                        transform_id, &state->connect_info,
                                        stream_kip_connect_callback_handler);

            /* propagate potential usable_octets ... */
            if (STREAM_EP_IS_SHADOW_ID(state->sink_id) ||
                STREAM_EP_IS_SHADOW_ID(state->source_id))
            {
                ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
                ENDPOINT *sink_ep = stream_endpoint_from_extern_id(state->sink_id);
                ENDPOINT *op_term;
                endpoint_shadow_state *shadow_state;
                tCbuffer *shared_buffer;
                KIP_MSG_STREAM_CONNECT_REQ req;
                unsigned ext_opid;

                if (STREAM_EP_IS_SHADOW_ID(state->sink_id))
                {
                    /* ... to shared_buffer */
                    op_term = source_ep;
                    shadow_state = &sink_ep->state.shadow;
                    shared_buffer = ipc_data_channel_get_cbuffer(shadow_state->channel_id);
                    STREAM_KIP_ASSERT(shared_buffer != NULL);
                    cbuffer_set_usable_octets(shared_buffer,
                             cbuffer_get_usable_octets(shadow_state->buffer));
                }
                else
                {
                    /* ... from shared_buffer */
                    op_term = sink_ep;
                    shadow_state = &source_ep->state.shadow;
                    shared_buffer = ipc_data_channel_get_cbuffer(shadow_state->channel_id);
                    STREAM_KIP_ASSERT(shared_buffer != NULL);
                    cbuffer_set_usable_octets(shadow_state->buffer,
                             cbuffer_get_usable_octets(shared_buffer));

                }

                /* send kip stream_connect_confirm_req message for similar action on second core */
                ext_opid = stream_external_id_from_endpoint(op_term);
                KIP_MSG_STREAM_CONNECT_CONFIRM_REQ_CONN_TO_SET(&req, ext_opid);
                kip_adaptor_send_message(UNPACK_REVERSE_CONID(con_id),
                                         KIP_MSG_ID_STREAM_CONNECT_CONFIRM_REQ,
                                         KIP_MSG_STREAM_CONNECT_CONFIRM_REQ_WORD_SIZE,
                                         (uint16*) &req, NULL);
            }

            return;
        }
    }

    /* orginal accmd callback */
    state->callback(UNPACK_REVERSE_CONID(con_id), status, transform_id);

    if (STREAM_KIP_STATE_IN_CONNECT())
    {
        /* Unblock KIP if it was blocked*/
        stream_kip_state_to_none(FALSE);
    }
    else
    {
        /*
         * Stream connection of 2 remote endpoints are forwarded
         * by P0 without blocking KIP using the CONNECT state.
         * In that case, free the context.
         */
        pfree(state);
    }

    set_system_event(SYS_EVENT_EP_CONNECT);

    return;
}

/**
 * \brief Create local endpoints and send a remote request to create
 *        endpoints at secondary core.
 *
 * \param con_id     - The packed connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 * \param state      - The connect state info
 *
 * \return           TRUE on success
 */
bool stream_kip_create_endpoints(CONNECTION_LINK packed_con_id,
                                 unsigned remote_source_id,
                                 unsigned remote_sink_id,
                                 STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ req;
    STREAM_TRANSFORM_BUFFER_INFO *buffer_info = &(state->connect_info.buffer_info);
    unsigned data_channel_id = 0;

    patch_fn_shared(stream_kip);

    /* If KIP is busy, return failure */
    if (!STREAM_KIP_STATE_IN_IDLE())
    {
        return FALSE;
    }

    /* Create endpoints locally and get the buffer details */
    if (!stream_kip_create_eps_for_connect(packed_con_id, state))
    {
        return FALSE;
    }

    /* Get the endpoints buffer details and it is not expected to fail*/
    stream_kip_ep_get_buffer_info(state);

    stream_kip_set_ep_meta_info(state, buffer_info->flags.supports_metadata);

    data_channel_id = (ipc_get_data_channelid_port(state->data_channel_id) == 0) ?
                       state->data_channel_id :
                       ipc_invert_chanid_dir(state->data_channel_id);

    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_SINK_ID_SET(&req, remote_sink_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_CHANNEL_ID_SET(&req, data_channel_id);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_BUFFER_SIZE_SET(&req, buffer_info->buffer_size);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_TINFO_BLOCK_SIZE_SET(&req, state->tinfo_block_size);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_TINFO_PERIOD_SET(&req, state->tinfo_period);
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_FLAGS_SET(&req, *((uint16*)&(buffer_info->flags)));

    /* Now send the message */
    if (!kip_adaptor_send_message(packed_con_id,
                                  KIP_MSG_ID_STREAM_CREATE_ENDPOINTS_REQ,
                                  KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ_WORD_SIZE,
                                  (uint16*) &req, (void*) state))
    {
        stream_kip_destroy_endpoint_ids(state);

        /* state info will be free'd by the caller in case of failure*/
        stream_kip_state_to_none(FALSE);
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief  Send a KIP stream disconnect
 *
 * \param state      - disconnect state
 * \param offset     - Offset to start processing the transform list
 *                     of the state for the secondary core.
 *
 * \return           TRUE on success
 */
bool stream_kip_transform_disconnect(STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state,
                                     unsigned px_tr_offset)
{
    bool result = TRUE;

    patch_fn_shared(stream_kip);

    /*
     * Avoid any other stream operations over KIP
     * while disconnect is in progress.
     */
    stream_kip_state_to_disconnect(state);

    if ((px_tr_offset > 0) && (state->success_count < px_tr_offset))
    {
        /* start with disconnecting local transforms */
        stream_if_part_transform_disconnect(
                          REVERSE_CONNECTION_ID(state->packed_con_id),
                                              px_tr_offset,
                                              state->tr_list,
                                              state->success_count,
                                         stream_kip_disconnect_callback_handler);
    }
    else
    {
        unsigned count;

        /* find the offset to next P0 only transform */
        count = stream_kip_find_p0_only_transform_start((state->count -
                                                         state->success_count),
                                                         state->tr_list);

        /* Request to send remote transform disconnect */
        result = stream_kip_send_transform_disconnect(state->packed_con_id,
                                                      count,
                                                      (state->tr_list +
                                                      state->success_count),
                                                      (void*)state);
    }

    return result;
}

/**
 * \brief Generate a transform id and send a KIP stream connect request
 *        Sends a KIP connect REQ to secondary core(s) - only used on P0
 *
 * \param con_id     - The packed connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 * \param state      - The connect state info
 *
 * \return           TRUE on success
 */
bool stream_kip_connect_endpoints(CONNECTION_LINK packed_con_id,
                                  unsigned remote_source_id,
                                  unsigned remote_sink_id,
                                  STREAM_KIP_CONNECT_INFO *state)
{
    KIP_MSG_STREAM_CONNECT_REQ req;
    unsigned id;
    unsigned data_channel_id = 0;

    patch_fn_shared(stream_kip);

    /* Generate a transform id */
    id = stream_get_next_transform_id();

    /* add it in the remote transform list */
    if (stream_kip_add_transform_info(id, GET_RECV_PROC_ID(packed_con_id),
                                      remote_source_id, remote_sink_id,
                                      state->data_channel_id) == NULL)
    {
        return FALSE;
    }

    /* update the connect state info with the transform id */
    state->tr_id = (uint16)id;

    data_channel_id = (state->data_channel_id == 0) ?
                      0 : ipc_invert_chanid_dir(state->data_channel_id);

    /* pack the kip stream connect request */
    KIP_MSG_STREAM_CONNECT_REQ_SOURCE_ID_SET(&req, remote_source_id);
    KIP_MSG_STREAM_CONNECT_REQ_SINK_ID_SET(&req, remote_sink_id);
    KIP_MSG_STREAM_CONNECT_REQ_TRANSFORM_ID_SET(&req,
                                                STREAM_TRANSFORM_GET_EXT_ID(id));
    KIP_MSG_STREAM_CONNECT_REQ_CHANNEL_ID_SET(&req, data_channel_id);

    /* Now send the message */
    if (!kip_adaptor_send_message(packed_con_id,
                                  KIP_MSG_ID_STREAM_CONNECT_REQ,
                                  KIP_MSG_STREAM_CONNECT_REQ_WORD_SIZE,
                                  (uint16*) &req, (void*) state))
    {
        stream_kip_remove_transform_info_by_id(id);
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Send operator's endpoint ID request to another processor (P1)
 *
 * \param opid       - The operator id of which to request source/sink ep id
 * \param idx        - The endpoint source/sink channel index of the operator
 * \param direction  - Endpoint direction (source/sink)
 * \param con_id     - The connection id
 *
 * \return           TRUE on message successfully sent
 */
bool stream_kip_operator_get_endpoint(unsigned int opid,
                                      unsigned int idx,
                                      ENDPOINT_DIRECTION dir,
                                      CONNECTION_LINK con_id,
                                      bool (*callback)(CONNECTION_LINK con_id,
                                                       STATUS_KYMERA status,
                                                       unsigned source_id))
{
    KIP_MSG_OPERATOR_GET_ENDPOINT_REQ req;

    patch_fn_shared(stream_kip);

    KIP_MSG_OPERATOR_GET_ENDPOINT_REQ_OPID_SET(&req, opid);
    KIP_MSG_OPERATOR_GET_ENDPOINT_REQ_IDX_SET(&req, idx);
    KIP_MSG_OPERATOR_GET_ENDPOINT_REQ_DIRECTION_SET(&req, dir);

    /* Now send the message */
    return (kip_adaptor_send_message(con_id,
                                     KIP_MSG_ID_OPERATOR_GET_ENDPOINT_REQ,
                                     KIP_MSG_OPERATOR_GET_ENDPOINT_REQ_WORD_SIZE,
                                     (uint16*) &req, (void*) callback));
}

/****************************************************************************
 *  Audio second core section
 ****************************************************************************/

/**
 * \brief Handle the remove transform entry from P0 kip_transform_list
 *        response from P0. The function is only provided for Px.
 *
 * \param con_id        The connection id
 * \param status        status of the request
 * \param count         The number of removed pxcopy transforms
 * \param state         The disconnect state
 */
void stream_kip_transform_list_remove_entry_response_handler(CONNECTION_LINK con_id,
                                                             STATUS_KYMERA status,
                                                             unsigned count,
                                                             void *state)
{

    patch_fn_shared(stream_kip);

    /*
     * Ignore the response. If the transform was present on the P0
     * kip_transform_list, it will have been removed. If it was not,
     * it will not have been removed - but we don't care as we're
     * just interested removing the transform from kip_transform_list
     * on P0 if the transform existed in the list.
     */
    return;
}

/**
 * \brief Handling the incoming stream connect request from P0
 *
 * \param con_id       - The connection id
 * \param source_id    - The source endpoint id
 * \param sink_id      - The sink endpoint id
 * \param transform_id - The transform id
 * \param channel_id   - The data channel id
 */
void stream_kip_connect_request_handler(CONNECTION_LINK con_id,
                                        unsigned source_id,
                                        unsigned sink_id,
                                        unsigned transform_id,
                                        unsigned channel_id)
{
    bool valid = FALSE;
    STREAM_KIP_CONNECT_INFO* state_info = NULL;
    STREAM_CONNECT_INFO *state = NULL;

    patch_fn_shared(stream_kip);

    /* If data channel has been created already , it must have been activated*/
    if ((channel_id != 0) && (transform_id != 0))
    {
        /* Get the latest context and verify */
        if ((STREAM_KIP_STATE_IN_CONNECT()              ) &&
            (NULL != stream_kip_state_get_connect_info())    )
        {
            ENDPOINT* ep;
            state_info = stream_kip_state_get_connect_info();
            state = &state_info->connect_info;
            ep = stream_kip_get_kip_endpoint_from_state(state_info);

            valid = ((channel_id == state_info->data_channel_id) &&
                     (state->buffer_info.buffer != NULL) && (ep != NULL));

            if (valid)
            {
                /* Create the transform record */
                valid = (stream_kip_add_transform_info(
                                    STREAM_TRANSFORM_GET_INT_ID(transform_id),
                                    GET_SEND_PROC_ID(con_id),
                                    source_id, sink_id,
                                (uint16) channel_id) != NULL);
            }
        }
    }
    else
    {
        /* Channel id is 0 when the request has received either both operators
         * are on P1 or one of the endpoint in P1 must be an operator endpoint
         * and other endpoint is an audio endpoint when audio endpoints are
         * delegated (Audio endpoint delegation is not supported).
         *
         * The connection context also must be idle.
         */
        valid = STREAM_KIP_VALIDATE_EPS(source_id, sink_id) &&
                stream_kip_state_to_connect(NULL) &&
                (transform_id != 0);
    }

    if (valid)
    {
        /* Connect both operators at P1 */
        stream_if_transform_connect(con_id, source_id, sink_id,
                                    transform_id, state,
                                    stream_kip_send_connect_resp);
    }
    else
    {
        stream_kip_send_connect_resp(REVERSE_CONNECTION_ID(con_id),
                                         STATUS_CMD_FAILED, 0);
    }
}

/**
 * \brief Handle the incoming stream connect confirm request
 *        from the primary core
 *
 * \param con_id       - The connection id
 * \param conn_to      - The shadow endpoint ID connected to
 */
void stream_kip_connect_confirm_handler(CONNECTION_LINK con_id,
                                        unsigned conn_to)
{
    unsigned shadow_id;
    ENDPOINT *shadow_ep;
    endpoint_shadow_state *state;
    tCbuffer *shared_buffer;

    patch_fn_shared(stream_kip);

    shadow_id = STREAM_GET_SHADOW_EP_ID(conn_to);
    shadow_ep = stream_endpoint_from_extern_id(shadow_id);
    state = &shadow_ep->state.shadow;

    /* Propagate potential usable_octets from shared_buffer */
    shared_buffer = ipc_data_channel_get_cbuffer( state->channel_id);
    STREAM_KIP_ASSERT( shared_buffer != NULL );
    cbuffer_set_usable_octets( state->buffer, cbuffer_get_usable_octets(shared_buffer) );
}

/**
 * \brief Handling the incoming destroy endpoints request from P0
 *
 * \param con_id     - The connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 */
void stream_kip_destroy_endpoints_request_handler(CONNECTION_LINK con_id,
                                                  unsigned source_id,
                                                  unsigned sink_id)
{
    STATUS_KYMERA status = STATUS_CMD_FAILED;
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES resp;

    patch_fn_shared(stream_kip);

    /* Allowed to destroy the endpoints only during connect state */
    if ((STREAM_KIP_STATE_IN_CONNECT()              ) &&
        (NULL != stream_kip_state_get_connect_info())    )
    {
         STREAM_KIP_CONNECT_INFO* state_info;
         state_info = stream_kip_state_get_connect_info();

         if ((state_info->source_id == source_id) &&
             (state_info->sink_id == sink_id))
        {
            /* clean up */
            stream_kip_destroy_endpoint_ids(state_info);
            status = STATUS_OK;
        }
        stream_kip_state_to_none(TRUE);
    }

    KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES_STATUS_SET(&resp, status);

    kip_adaptor_send_message(REVERSE_CONNECTION_ID(con_id),
                             KIP_MSG_ID_STREAM_DESTROY_ENDPOINTS_RES,
                             KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES_WORD_SIZE,
                             (uint16 *) &resp, NULL);
}

/**
 * \brief Handling the incoming create endpoints request from P0
 *
 * \param con_id      - The connection id
 * \param source_id   - The source endpoint id
 * \param sink_id     - The sink endpoint id
 * \param channel_id  - The data channel id
 * \param buffer_size - The buffer size for negotiation
 * \param block_size  - The block size for ep timing info
 * \param period      - The period for ep timing info
 * \param flags       - The buffer related flags
 */
void stream_kip_create_endpoints_request_handler(CONNECTION_LINK con_id,
                                                 unsigned source_id,
                                                 unsigned sink_id,
                                                 unsigned channel_id,
                                                 unsigned buffer_size,
                                                 unsigned block_size,
                                                 unsigned period,
                                                 unsigned flags)
{
    bool result = FALSE, resp_result;
    STREAM_KIP_CONNECT_INFO *state = NULL;

    patch_fn_shared(stream_kip);

    /* If Pn is involved in a multi sequence kip command handling
     * it must be busy. Do not handle a new connection when KIP is busy.
     */
    if (STREAM_KIP_STATE_IN_IDLE() &&  STREAM_KIP_VALIDATE_EPS(source_id, sink_id))
    {
        STREAM_EP_LOCATION ep_location = STREAM_EP_IS_SHADOW_ID (source_id) ?
                              STREAM_EP_REMOTE_SOURCE : STREAM_EP_REMOTE_SINK;

        state = stream_kip_create_connect_info_record_ex(con_id,
                                                         source_id,
                                                         sink_id,
                                                         ep_location,
                                                         block_size,
                                                         period,
                                                 stream_kip_send_connect_resp);

        /* create the connect state record only for connection */
        if (state != NULL)
        {
            /* update the state buffer details */
            stream_kip_update_buffer_info(state, channel_id,
                                          buffer_size, flags);

            /* create the endpoints and set state */
            result = stream_kip_create_eps_for_connect(con_id, state);

            if (result)
            {
                /*
                 * At this point the flag below is already initialised by the call
                 * to stream_kip_update_buffer_info() but it only includes the
                 * information from the the remote endpoint. The intention of the
                 * flag is to include combined information from both endpoints so
                 * we update it here to reflect the overall result.
                 */
                if (stream_kip_endpoints_support_metadata(state))
                {
                    state->connect_info.buffer_info.flags.supports_metadata = TRUE;
                }
                else
                {
                    state->connect_info.buffer_info.flags.supports_metadata = FALSE;
                }

                /* Source will have to create the data channel and activate it */
                if ((ep_location == STREAM_EP_REMOTE_SINK))
                {

                    PROC_ID_NUM proc_id = GET_SEND_PROC_ID(state->packed_con_id);
                    result = kip_get_buffer_and_activate_channels(state, proc_id);
                    if (result)
                    {
                        /* If channel activation is successful, endpoints
                         * response will be sent after receiving
                         * DATA_CHANNEL_ACTIVATED event.
                         */
                         return;
                    }

#ifdef STREAM_CONNECT_FAULT_CODE
                    stream_connect_fault = SC_KIP_GET_DATA_CHANNEL_FAILED;
                    L1_DBG_MSG("stream_connect_fault: Failed to get_data_channel");
#endif /* STREAM_CONNECT_FAULT_CODE */
                }
                else
                {
                    /* Get the endpoints buffer details and
                     * it is not expected to fail*/
                    result = (channel_id != 0) ?
                             stream_kip_ep_get_buffer_info(state) : FALSE;
                }
            }
        }
    }

    resp_result = stream_kip_send_create_endpoints_resp(
                      REVERSE_CONNECTION_ID(con_id),
                      (result) ? STATUS_OK : STATUS_CMD_FAILED);

    if (!resp_result || !result)
    {
        /* free the connect resources */
        stream_kip_destroy_endpoint_ids(state);

        stream_kip_state_to_none(TRUE);
    }
}

/**
 * \brief Return the external id for a specified operator endpoint on Px
 *        (x not 0). Whether a source or sink is requested is parameterised.
 *        If the source could not be found then the id will be returned as 0.
 *        Note: like stream_if_get_endpoint but specifically for enquiries
 *        from P0 on Px as to the operator endpoint ID
 *        (ACCMD stream_get_source/sink_req).
 *
 * \param con_id     - connection ID of the originator of this request
 * \param device     - the endpoint device type, such as SCO, USB, PCM or I2S.
 * \param num_params - the number of parameters passed in the params array
 * \param params     - an array of typically 2 values a supporting parameter
 *                     that typically specifies the particular instance and
 *                     channel of the device type.
 * \param dir        - Whether a source or sink is being requested
 * \param callback   - The callback function to be called when
 *                     the result is known
 */
void stream_kip_px_if_get_endpoint(CONNECTION_LINK con_id,
                                   unsigned device,
                                   unsigned num_params,
                                   unsigned *params,
                                   ENDPOINT_DIRECTION dir,
                                   bool (*callback)(CONNECTION_LINK con_id,
                                                    STATUS_KYMERA status,
                                                    unsigned source_id))
{
    STATUS_KYMERA status = STATUS_CMD_FAILED;
    ENDPOINT *ep = NULL;

    patch_fn_shared(stream_if);

    /* Right then this is the first interface that is going to get called */
    switch (device)
    {
        case STREAM_DEVICE_OPERATOR :
            if (num_params == 2)
            {
                ep = stream_operator_get_endpoint(params[0], params[1], dir, con_id);
            }
            else
            {
                status = STATUS_INVALID_CMD_LENGTH;
            }
            break;
    }

    if (ep)
    {
        /* Check that the endpoint is tagged with this user - only look at "owner", i.e. what is sender ID */
        /* in the connection ID. If matches, send a response and make sure connection ID is reversed. */
        if (GET_CON_ID_SEND_ID(ep->con_id) == GET_CON_ID_SEND_ID(con_id))
        {
            status = STATUS_OK;
            callback(REVERSE_CONNECTION_ID(con_id), status, stream_external_id_from_endpoint(ep));
            return;
        }
    }

    callback(REVERSE_CONNECTION_ID(con_id), status, 0);
}

/**
 * \brief Handling the incoming stream disconnect request from P0
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to disconnect
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_disconnect_request_handler(CONNECTION_LINK con_id,
                                                     unsigned count,
                                                     unsigned *tr_list)
{
    patch_fn_shared(stream_kip);

    /* Process the request only if the state is idle */
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        /* set the disconnect state */
        stream_kip_state_to_disconnect(NULL);

        /*
         * Make it deactivate ready hence the endpoint will deactivate
         * while disconnecting.
         */
        stream_kip_transform_deactivate_ready(count, tr_list);

        stream_if_part_transform_disconnect(con_id, count, tr_list, 0,
                                stream_kip_send_transform_disconnect_resp);
    }
    else
    {
        stream_kip_send_transform_disconnect_resp(
                                  REVERSE_CONNECTION_ID(con_id),
                                  STATUS_CMD_FAILED, 0);
    }
}

/**
 * \brief Handling the incoming kip_transform_list entry remove
 *        request from secondary core
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to cleanup/remove
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_list_remove_entry_request_handler(CONNECTION_LINK con_id,
                                                            unsigned count,
                                                            unsigned *tr_list)
{
    unsigned i;
    STATUS_KYMERA status;

    patch_fn_shared(stream_kip);

    /* Process the request only if the state is idle */
    if (STREAM_KIP_STATE_IN_IDLE())
    {
        /* set the disconnect state */
        stream_kip_state_to_disconnect(NULL);

        for (i = 0; i < count; i++)
        {
            stream_kip_remove_transform_info_by_id(STREAM_TRANSFORM_GET_INT_ID(tr_list[i]));
        }

        status = STATUS_OK;
    }
    else
    {
        status = STATUS_CMD_FAILED;
        count  = 0;
    }

    stream_kip_transform_list_remove_entry_resp(
                              REVERSE_CONNECTION_ID(con_id),
                              status, count);
}

/**
 * \brief Indication from IPC when the data channel activated
 *
 * \param status     - STATUS_OK on success
 * \param proc_id    - The remote processor id connected to the data channel
 * \param channel_id - The data channel id
 * \param param_len  - param length ( expected 0 and ignored)
 * \param params     - params ( none expected)
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_activated(STATUS_KYMERA status,
                                                PROC_ID_NUM proc_id,
                                                uint16 channel_id,
                                                unsigned param_len,
                                                void* params)
{
    patch_fn_shared(stream_kip);

    STREAM_KIP_CONNECT_INFO *state = stream_kip_state_get_connect_info();
    ENDPOINT *ep;

    /* Validate it with the active context. If it is
     * not found, there is something wrong. panic
     */
    if (!STREAM_KIP_STATE_IN_CONNECT() || (state == NULL) ||
        PROC_ON_SAME_CORE(proc_id))
    {
        fault_diatribe(FAULT_AUDIO_MULTICORE_CONNECT_INVALID_STATE, (proc_id) | ((stream_kip_state.state)<<2));

        status = STATUS_CMD_FAILED;

#ifdef STREAM_CONNECT_FAULT_CODE
        stream_connect_fault = SC_KIP_INVALID_STATE;
        L1_DBG_MSG("stream_kip_data_channel_activated: Lost state  ");
#endif /* STREAM_CONNECT_FAULT_CODE */

        /* We cannot recover this state */
        return status;
    }

    ep = stream_kip_get_kip_endpoint_from_state(state);

    STREAM_KIP_ASSERT(ep != NULL);

    if (DATA_CHANNEL_IS_META(channel_id))
    {
        if (ep == NULL)
        {
            /*
             * The ep for the metadata's partner channel (for example with ID
             * 0x180 for metadata channel ID 0x18F) is not found. This could
             * happen when 0x180 wasn't created (e.g. due to a fault) and the
             * setup nevertheless carries on trying to activate other channels.
             */
            fault_diatribe(FAULT_AUDIO_METADATA_PARTNER_EP_MISSING, channel_id);
            status = STATUS_CMD_FAILED;
        }

#ifdef STREAM_CONNECT_FAULT_CODE
        if (!state->data_channel_is_activated)
        {
            L3_DBG_MSG("stream_kip_data_channel_activated: inactivated data channel present");
        }
#endif /* STREAM_CONNECT_FAULT_CODE */

        /* Set metadata channel ID, buffer and flag in the shadow endpoint */
        ep->functions->configure(ep, EP_METADATA_CHANNEL_ID, channel_id);
        ep->functions->configure(ep, EP_METADATA_CHANNEL_BUFFER,
                                 (uintptr_t)ipc_data_channel_get_cbuffer(channel_id));
        ep->functions->configure(ep, EP_METADATA_SUPPORT, TRUE);

    }
    else
    {
        /* update the data channel id in KIP endpoint */
        ep->functions->configure(ep, EP_SET_DATA_CHANNEL, channel_id);
    }

    /*
     * Data channel can be activated by the local processor or remote
     * processor. This must be activated by the side that supplies
     * buffer. i.e when the remote endpoint is sink
     *
     * There are 4 cases
     * 1. Primary core initiated activation (remote sink case)
     *     - Proceed with stream connect request on success
     *
     * 2. Secondary core initiated activation (remote source case)
     *    - Respond the stream create endpoints request
     *
     * 3. Secondary core notified for case 1
     *     - Primary core will proceed with connection or
     *       disconnection
     *
     * 4. Primary core notified for case 2
     *     - Do Nothing. Secondary core will notify endpoint creation
     *       status.
     *
     */
    if (state->ep_location == STREAM_EP_REMOTE_SOURCE)
    {
        if (status == STATUS_OK)
        {
            uint32 *source_data_format = (uint32*) params;
            ENDPOINT *source_ep = stream_endpoint_from_extern_id(state->source_id);
            ENDPOINT *sink_ep = stream_endpoint_from_extern_id(state->sink_id);

            /* Activation initiated by remote side. case 3 & 4*/
            if (DATA_CHANNEL_IS_META(channel_id))
            {
                state->meta_channel_id = channel_id;
            }
            else
            {
                /* update the params */
                if (param_len < 2)
                {
                    return STATUS_INVALID_CMD_PARAMS;
                }

                state->data_channel_id = channel_id;
                source_ep->functions->configure(source_ep,
                                         EP_DATA_FORMAT,
                                         *source_data_format);

                /* Requesting to clone the buffer will create a
                 * clone of the remote buffer. This is called for
                 * the remote source endpoint only when data channel
                 * activation is completed.
                 * Create a clone buffer every time to enable metadata
                 * sync if enabled.
                 */
                source_ep->functions->configure(source_ep,
                                         EP_CLONE_REMOTE_BUFFER, 1);

                /* Now get the buffer which will be a cloned one
                 * if the kip endpoint is connected to a real endpoint.
                 */
                if (!stream_connect_get_buffer(source_ep, sink_ep,
                                            &state->connect_info))
                {
                    status = STATUS_CMD_FAILED;
                }
            }
        }

        return status;
    }

    /* Remote Sink only cases reach here */
    if (status != STATUS_OK)
    {
        /* Activation attempt failed. destroy the data channel */
        if (state->meta_channel_id == channel_id)
        {
            bool data_channel_status;

            ipc_destroy_data_channel(state->meta_channel_id);
            data_channel_status = stream_kip_data_channel_deactivate_ipc(state->data_channel_id);

            STREAM_KIP_ASSERT(data_channel_status);
        }
        else
        {
            /* Activation attempt failed. destroy the data channel */
            ipc_destroy_data_channel(state->data_channel_id);
        }
        /* don't destroy the endpoints now */
    }

    if (proc_id == PROC_PROCESSOR_0)
    {
        /* On Pn - case 2 */
        if (status == STATUS_OK)
        {
            if (state->data_channel_id == channel_id)
            {
                state->data_channel_is_activated = TRUE;
            }
            else
            {
                state->metadata_channel_is_activated = TRUE;
                state->meta_channel_id = channel_id;
                /* Update metedata_buffer information based on previous connections if any */
                stream_kip_update_metadata_buffer(state);
            }
        }

        if ((state->connect_info.buffer_info.flags.supports_metadata && BOTH_CHANNELS_ARE_ACTIVATED(state))
            ||
            (!(state->connect_info.buffer_info.flags.supports_metadata)))
        {
            bool res = stream_kip_send_create_endpoints_resp(
                           REVERSE_CONNECTION_ID(state->packed_con_id),
                           (status == STATUS_OK) ? status : STATUS_CMD_FAILED);

            if (!res || (status != STATUS_OK))
            {
                stream_kip_destroy_endpoint_ids(state);

                /* Free the global context and unblock KIP */
                stream_kip_state_to_none(TRUE);
            }
        }
    }

    if (proc_id != PROC_PROCESSOR_0)
    {
        bool result = FALSE;

        /* case 1 : Primary core initiated activation completed.
         * On Success,
         *    -  initiate the stream connect process
         *
         * On Failure
         *      - Send remote request to destroy the endpoints
         */
        if (status == STATUS_OK)
        {
            if (state->data_channel_id == channel_id)
            {
                state->data_channel_is_activated = TRUE;
            }
            else
            {
                state->metadata_channel_is_activated = TRUE;
                state->meta_channel_id = channel_id;
                /* Update metedata_buffer information based on previous connections if any */
                stream_kip_update_metadata_buffer(state);
            }
            if ((state->connect_info.buffer_info.flags.supports_metadata) &&
                 !BOTH_CHANNELS_ARE_ACTIVATED(state))
            {
                /* If not both data and metadata channels are activated,
                 * wait for the other connection and return STATUS_OK */
                return STATUS_OK;

            }
            else /* If we don't support metadata */
            {
                /* Send a KIP request to connect endpoints */
                result = stream_kip_connect_endpoints(state->packed_con_id,
                                STREAM_GET_SHADOW_EP_ID(state->source_id),
                                STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                                state);
            }
        }
        if (!result)
        {
            /* Send a KIP request to destroy endpoints */
            stream_kip_destroy_endpoints(state->packed_con_id,
                            STREAM_GET_SHADOW_EP_ID(state->source_id),
                            STREAM_EP_ID_FROM_SHADOW_ID(state->sink_id),
                            state);

        }
    }

    return STATUS_OK;
}

/**
 * \brief Indication from IPC when the data channel deactivated
 *
 * \param status     - STATUS_OK on success
 * \param channel_id - The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_deactivated(STATUS_KYMERA status,
                                                  uint16 channel)
{
    STREAM_KIP_TRANSFORM_INFO *tr;

    tr = stream_kip_transform_info_from_chanid(channel);
    if ((tr != NULL) && (status == STATUS_OK))
    {
        tr->enabled = FALSE;
    }

    /*
     * For data channels, IPC allocates and destroys its own cbuffer structure
     * but for metadata channels, we instruct IPC to directly use the passed
     * cbuffer which means IPC cannot destroy it. The destroy is done here
     * because it's the last place we can retrieve the pointer from IPC.
     */
    tCbuffer* cbuf = ipc_data_channel_get_cbuffer(channel);
    if (DATA_CHANNEL_IS_META(channel) &&
        (IPC_DATA_CHANNEL_WRITE == ipc_get_data_channelid_dir(channel)) &&
        (cbuf != NULL))
    {
        cbuffer_destroy(cbuf);
    }

    return STATUS_OK;
}

/**
 * \brief  Deactivate the data channel from ipc
 *         Shadow endpoint calls this directly to deactivate meta data channel
 *
 * \param channel - The data channel to be deactivated.
 *
 * \return FALSE if deactivation failed, TRUE if success
 */
bool stream_kip_data_channel_deactivate_ipc(uint16 channel)
{
    if (ipc_get_data_channelid_dir(channel) == IPC_DATA_CHANNEL_WRITE)
    {
        return (ipc_deactivate_data_channel(channel) == IPC_SUCCESS);
    }
    return TRUE;
}

/**
 * \brief  Shadow endpoint calls this to deactivate data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_deactivate(uint16 channel)
{
    bool result = TRUE;

    STREAM_KIP_TRANSFORM_INFO *tr;
    tr = stream_kip_transform_info_from_chanid(channel);

    if (tr != NULL)
    {
        result = stream_kip_data_channel_deactivate_ipc(channel);

        if (result)
        {
            stream_kip_remove_transform_info(tr);
        }
    }

    return result;
}

/**
 * \brief  Destroy the data channel from ipc
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy_ipc(uint16 channel)
{
    if (ipc_get_data_channelid_dir(channel) == IPC_DATA_CHANNEL_WRITE)
    {
        return (ipc_destroy_data_channel(channel) == IPC_SUCCESS);
    }
    return TRUE;
}

/**
 * \brief  Shadow endpoint calls this to destroy the data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy(uint16 channel)
{
    bool result = TRUE;

    patch_fn_shared(stream_kip);

    /*
     * to destroy the data channel, it must have been already
     * removed from the transform list
     */
    if (!stream_kip_transform_info_from_chanid(channel))
    {
        result = stream_kip_data_channel_destroy_ipc(channel);
    }
    else
    {
        result = FALSE;
    }

     return result;
}

/**
 * \brief   Get metadata_buffer from the same endpoint base
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  metadata_buffer if it found any, otherwise, NULL
 */
tCbuffer *stream_kip_return_metadata_buf(ENDPOINT *endpoint)
{
    tCbuffer *metadata_buffer = NULL;
    unsigned endpoint_base = GET_BASE_EPID_FROM_EPID(endpoint->id);
    ENDPOINT *ep = (STREAM_EP_SINK_BIT & endpoint_base) ?
                                    sink_endpoint_list :
                                    source_endpoint_list;

    while (ep != NULL)
    {
        if (STREAM_EP_IS_SHADOW_ID(ep->id) && (GET_BASE_EPID_FROM_EPID(ep->id) == endpoint_base))
        {
            /* Now we need to check both of the endpoint are having the same meta buffer */
            if ((stream_shadow_get_shared_metadata_buffer(ep) == stream_shadow_get_shared_metadata_buffer(endpoint)) &&
               (ep != endpoint))
            {
                metadata_buffer = stream_shadow_get_metadata_buffer(ep);
                break;
            }
        }
        ep = ep->next;
    }

    return metadata_buffer;
}

/**
 * \brief   Check if this endpoint is in the last metadata data connection
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  TRUE if it is, otherwise, FALSE;
 */
bool stream_kip_is_last_meta_connection(ENDPOINT *endpoint)
{
    if (stream_kip_return_metadata_buf(endpoint) != NULL)
    {
        /* Another connection exists */
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief   Request remote to set the activated flag in the kip state with
 *          an existing metadata data channel. Then, send a response back
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   meta_channel_id - The existing metadata data channel id
 */
void stream_kip_metadata_channel_activated_req_handler(CONNECTION_LINK packed_con_id,
                                                       uint16 meta_channel_id)
{
    /* At this point, we must have a existing metadata data channel so the IPC status is SUCCESS. */
    STATUS_KYMERA status = STATUS_OK;
    PROC_ID_NUM remote_proc_id;

    if (PROC_PRIMARY_CONTEXT())
    {
        remote_proc_id = GET_RECV_PROC_ID(packed_con_id);
    }
    else
    {
        remote_proc_id = GET_SEND_PROC_ID(packed_con_id);
    }

    /* Since the channel has been activated, we just need to set the activated flag */
    status = stream_kip_data_channel_activated(status, remote_proc_id,
                                               meta_channel_id, 0, NULL);

    /* We don't expect a failure here */
    STREAM_KIP_ASSERT(status == STATUS_OK);

    /* Now send a response back to activate the channel on the other core */
    stream_kip_send_metadata_channel_activated_resp(packed_con_id, status,
                                (uint16)ipc_invert_chanid_dir(meta_channel_id));
}

/**
 * \brief   Response to local to set the activated flag in the kip state
 *          with an existing metadata data channel
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   status          - STATUS_KYMERA
 * \param   meta_channel_id - The existing metadata data channel id
 */
void stream_kip_metadata_channel_activated_resp_handler(CONNECTION_LINK packed_con_id,
                                                        STATUS_KYMERA status,
                                                        uint16 meta_channel_id)
{
    PROC_ID_NUM remote_proc_id;

    if (PROC_PRIMARY_CONTEXT())
    {
        remote_proc_id = GET_RECV_PROC_ID(packed_con_id);
    }
    else
    {
        remote_proc_id = GET_SEND_PROC_ID(packed_con_id);
    }

    /* Since the channel has been activated, we just need to set the activated flag */
    status = stream_kip_data_channel_activated(status, remote_proc_id,
                                               meta_channel_id, 0, NULL);

    /* We don't expect a failure here */
    STREAM_KIP_ASSERT(status == STATUS_OK);
}

/**
 * \brief kick the kip endpoints on receiving kip signals
 *
 * \param data_channel_id data channel id
 * \param kick_dir  The kick direction
 *
 * \return void
 */
void stream_kick_kip_eps(uint16 data_chan_id,
                         ENDPOINT_KICK_DIRECTION kick_dir)
{
    ENDPOINT *sync_ep = NULL;
    ENDPOINT *ep = stream_shadow_ep_from_data_channel(data_chan_id);
    uint16 port_id = ipc_get_data_channelid_port(data_chan_id);

    patch_fn_shared(stream);
    if (ep != NULL)
    {
        ep->functions->kick(ep, kick_dir);
    }

    if (kick_dir == STREAM_KICK_BACKWARDS)
    {
        /* don't kick the group in case of remote bakward kicks */
        return;
    }

    sync_ep = (ep->direction == SINK ) ? sink_endpoint_list:
                                         source_endpoint_list;

    while (sync_ep != NULL)
    {
        uint16 pid;

        if ((sync_ep != ep) && STREAM_EP_IS_SHADOW(sync_ep))
        {
            pid= ipc_get_data_channelid_port(stream_shadow_get_channel_id(sync_ep));
            if (pid == port_id)
            {
                sync_ep->functions->kick(sync_ep, kick_dir);
            }
        }

        sync_ep = sync_ep->next;
    }
}

bool stream_kip_raise_ipc_signal(ENDPOINT *ep, uint16 channel_id)
{
    ipc_signal sig;
    PROC_ID_NUM proc_id;
    uint16 signal_id = (ep->direction == SINK) ? KIP_SIGNAL_ID_KICK : KIP_SIGNAL_ID_REVERSE_KICK;

    if (PROC_PRIMARY_CONTEXT())
    {
        proc_id = PROC_PROCESSOR_1;
    }
    else
    {
        proc_id = PROC_PROCESSOR_0;
    }

    sig.signal_size = sizeof(uint16);
    sig.signal_ptr = kip_get_signal_ptr(channel_id, signal_id);

    return (ipc_raise_signal(proc_id, signal_id, &sig) == IPC_SUCCESS);
}
