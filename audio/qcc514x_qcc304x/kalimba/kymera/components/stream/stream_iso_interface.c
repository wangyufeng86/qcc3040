/****************************************************************************
 * Copyright (c) 2018 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  stream_iso_hydra.c
 * \ingroup stream
 *
 * stream iso type file. <br>
 * This file contains stream functions for iso endpoints. <br>
 *
 * \section public Contains:
 * stream_iso_get_endpoint <br>
 * stream_create_iso_endpoints_and_cbuffers <br>
 * stream_delete_iso_endpoints_and_cbuffers <br>
 */

/****************************************************************************
Include Files
*/

#include "stream_private.h"
#include "stream_endpoint_iso.h"
#include "stream/stream_for_sco_operators.h"

/****************************************************************************
Private Type Declarations
*/

/****************************************************************************
Private Constant Declarations
*/
/** The location of the hci handle in the iso_get_endpoint params */
#define HCI_HANDLE  0

#ifdef INSTALL_ISO_CHANNELS
/****************************************************************************
Private Macro Declarations
*/

/****************************************************************************
Private Variable Definitions
*/

/****************************************************************************
Private Function Declarations
*/
static bool iso_set_up_endpoint(ENDPOINT * ep, ENDPOINT_DIRECTION dir, sco_buf_desc * buf_desc);
static void clean_up_endpoint(ENDPOINT * ep);
static inline void iso_rx_reset_toa_packet_offset(endpoint_iso_state *iso);
static inline void clean_up_iso(ENDPOINT * source_ep, ENDPOINT * sink_ep);

/****************************************************************************
Public Function Definitions
*/

/****************************************************************************
 *
 * stream_iso_get_endpoint
 *
 */
ENDPOINT *stream_iso_get_endpoint(CONNECTION_LINK con_id,
                                  ENDPOINT_DIRECTION dir,
                                  unsigned num_params,
                                  unsigned *params)
{
    ENDPOINT *ep;
    unsigned key;

    patch_fn_shared(stream_iso_interface);

    /* Expect an hci handle and potentially some padding */
    if (num_params < 1)
    {
        L3_DBG_MSG("hydra stream_iso_get_endpoint (num_params < 1) return NULL");
        return NULL;
    }
    /* The hci handle forms the key (unique for the type and direction) */
    key = params[HCI_HANDLE];

    L3_DBG_MSG1("hydra stream_iso_get_endpoint hci handle: %d", key);

    /* Return the requested endpoint (NULL if not found) */
    ep = iso_get_endpoint(key, dir);
    if (ep)
    {
        /* The endpoint has been created, however we now need to check
           the ID */
        if (ep->con_id == INVALID_CON_ID)
        {
            ep->con_id = con_id;
        }
        /* If the client does not own the endpoint they can't access it. */
        else if (ep->con_id != con_id)
        {
            L3_DBG_MSG("hydra stream_iso_get_endpoint (ep->con_id != con_id) return NULL");
            return NULL;
        }
    }
    L3_DBG_MSG1("hydra stream_iso_get_endpoint normal execution ep: %d", ep);
    return ep;
}


/****************************************************************************
 *
 * stream_create_iso_endpoints_and_cbuffers
 *
 */
bool stream_create_iso_endpoints_and_cbuffers(unsigned int hci_handle,
                                              sco_buf_desc *source_buf_desc,
                                              sco_buf_desc *sink_buf_desc,
                                              tCbuffer **source_cbuffer,
                                              tCbuffer **sink_cbuffer)
{
    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;

    ENDPOINT *source_ep, *sink_ep;
    source_ep = sink_ep = NULL;

    patch_fn_shared(stream_iso_interface);

    /* Clear contents of source and sink cbuffer pointer parameters as a
     * precaution to minimise the risk of a caller treating them as valid
     * pointers in the event of the function failing and returning FALSE.
     */
    *source_cbuffer = NULL;
    *sink_cbuffer = NULL;

    L3_DBG_MSG1("hydra stream_create_iso_endpoints_and_cbuffers hci handle: %d", key);

    /* Check that we don't already have a source endpoint for the
     * specified hci handle.
     */
    if (iso_get_endpoint(key, SOURCE) != NULL
            || iso_get_endpoint(key, SINK) != NULL)
    {
        /* Caller should not have called us for a second time without
         * deleting the existing buffers first.
         */
        panic(PANIC_AUDIO_SCO_BUFFERS_ALREADY_EXIST);
    }

    /* Check if the channel is active in the source direction */
    if (source_buf_desc->size > 0)
    {
        /* Create and initialise a source endpoint
         * ---------------------------------------
         */
        if ((source_ep = iso_create_endpoint(key, SOURCE)) == NULL)
        {
            return FALSE;
        }

        if (!iso_set_up_endpoint(source_ep, SOURCE, source_buf_desc))
        {
            /* we are failing so free up created source endpoint*/
            clean_up_endpoint(source_ep);
            return FALSE;
        }
        /* source buffer will have metadata, it will be enabled during
         * connection process
         */
        source_ep->state.iso.generate_metadata = TRUE;
        source_ep->state.iso.prev_write_offset =
                    cbuffer_get_write_mmu_offset(source_ep->state.iso.cbuffer);

        L2_DBG_MSG2("SCO_RX_GENERATE_METADATA, buf configured, buf=0x%x, prev_offset=%d",
                    (unsigned)(uintptr_t)source_ep->state.iso.cbuffer,
                    source_ep->state.iso.prev_write_offset);
    }

    /* Check if the channel is active in the sink direction */
    if (sink_buf_desc->size > 0)
    {
        /* Create and initialise a sink endpoint
         * -------------------------------------
         */
        if ((sink_ep = iso_create_endpoint(key, SINK)) == NULL)
        {
            /* we are failing so free up any resources
             */
            clean_up_iso(source_ep, sink_ep);
            return FALSE;
        }

        if (!iso_set_up_endpoint(sink_ep, SINK, sink_buf_desc))
        {
            /* we are failing so free up any resources
             */
            clean_up_iso(source_ep, sink_ep);
            return FALSE;
        }

    }

    /* Update incoming pointer parameters to give caller access to the
     * created source and sink cbuffer structures.
     */
    if (source_ep != NULL)
    {
        *source_cbuffer = source_ep->state.iso.cbuffer;
    }
    if(sink_ep != NULL)
    {
        *sink_cbuffer = sink_ep->state.iso.cbuffer;
    }

    /* Succeeded */
    return TRUE;
}

/****************************************************************************
 *
 * stream_delete_iso_endpoints_and_cbuffers
 *
 */
void stream_delete_iso_endpoints_and_cbuffers(unsigned int hci_handle)
{
    ENDPOINT *ep;
    tCbuffer *temp_buffer_ptr;

    /* The hci handle is the key (unique for the type and direction) */
    unsigned key = hci_handle;

    patch_fn_shared(stream_iso_interface);

    /* Get and close the source endpoint associated with the hci handle */
    if ((ep = iso_get_endpoint(key, SOURCE)) != NULL)
    {
        /* Remember the buffer that we need to free */
        temp_buffer_ptr = ep->state.sco.cbuffer;

        ep->can_be_closed = TRUE;
        ep->can_be_destroyed = TRUE;
        stream_close_endpoint(ep);

        /* Free up the buffer and associated data space */
        cbuffer_destroy(temp_buffer_ptr);
    }

    /* Get and close the sink endpoint associated with the hci handle */
    if ((ep = iso_get_endpoint(key, SINK)) != NULL)
    {
        /* Remember the buffer that we need to free */
        temp_buffer_ptr = ep->state.sco.cbuffer;

        ep->can_be_closed = TRUE;
        ep->can_be_destroyed = TRUE;
        stream_close_endpoint(ep);

        /* Free up the buffer and associated data space */
        cbuffer_destroy(temp_buffer_ptr);
    }
}

/**
 * stream_iso_reset_sco_metadata_buffer_offset
 * NOTE: this is likely to never be needed, as we hope to keep sco_framework
 * (sco_fw_c) possibly agnostic of the differences from SCO to ISO
 */
void stream_iso_reset_sco_metadata_buffer_offset(ENDPOINT *ep)
{
    patch_fn_shared(stream_iso_interface);
    stream_sco_reset_sco_metadata_buffer_offset(ep);
}

/****************************************************************************
Private Function Definitions
*/

/*
 * Set up a newly created endpoint and create/wrap mmu buffers
 */
static bool iso_set_up_endpoint(ENDPOINT * ep, ENDPOINT_DIRECTION dir, sco_buf_desc * buf_desc)
{
    unsigned remote_flags;
    unsigned flags_aux;
    unsigned flags, shift;

    patch_fn_shared(stream_iso_interface);

    if (dir == SOURCE)
    {
        remote_flags = BUF_DESC_WRAP_REMOTE_MMU_MOD_RD;
        flags_aux = BUF_DESC_MMU_BUFFER_AUX_WR;
    }
    else
    {
        remote_flags = BUF_DESC_WRAP_REMOTE_MMU_MOD_WR;
        flags_aux = BUF_DESC_MMU_BUFFER_AUX_RD;
    }

    ep->can_be_closed = FALSE;
    ep->can_be_destroyed = FALSE;
    /* ISO endpoints are always at the end of a chain */
    ep->is_real = TRUE;

    if (buf_desc->is_remote)
    {
        /* We're just going to wrap a remote buffer and handles with the
         * requirement that we can modify the read handle.
         */
        ep->state.iso.cbuffer = cbuffer_wrap_remote(
                                            remote_flags,
                                            buf_desc->remote_rd_handle,
                                            buf_desc->remote_wr_handle,
                                            buf_desc->size );
    }
    else
    {
#if defined(BAC32)
        flags = MMU_UNPACKED_16BIT_MASK;
        shift = 16;
#elif defined(BAC24)
        flags = 0;
        shift = 8;
#else
#error "ISO buffer flags and shift undefined for this BAC"
#endif
        /* We're hosting the buffer so create the source buffer and handles.
         * This includes a third auxiliary handle which is a write handle.
         */
        ep->state.iso.cbuffer = cbuffer_create_mmu_buffer(
                                          flags | flags_aux,
                                          buf_desc->size);
        if (dir == SOURCE)
        {
            cbuffer_set_write_shift(ep->state.iso.cbuffer, shift);
        }
        else
        {
            cbuffer_set_read_shift(ep->state.iso.cbuffer, shift);
        }
    }

    if (ep->state.iso.cbuffer == NULL)
    {
        return FALSE;
    }

    /* initialise measured rate */
    ep->state.iso.rate_measurement = 1<<STREAM_RATEMATCHING_FIX_POINT_SHIFT;
#ifdef INSTALL_SCO_EP_CLRM
    ep->state.iso.rm_enable_clrm_measurement = TRUE;
    ep->state.iso.rm_enable_clrm_trace = FALSE;
#endif
    return TRUE;
}

/*
 * Reset the toa packet offset procedure
 */
static inline void  iso_rx_reset_toa_packet_offset(endpoint_iso_state *iso)
{
    iso->packet_offset = 0;
    iso->packet_offset_counter = 0;
    iso->packet_offset_stable = FALSE;
}

/*
 * Cleanup endpoint and cbuffer
 */
static void clean_up_endpoint(ENDPOINT * ep)
{
    patch_fn_shared(stream_iso_interface);
    if (ep != NULL)
    {
        if (ep->state.iso.cbuffer != NULL)
        {
            /* Free up the buffer and associated data space */
            cbuffer_destroy(ep->state.iso.cbuffer);
        }
        ep->can_be_destroyed = TRUE;
        stream_destroy_endpoint(ep);
    }
}

/*
 * Cleanup source and sink endpoints and cbuffers if they exist
 */
static inline void clean_up_iso(ENDPOINT * source_ep, ENDPOINT * sink_ep)
{
    if (source_ep != NULL)
    {
        clean_up_endpoint(source_ep);
    }
    if (sink_ep != NULL)
    {
        clean_up_endpoint(sink_ep);
    }
}


#endif /* INSTALL_ISO_CHANNELS */
