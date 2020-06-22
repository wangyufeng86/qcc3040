/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    hfp_handover.c

DESCRIPTION
    Implements HFP handover logic (Veto, Marshals/Unmarshals, Handover, etc).
 
NOTES
    See handover_if.h for further interface description
    
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER    
 */

#include "hfp_marshal_desc.h"
#include "hfp_private.h"
#include "hfp_init.h"
#include "hfp_link_manager.h"
#include "hfp_handover_policy.h"

#include "marshal.h"
#include "bdaddr.h"
#include <panic.h>
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>


#define RFC_INVALID_SERV_CHANNEL   0x00

static bool hfpVeto(void);

static bool hfpMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written);

static bool hfpUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed);

static void hfpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);

static void hfpHandoverComplete( const bool newRole );

static void hfpHandoverAbort( void );

extern const handover_interface hfp_handover_if =  {
        &hfpVeto,
        &hfpMarshal,
        &hfpUnmarshal,
        &hfpHandoverCommit,
        &hfpHandoverComplete,
        &hfpHandoverAbort};

typedef struct
{
    unmarshaller_t unmarshaller;
    hfp_marshalled_obj *data;
    tp_bdaddr bd_addr;
} hfp_marshal_instance_t;

static hfp_marshal_instance_t *hfp_marshal_inst = NULL;


/****************************************************************************
NAME    
    getRemoteServerChannel

DESCRIPTION
    Returns remote server channel of the specified link. If it does not exist
    or is idle or disabled then it returns RFC_INVALID_SERV_CHANNEL

RETURNS
    uint8 Remote server channel
*/
static uint8 getRemoteServerChannel(hfp_link_data *link)
{
    uint8 channel = RFC_INVALID_SERV_CHANNEL;
                            
    if (link->bitfields.ag_slc_state == hfp_slc_idle ||
        link->bitfields.ag_slc_state == hfp_slc_disabled)
    {
        /* Not connected */
    }
    else
    {
        /* Connected */
        channel = SinkGetRfcommServerChannel(link->identifier.sink);
    }

    return channel;
}

/****************************************************************************
NAME
    hfpHandoverAbort

RETURNS
    void
*/
static void hfpHandoverAbort(void)
{
    if (hfp_marshal_inst)
    {
        UnmarshalDestroy(hfp_marshal_inst->unmarshaller, TRUE);
        hfp_marshal_inst->unmarshaller = NULL;
        free(hfp_marshal_inst);
        hfp_marshal_inst = NULL;
    }
}

/****************************************************************************
NAME    
    hfpMarshal

DESCRIPTION
    Marshal the data associated with HFP connections

RETURNS
    bool TRUE if HFP module marshalling complete, otherwise FALSE
*/
static bool hfpMarshal(const tp_bdaddr *tp_bd_addr,
                       uint8 *buf,
                       uint16 length,
                       uint16 *written)
{
    bool validLink = TRUE;
    uint8 channel = RFC_INVALID_SERV_CHANNEL;

    /* Check we have a valid link */
    hfp_link_data *link = hfpGetLinkFromBdaddr(&tp_bd_addr->taddr.addr);

    if(link)
    {
        /* check for a valid RFC channel */
        channel = getRemoteServerChannel(link);
        if (channel == RFC_INVALID_SERV_CHANNEL)
        {
            validLink = FALSE;
        }
    }
    else
    {
        validLink = FALSE;
    }

    if(validLink)
    {
        bool marshalled;
        marshaller_t marshaller = MarshalInit(mtd_hfp, HFP_MARSHAL_OBJ_TYPE_COUNT);
        hfp_marshalled_obj obj;
        obj.link = link;
        obj.channel = channel;
        obj.bitfields = theHfp->bitfields;

        MarshalSetBuffer(marshaller, (void *) buf, length);

        marshalled = Marshal(marshaller, &obj, MARSHAL_TYPE(hfp_marshalled_obj));

        *written = marshalled ? MarshalProduced(marshaller) : 0;

        MarshalDestroy(marshaller, FALSE);
        return marshalled;
    }
    else
    {
        /* Link not valid, nothing to marshal */
        *written = 0;
        return TRUE;
    }
}

/****************************************************************************
NAME
    hfpUnmarshal

DESCRIPTION
    Unmarshal the data associated with the HFP connections

RETURNS
    bool TRUE if HFP unmarshalling complete, otherwise FALSE
*/
static bool hfpUnmarshal(const tp_bdaddr *tp_bd_addr,
                         const uint8 *buf,
                         uint16 length,
                         uint16 *consumed)
{
    marshal_type_t type;
    bool unmarshalled = TRUE;

    if (!hfp_marshal_inst)
    {
        hfp_marshal_inst = PanicUnlessNew(hfp_marshal_instance_t);
        hfp_marshal_inst->unmarshaller = UnmarshalInit(mtd_hfp, HFP_MARSHAL_OBJ_TYPE_COUNT);
        hfp_marshal_inst->bd_addr = *tp_bd_addr;
    }

    UnmarshalSetBuffer(hfp_marshal_inst->unmarshaller, (void *) buf, length);

    unmarshalled = Unmarshal(hfp_marshal_inst->unmarshaller,
                             (void **) &hfp_marshal_inst->data, &type);
    if (unmarshalled)
    {
        PanicFalse(type == MARSHAL_TYPE(hfp_marshalled_obj));
    }
    *consumed = UnmarshalConsumed(hfp_marshal_inst->unmarshaller);
    return unmarshalled;
}


/****************************************************************************
NAME    
    hfpVeto

DESCRIPTION
    Veto check for HFP library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the HFP Library wishes to veto the handover attempt.
*/
bool hfpVeto( void )
{
    hfp_link_data* link;

    /* Check the HFP library is initialized */
    if ( !theHfp )
    {
        return TRUE;
    }

    /* check multipoint is not active */
    if (hfpGetLinkFromPriority(hfp_secondary_link) != NULL)
    {
        return TRUE;
    }

    /* Check if an AT command response is pending from the AG.  */
    link = hfpGetLinkFromPriority(hfp_primary_link);
    if(link && link->bitfields.at_cmd_resp_pending != hfpNoCmdPending)
    {
        return TRUE;
    }

    /* Check message queue status */
    if(MessagesPendingForTask(&theHfp->task, NULL) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

/****************************************************************************
NAME    
    hfpHandoverCommit

DESCRIPTION
    The HFP library performs time-critical actions to commit to the specified
    new role (primary or secondary)

RETURNS
    void
*/
static void hfpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    if (newRole && hfp_marshal_inst)
    {
        UNUSED(tp_bd_addr);
        Source src;
        uint16 conn_id;
        hfp_link_data *link = hfpGetIdleLink();
        hfp_marshalled_obj *objp = hfp_marshal_inst->data;

        /* Commit must be called after unmarshalling */
        PanicNull(hfp_marshal_inst->data);
        PanicNull(link);

        theHfp->bitfields = objp->bitfields;

        objp->link->identifier.bd_addr = hfp_marshal_inst->bd_addr.taddr.addr;
        objp->link->identifier.sink = StreamRfcommSinkFromServerChannel(&hfp_marshal_inst->bd_addr,
                                                                        objp->channel);
        *link = *objp->link;
        /* The sink should always be valid, do the state copy first, so its easier
        to debug by looking at appHfp state */
        PanicNull(link->identifier.sink);

        conn_id = SinkGetRfcommConnId(link->identifier.sink);
        PanicFalse(VmOverrideRfcommConnContext(conn_id, (conn_context_t)&theHfp->task));
        
        /* Set the handover policy on the stream */
        src = StreamSourceFromSink(link->identifier.sink);
        PanicFalse(hfpSourceConfigureHandoverPolicy(src, SOURCE_HANDOVER_ALLOW_WITHOUT_DATA));
        
        /* Stitch the RFCOMM sink and the task */
        MessageStreamTaskFromSink(link->identifier.sink, &theHfp->task);
        /* Configure RFCOMM sink messages */
        SinkConfigure(link->identifier.sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL);
    }
}

/****************************************************************************
NAME    
    hfpHandoverComplete

DESCRIPTION
    The HFP library performs pending actions and completes transition to 
    specified new role (primary or secondary)

RETURNS
    void
*/
static void hfpHandoverComplete( const bool newRole )
{
    if (newRole && hfp_marshal_inst)
    {
        PanicNull(hfp_marshal_inst->data);
        UnmarshalDestroy(hfp_marshal_inst->unmarshaller, TRUE);
        hfp_marshal_inst->unmarshaller = NULL;
        free(hfp_marshal_inst);
        hfp_marshal_inst = NULL;
    }
}

