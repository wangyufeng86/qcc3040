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

#ifndef STREAM_KIP_H
#define STREAM_KIP_H

/****************************************************************************
Include Files
*/

#include "status_prim.h"
#include "stream/stream.h"
#include "stream/stream_transform.h"
#include "proc/proc.h"

/****************************************************************************
Type Declarations
*/

/* Do not change these value since it is used for bit operation
 * Location of the endpoint based on the processor context. From P0
 * STREAM_EP_REMOTE_ALL means source and sink endpoints are not located
 * with P0 (on dual core, those are with P1)
 */
typedef enum
{
    STREAM_EP_REMOTE_NONE   = 0x0,
    STREAM_EP_REMOTE_SOURCE = 0x1,
    STREAM_EP_REMOTE_SINK   = 0x2,
    STREAM_EP_REMOTE_ALL    = 0x3
} STREAM_EP_LOCATION;

/* Structure for keeping record of connect stages when shadow EPs are involved */
typedef struct
{
    /* Packed connection ID with remote processor id */
    uint16 packed_con_id;

    /* Local endpoint ids
     * This might be a external operator endpoint id or a real endpoint id
     * or a shadow operator endpoint id.
     */
    uint16 source_id;
    uint16 sink_id;

    /* internal transform id */
    uint16 tr_id;

    uint16 data_channel_id;

    uint16 meta_channel_id;
    bool metadata_channel_is_activated : 1;

    bool data_channel_is_activated : 1;

    /* endpoint locations */
    STREAM_EP_LOCATION ep_location;

    STREAM_CONNECT_INFO connect_info;

    /* The callback to notify connection status */
    bool (*callback)(CONNECTION_LINK con_id,
                     STATUS_KYMERA status,
                     unsigned transform_id);

    /* Copy of operator timing info for shadowed endpoint */
    unsigned tinfo_block_size;
    unsigned tinfo_period;

} STREAM_KIP_CONNECT_INFO;

typedef union
{
    bool (*tr_disc_cb)(CONNECTION_LINK con_id,
                       STATUS_KYMERA status,
                       unsigned count);
    bool (*disc_cb)(CONNECTION_LINK con_id,
                    STATUS_KYMERA status,
                    unsigned transform_id_source,
                    unsigned transform_id_sink);
}STREAM_KIP_TRANSFORM_DISCONNECT_CB;

/* Structure for keeping record of transform disconnect
 * when shadow EPs are involved
 **/
typedef struct
{
    /* Packed connection ID with remote processor id */
    uint16 packed_con_id;

    /* count */
    uint16 count;

    /* success count */
    uint16 success_count;

    /* flag to show which callback to call */
    bool disc_cb_flag;

    /* remote success count */
    uint16 remote_success_count;

    /* The callback to notify transform disconnect status */
    STREAM_KIP_TRANSFORM_DISCONNECT_CB callback;

    /* transform list. Don't change the position from here */
    unsigned tr_list[1];

} STREAM_KIP_TRANSFORM_DISCONNECT_INFO;

/**
 * \brief kick the kip endpoints on receiving kip signals
 *
 * \param data_channel_id data channel id
 * \param kick_dir  The kick direction
 *
 * \return void
 */
void stream_kick_kip_eps(uint16 data_chan_id,
                         ENDPOINT_KICK_DIRECTION kick_dir);

/* Structure used for maintaining remote kip transform information.
 * P0 uses the same to maintain remote transform list as well */
typedef struct STREAM_KIP_TRANSFORM_INFO
{
    /* This is the source id at the secondary core*/
    unsigned source_id;

    /* This is the sink id at the secondary core*/
    unsigned sink_id;

    /* data channel id. 0 if it is not present */
    uint16 data_channel_id;

    /* Metadata channel ID */
    uint16 meta_channel_id;

    /* External transform id */
    unsigned id:8;

    /* Remote Processor id */
    unsigned processor_id:3;

    /* Enable to allow using the data channel connected to
     * the transform. Disable it to allow deactivating the
     * data channel.
     */
    bool enabled:1;

    /* Real source ep */
    bool real_source_ep:1;

    /* Real sink ep */
    bool real_sink_ep:1;

    struct STREAM_KIP_TRANSFORM_INFO* next;
} STREAM_KIP_TRANSFORM_INFO;

/****************************************************************************
Constant Declarations
*/

/****************************************************************************
Macro Declarations
*/

#ifdef KIP_DEBUG
#define STREAM_KIP_ASSERT(x) PL_ASSERT(x)
#else
#define STREAM_KIP_ASSERT(x) (void)(x)
#endif

#ifndef INSTALL_DELEGATE_AUDIO_HW

#define STREAM_KIP_VALIDATE_EPS(ep1, ep2) \
            (!(STREAM_EP_IS_REALEP_ID(ep1) || STREAM_EP_IS_REALEP_ID(ep2)))
#else

#define STREAM_KIP_VALIDATE_EPS(ep1, ep2) \
            (!(STREAM_EP_IS_REALEP_ID(ep1) && STREAM_EP_IS_REALEP_ID(ep2)))
#endif

/****************************************************************************
Variable Definitions
*/
/**
 * This keeps information about the remote transforms. On P0, this list contains
 * both transform associated with KIP endpoints as well as P1 transforms.
 * On P1 list, it contains only the KIP transforms. P0 and P1 local transforms
 * are maintained in its transform_list.
 **/
extern STREAM_KIP_TRANSFORM_INFO *kip_transform_list;

/****************************************************************************
Function Declarations
*/

/* IPC event callback handlers.*/
/**
 * \brief Indication from IPC when the data channel activated
 *
 * \param status     - STATUS_OK on success
 * \param proc_id    - The remote processor id connected to the data channel
 * \param channel_id - The data channel id
 * \param param_len  - param length (expected 0 and ignored)
 * \param params     - params (none expected)
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_activated(STATUS_KYMERA status,
                                                PROC_ID_NUM proc_id,
                                                uint16 channal_id,
                                                unsigned param_len,
                                                void* params);
/**
 * \brief Indication from IPC when the data channel deactivated
 *
 * \param status     - STATUS_OK on success
 * \param channel_id - The data channel id
 *
 * \return STATUS_KYMERA
 */
STATUS_KYMERA stream_kip_data_channel_deactivated(STATUS_KYMERA status,
                                                  uint16 channel);

/* TODO: This #ifndef is temporary, to not stop crescendo tests  */
/* Should properly resolve unit test build errors without ifndef */
#ifndef UNIT_TEST_BUILD
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
                                                                                   unsigned transform_id));
#endif

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
                                                                                unsigned transform_id));

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
                                  unsigned source_id,
                                  unsigned sink_id,
                                  STREAM_KIP_CONNECT_INFO *state);

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
                                 unsigned source_id,
                                 unsigned sink_id,
                                 STREAM_KIP_CONNECT_INFO *state);

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
                                         STREAM_KIP_CONNECT_INFO *state);

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
                                                      STREAM_KIP_TRANSFORM_DISCONNECT_INFO *state);

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
                                                  STREAM_KIP_CONNECT_INFO *state_info);

/**
 * \brief Handling the incoming destroy endpoint response.
 *
 * \param con_id      - The connection id
 * \param status      - Status
 * \param state_info  - The connection state information.
 */
void stream_kip_destroy_endpoints_response_handler(CONNECTION_LINK con_id,
                                                   STATUS_KYMERA status,
                                                   STREAM_KIP_CONNECT_INFO *state_info);

/**
 * \brief Find the first Px transform offset from the kip_transform_list
 *
 * \param count       - The total number of transforms in the list
 * \param tr_id_list  - The transform list
 *
 * \return            The first Px transform offset from the list
 */
unsigned stream_kip_find_px_transform_start(unsigned count, unsigned *tr_id_list);

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
                                                                               STREAM_KIP_TRANSFORM_DISCONNECT_CB callback);

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
                                     unsigned success_count);

/**
 * \brief Get the (external) transform id associated with a given endpoint, if any.
 *
 * \param endpoint [IN]  - The KIP endpoint
 * \param tr_id    [OUT] - The (external) transform id
 *
 * \return TRUE if a valid transform (id) was found, FALSE if not.
 */
bool stream_transform_id_from_endpoint(ENDPOINT *endpoint, unsigned *tr_id);

/**
 * \brief Request to P0 to remove entry from kip_transform_list 
 *        (where P0 keeps a copy/entry/id of each transform on Px).
 *
 * \param tr_id       - The external transform id
 * \param proc_id     - The processor ID
 *
 * \return            TRUE if successful
 */
bool stream_kip_cleanup_endpoint_transform(unsigned tr_id, PROC_ID_NUM proc_id);

/**
 * \brief Disconnect the transform associated with a KIP endpoint
 *
 * \param endpoint    - The KIP endpoint
 * \param proc_id     - The processor_id
 *
 * \return            TRUE if success, FALSE otherwise
 */
bool stream_kip_disconnect_endpoint(ENDPOINT *endpoint, PROC_ID_NUM proc_id);

/**
 * \brief  Destroy the data channel from ipc
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy_ipc(uint16 channel);

/**
 * \brief  Shadow endpoint calls this to destroy the data channel
 *
 * \param  channel - The data channel to be destroyed.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_destroy(uint16 channel);

/**
 * \brief  Deactivate the data channel from ipc
 *         Shadow endpoint calls this directly to deactivate meta data channel
 *
 * \param channel - The data channel to be deactivated.
 *
 * \return FALSE if deactivation failed, TRUE if success
 */
bool stream_kip_data_channel_deactivate_ipc(uint16 channel);

/**
 * \brief  Shadow endpoint calls this to deactivate data channel
 *
 * \param  channel - The data channel to be deactivated.
 *
 * \return FALSE if channel not found, TRUE if channel destroyed
 */
bool stream_kip_data_channel_deactivate(uint16 channel);

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param id          - KIP transform info id
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_id(unsigned id);

/**
 * \brief Get KIP transform info from KIP transform information list
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return            Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_epid(unsigned epid);

/**
 * \brief Helper function to find the ID of a remote endpoint
 *        connected to a known endpoint.
 *
 * \param epid        - The ID of the endpoint that is known
 *
 * \return The ID of the endpoint connected to endpoint with ID epid. 
 *         0 if not found.
 */
unsigned stream_kip_connected_to_epid(unsigned epid);

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
                                uint16 data_chan_id);

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param transform - Transform to be removed
 */
void stream_kip_remove_transform_info(STREAM_KIP_TRANSFORM_INFO *transform);

/**
 * \brief Helper function to remove remote transform info
 *        entry from respective list
 *
 * \param id - Internal transform id of transform to be removed
 */
void stream_kip_remove_transform_info_by_id(unsigned tr_id);

/**
 * \brief Helper function to retrieve entry in remote transform
 *        info list based on data channel ID.
 *
 * \param data_chan_id - data channel id
 *
 * \return             Pointer to KIP transform info object
 */
STREAM_KIP_TRANSFORM_INFO* stream_kip_transform_info_from_chanid(uint16 data_chan_id);

/**
 * \brief Handling the incoming stream disconnect request from P0
 *
 * \param con_id     - The connection id
 * \param count      - Number of transforms to disconnect
 * \param tr_list    - The list of transforms
 */
void stream_kip_transform_disconnect_request_handler(CONNECTION_LINK con_id,
                                                     unsigned count,
                                                     unsigned *tr_list);

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
                                                       unsigned source_id));

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
                                                            unsigned *tr_list);

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
                                                             void     *state);

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
                                        unsigned channel_id);

/**
 * \brief Handle the incoming stream connect confirm request
 *        from the primary core
 *
 * \param con_id       - The connection id
 * \param conn_to      - The shadow endpoint ID connected to
 */
void stream_kip_connect_confirm_handler(CONNECTION_LINK con_id,
                                        unsigned conn_to);



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
                                                 unsigned flags);

/**
 * \brief Handling the incoming destroy endpoints request from P0
 *
 * \param con_id     - The connection id
 * \param source_id  - The source endpoint id
 * \param sink_id    - The sink endpoint id
 */
void stream_kip_destroy_endpoints_request_handler(CONNECTION_LINK con_id,
                                                  unsigned source_id,
                                                  unsigned sink_id);

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
                                                    unsigned source_id));

/**
 * \brief   Get metadata_buffer from the same endpoint base
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  metadata_buffer if it found any, otherwise, NULL
 */
extern tCbuffer *stream_kip_return_metadata_buf(ENDPOINT *ep);

/**
 * \brief   Check if this endpoint is in the last metadata data connection
 *
 * \param   endpoint   - Pointer to endpoint object
 *
 * \return  TRUE if it is, otherwise, FALSE;
 */
extern bool stream_kip_is_last_meta_connection(ENDPOINT *endpoint);

/**
 * \brief   Request remote to set the activated flag in the kip state with
 *          an existing metadata data channel. Then, send a response back
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   meta_channel_id - The existing metadata data channel id
 */
extern void stream_kip_metadata_channel_activated_req_handler(CONNECTION_LINK packed_con_id,
                                                              uint16 meta_channel_id);
/**
 * \brief   Response to local to set the activated flag in the kip state
 *          with an existing metadata data channel
 *
 * \param   packed_con_id   - Packed send/receive connection ID
 * \param   status          - STATUS_KYMERA
 * \param   meta_channel_id - The existing metadata data channel id
 */
extern void stream_kip_metadata_channel_activated_resp_handler(CONNECTION_LINK packed_con_id,
                                                               STATUS_KYMERA status,
                                                               uint16 meta_channel_id);

/**
 * \brief   Raise a signal on a channel associated with an endpoint
 *
 * \param   ep          The endpoint
 * \param   channel_id  The channel identifier
 *
 * \return  True if raising the signal was successful.
 */
extern bool stream_kip_raise_ipc_signal(ENDPOINT *ep, uint16 channel_id);
                                                       
#endif /* STREAM_KIP_H */
