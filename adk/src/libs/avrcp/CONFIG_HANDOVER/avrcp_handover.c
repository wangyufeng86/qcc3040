/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.


FILE NAME
    avrcp_handover.c

DESCRIPTION
    Implements AVRCP handover logic (Veto, Marshals/Unmarshals, Handover, etc).

NOTES
    See handover_if.h for further interface description 

    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER    
*/


#include "avrcp_marshal_desc.h"
#include "avrcp_private.h"
#include "avrcp_init.h"
#include "avrcp_profile_handler.h"
#include "avrcp_handover_policy.h"

#include "marshal.h"
#include <sink.h>
#include <source.h>
#include <stream.h>
#include <panic.h>
#include <stdlib.h>


static bool avrcpVeto( void );
static bool avrcpMarshal(const tp_bdaddr *tp_bd_addr,
                         uint8 *buf,
                         uint16 length,
                         uint16 *written);
static bool avrcpUnmarshal(const tp_bdaddr *tp_bd_addr,
                           const uint8 *buf,
                           uint16 length,
                           uint16 *consumed);
static void avrcpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole);
static void avrcpHandoverComplete( const bool newRole );
static void avrcpHandoverAbort(void);


const handover_interface avrcp_handover =
{
    avrcpVeto,
    avrcpMarshal,
    avrcpUnmarshal,
    avrcpHandoverCommit,
    avrcpHandoverComplete,
    avrcpHandoverAbort
};

typedef struct
{
    unmarshaller_t unmarshaller;
    AVRCP *avrcp;
    bdaddr bd_addr;
} avrcp_marshal_inst_t;

avrcp_marshal_inst_t *avrcp_marshal_inst = NULL;


/****************************************************************************
NAME    
    browsingSupported

DESCRIPTION
    Finds out whether browsing is supported or not 

RETURNS
    bool TRUE if browsing supported
*/
static bool browsingSupported(void)
{
    AvrcpDeviceTask *avrcp_device_task = avrcpGetDeviceTask();

    return (isAvrcpBrowsingEnabled(avrcp_device_task) ||
            isAvrcpTargetCat1Supported(avrcp_device_task) ||
            isAvrcpTargetCat3Supported(avrcp_device_task));
}

/****************************************************************************
NAME    
    stitchAvrcp

DESCRIPTION
    Stitch an unmarshalled AVRCP connection instance 

RETURNS
    void
*/
static void stitchAvrcp(AVRCP *unmarshalled_avrcp, const bdaddr *bd_addr)
{
    uint16 cid;

    unmarshalled_avrcp->task.handler = avrcpProfileHandler;
    unmarshalled_avrcp->dataFreeTask.cleanUpTask.handler = avrcpDataCleanUp;

    if (browsingSupported())
    {
        AVRCP_AVBP_INIT *avrcp_avbp = (AVRCP_AVBP_INIT *) unmarshalled_avrcp;

        avrcp_avbp->avbp.task.handler = avbpProfileHandler;
        avrcp_avbp->avrcp.avbp_task = &avrcp_avbp->avbp.task;
        avrcp_avbp->avbp.avrcp_task = &avrcp_avbp->avrcp.task;
    }

    /* Initialize the connection context for the relevant connection id */
    convertL2capCidToSink(&unmarshalled_avrcp->sink);
    cid = SinkGetL2capCid(unmarshalled_avrcp->sink);
    PanicZero(cid);   /* Invalid Connection ID */
    VmOverrideL2capConnContext(cid, (conn_context_t)&unmarshalled_avrcp->task);
    /* Stitch the sink and the task */
    MessageStreamTaskFromSink(unmarshalled_avrcp->sink, &unmarshalled_avrcp->task);
    /* Configure sink messages */
    SinkConfigure(unmarshalled_avrcp->sink, VM_SINK_MESSAGES, VM_MESSAGES_ALL);

    /* Add to the connection list */
    avrcpAddTaskToList(unmarshalled_avrcp, bd_addr, unmarshalled_avrcp->bitfields.connection_incoming);
}

/****************************************************************************
NAME    
    avrcpHandoverAbort

DESCRIPTION
    Abort the AVRCP library Handover process, free any memory
    associated with the marshalling process.

RETURNS
    void
*/
static void avrcpHandoverAbort(void)
{
    if (avrcp_marshal_inst)
    {
        UnmarshalDestroy(avrcp_marshal_inst->unmarshaller, TRUE);
        avrcp_marshal_inst->unmarshaller = NULL;
        free(avrcp_marshal_inst);
        avrcp_marshal_inst = NULL;
    }
}

/****************************************************************************
NAME    
    avrcpMarshal

DESCRIPTION
    Marshal the data associated with AVRCP connections

RETURNS
    bool TRUE if AVRCP module marshalling complete, otherwise FALSE
*/
static bool avrcpMarshal(const tp_bdaddr *tp_bd_addr,
                         uint8 *buf,
                         uint16 length,
                         uint16 *written)
{
    AVRCP *avrcp = NULL;
    if (AvrcpGetInstanceFromBdaddr(&tp_bd_addr->taddr.addr, &avrcp))
    {
        bool marshalled;
        marshaller_t marshaller = MarshalInit(mtd_avrcp, AVRCP_MARSHAL_OBJ_TYPE_COUNT);
        PanicNull(marshaller);

        MarshalSetBuffer(marshaller, (void *) buf, length);

        marshalled = Marshal(marshaller, avrcp,
                             browsingSupported() ? MARSHAL_TYPE(AVRCP_AVBP_INIT) :
                                                   MARSHAL_TYPE(AVRCP));

        *written = marshalled ? MarshalProduced(marshaller) : 0;

        MarshalDestroy(marshaller, FALSE);
        return marshalled;
    }
    else
    {
        *written = 0;
        return TRUE;
    }
}

/****************************************************************************
NAME    
    avrcpUnmarshal

DESCRIPTION
    Unmarshal the data associated with AVRCP connections

RETURNS
    bool TRUE if AVRCP unmarshalling complete, otherwise FALSE
*/
static bool avrcpUnmarshal(const tp_bdaddr *tp_bd_addr,
                           const uint8 *buf,
                           uint16 length,
                           uint16 *consumed)
{
    marshal_type_t unmarshalled_type;

    if (!avrcp_marshal_inst)
    {
        /* Initiating unmarshalling, initialize the instance */
        avrcp_marshal_inst = PanicUnlessNew(avrcp_marshal_inst_t);
        avrcp_marshal_inst->unmarshaller = UnmarshalInit(mtd_avrcp, AVRCP_MARSHAL_OBJ_TYPE_COUNT);
        PanicNull(avrcp_marshal_inst->unmarshaller);
        avrcp_marshal_inst->bd_addr = tp_bd_addr->taddr.addr;
    }
    else
    {
        /* Resuming the unmarshalling */
    }

    UnmarshalSetBuffer(avrcp_marshal_inst->unmarshaller,
                       (void *) buf,
                       length);

    if (Unmarshal(avrcp_marshal_inst->unmarshaller,
                  (void**)&avrcp_marshal_inst->avrcp,
                  &unmarshalled_type))
    {
        PanicFalse(unmarshalled_type == MARSHAL_TYPE(AVRCP) ||
                   unmarshalled_type == MARSHAL_TYPE(AVRCP_AVBP_INIT));

        *consumed = UnmarshalConsumed(avrcp_marshal_inst->unmarshaller);

        /* Only expecting one object, so unmarshalling is complete */
        return TRUE;

    }
    else
    {
        *consumed = UnmarshalConsumed(avrcp_marshal_inst->unmarshaller);
        return FALSE;
    }
}

/****************************************************************************
NAME    
    avrcpHandoverCommit

DESCRIPTION
    The AVRCP  library performs time-critical actions to commit to the specified
    new role (primary or secondary)

RETURNS
    void
*/
static void avrcpHandoverCommit(const tp_bdaddr *tp_bd_addr, const bool newRole)
{
    UNUSED(tp_bd_addr);

    if (newRole && avrcp_marshal_inst)
    {
        Source src;
        /* Stitch unmarshalled AVRCP connection instance */
        stitchAvrcp(avrcp_marshal_inst->avrcp, &avrcp_marshal_inst->bd_addr);
        
        /* Set the handover policy */
        src = StreamSourceFromSink(avrcp_marshal_inst->avrcp->sink);
        PanicFalse(avrcpSourceConfigureHandoverPolicy(src, SOURCE_HANDOVER_ALLOW_WITHOUT_DATA));
    }
}

/****************************************************************************
NAME    
    avrcpHandoverComplete

DESCRIPTION
    The AVRCP  library performs pending actions and completes transition to 
    specified new role (primary or secondary)

RETURNS
    void
*/
static void avrcpHandoverComplete( const bool newRole )
{
    if (newRole && avrcp_marshal_inst)
    {
        UnmarshalDestroy(avrcp_marshal_inst->unmarshaller, FALSE);
        avrcp_marshal_inst->unmarshaller = NULL;
        free(avrcp_marshal_inst);
        avrcp_marshal_inst = NULL;
    }
}

/****************************************************************************
NAME    
    avrcpVeto

DESCRIPTION
    Veto check for AVRCP library

    Prior to handover commencing this function is called and
    the libraries internal state is checked to determine if the
    handover should proceed.

RETURNS
    bool TRUE if the AVRCP Library wishes to veto the handover attempt.
*/
static bool avrcpVeto( void )
{
    avrcpList *list = avrcpListHead;
    AvrcpDeviceTask *avrcp_device_task = avrcpGetDeviceTask();
    avrcp_device_role device_role = avrcp_device_task->bitfields.device_type;

    /* If AVRCP library initialization is not complete or AvrcpInit has not been
     * called the set device role will not be set */
    if (device_role != avrcp_target &&
        device_role != avrcp_controller &&
        device_role != avrcp_target_and_controller  )
    {
        return TRUE;
    }

    /* Check message queue status */
    if(MessagesPendingForTask(avrcp_device_task->app_task, NULL) != 0)
    {
        return TRUE;
    }

    /* Check the AVRCP tasks */
    while (list != NULL)
    {
        AVRCP *avrcp = list->avrcp;

        /* Check whether there is a connection in progress. */
        if(avrcp->bitfields.state == avrcpConnecting)
        {
            return TRUE;
        }

        list = list->next;
    }

    return FALSE;
}



