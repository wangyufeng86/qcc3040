/* Copyright (c) 2010 - 2020 Qualcomm Technologies International, Ltd. */
/*  

FILE NAME
	sppc_service_handler.c        

DESCRIPTION
	Functions used for starting up and also stopping an SPP Server.

*/

#include "spps_private.h"

#include <sdp_parse.h>
#include <stdlib.h>

void sppServiceHandler(Task task, MessageId id, Message message);

const TaskData sppsServiceTask = { sppServiceHandler };

static Task sppsClientTask = 0;
static uint8 spps_rfcomm_channel;
static uint8* spps_registered_service_record=NULL;

/*****************************************************************************/

static void sendSppStartServiceCfm(spp_start_status status)
{
    if (sppsClientTask)
    {
        MAKE_SPP_MESSAGE(SPP_START_SERVICE_CFM);
        message->status = status;
        MessageSend(sppsClientTask, SPP_START_SERVICE_CFM, message);
    }
    else
    {
        SPP_DEBUG(("sppsClientTask is NULL!\n"));
    }
}

/*****************************************************************************/

static void sppHandleRfcommRegisterCfm(const CL_RFCOMM_REGISTER_CFM_T *cfm)
{
    
    if (!sppsClientTask)
    {
        SPP_DEBUG(("sppsClientTask is NULL!\n"));
    }

    if (cfm->status != success)
    {
        sendSppStartServiceCfm(spp_start_rfc_chan_fail);
        sppsClientTask = 0;
    }
    else
    {
        spps_rfcomm_channel = cfm->server_channel;
        sppsRegisterSdpServiceRecord((Task)&sppsServiceTask);
    }
}

/*****************************************************************************/

static void sppHandleSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    SPP_PRINT(("sppHandleSdpRegisterCfm %d", cfm->status));

    if (cfm->status == sds_status_pending)
    {
        return;
    }

    if (cfm->status == sds_status_success)
    {
        sendSppStartServiceCfm(spp_start_success);
        sppsStoreServiceHandle(cfm->service_handle);
    }
    else if (cfm->status != sds_status_pending)
    {
        sendSppStartServiceCfm(spp_start_sdp_reg_fail);
        sppsClientTask = 0;
    }
}

/*****************************************************************************/

static void sppHandleRfcommConnectInd( CL_RFCOMM_CONNECT_IND_T *ind )
{
    MAKE_SPP_MESSAGE(SPP_CONNECT_IND);

    if (!sppsClientTask)
    {
        SPP_DEBUG(("sppsClientTask is NULL!\n"));
    }

    message->addr = ind->bd_addr;
    message->server_channel = ind->server_channel;
    message->sink = ind->sink;
    MessageSend(sppsClientTask, SPP_CONNECT_IND, message);
}

/*****************************************************************************/

static void sendSppStopServiceCfm(Task theClientTask, spp_stop_status status)
{
    if (theClientTask)
    {
        MAKE_SPP_MESSAGE(SPP_STOP_SERVICE_CFM);
        message->status = status;
        MessageSend(theClientTask, SPP_STOP_SERVICE_CFM, message);
    }
    else
    {
        SPP_DEBUG(("theClientTask is NULL!\n"));
    }
}

/*****************************************************************************/

void sppsRegisterSdpServiceRecord(Task response_handler_task)
{
    spps_registered_service_record = PanicUnlessMalloc(spps_service_record_size());
    
    memcpy(spps_registered_service_record, spps_service_record(), spps_service_record_size());
    PanicFalse(SdpParseInsertRfcommServerChannel(spps_service_record_size(),
                                                 spps_registered_service_record,
                                                 spps_rfcomm_channel));
    ConnectionRegisterServiceRecord(response_handler_task,
                                    spps_service_record_size(),
                                    spps_registered_service_record);
}

/*****************************************************************************/

void sppsUnregisterSdpServiceRecord(void)
{
    ConnectionUnregisterServiceRecord((Task)&sppsServiceTask, sppsGetServiceHandle());
}


/*****************************************************************************/

static void sppHandleSdpUnregisterCfm(CL_SDP_UNREGISTER_CFM_T *cfm)
{
    if (cfm->status == sds_status_success)
    {
        /* Set the service handle to 0 now */
        sppsStoreServiceHandle(0);

        /* The record is freed before we get the CL_SDP_UNREGISTER_CFM */
        if (spps_registered_service_record)
        {
            spps_registered_service_record = NULL;
        }

        ConnectionRfcommDeallocateChannel((Task)&sppsServiceTask, spps_rfcomm_channel);
    }
    else if (cfm->status != sds_status_pending)
    {
        sendSppStopServiceCfm(sppsClientTask, spp_stop_sdp_unreg_fail);
    }
}

/*****************************************************************************/

static void sppHandleRfcommUnregisterCfm(CL_RFCOMM_UNREGISTER_CFM_T *cfm)
{
    if (cfm->status == success)
    {
        sendSppStopServiceCfm(sppsClientTask, spp_stop_success);
        sppsClientTask = 0;
    }
    else
    {
        sendSppStopServiceCfm(sppsClientTask, spp_stop_rfc_chan_fail);
    }
}


/*****************************************************************************/

void sppServiceHandler(Task task, MessageId id, Message message)
{
    UNUSED(task);

    if (sppsClientTask)
    {
        switch(id)
        {
        case CL_RFCOMM_REGISTER_CFM:  
            sppHandleRfcommRegisterCfm((CL_RFCOMM_REGISTER_CFM_T *)message);
            break;

        case CL_RFCOMM_UNREGISTER_CFM:
            sppHandleRfcommUnregisterCfm((CL_RFCOMM_UNREGISTER_CFM_T *)message);
            break;
            
        case CL_SDP_REGISTER_CFM: 
            sppHandleSdpRegisterCfm((CL_SDP_REGISTER_CFM_T *)message);
            break;

        case CL_SDP_UNREGISTER_CFM:
            sppHandleSdpUnregisterCfm((CL_SDP_UNREGISTER_CFM_T *)message);
            break;

        case CL_RFCOMM_CONNECT_IND:
            sppHandleRfcommConnectInd( (CL_RFCOMM_CONNECT_IND_T *)message);
            break;
            
        default:
            /* Received an unexpected message */
            SPP_DEBUG(("sppServiceHandler - unexpected msg type 0x%x\n", id));
            break;
        }
    }
    else
    {
        SPP_DEBUG(("sppsClientTask is NULL!\n"));
    }
}

    
/*****************************************************************************/

void SppStartService(Task theAppTask )
{
    /* Is there already an SPP service initiated?  */
    if (sppsClientTask)
    {
        sendSppStartServiceCfm(spp_start_already_started);
    }
    else
    {
        sppsClientTask = theAppTask;
        ConnectionRfcommAllocateChannel(
               (Task)&sppsServiceTask, 
               SPP_DEFAULT_CHANNEL
               );
    }
}

/*****************************************************************************/

void SppStopService(Task theAppTask)
{
    /* Is there an SPP service to stop? */
    if (!sppsClientTask)
    {
        sendSppStopServiceCfm(theAppTask, spp_stop_not_started);
    }
    else if (sppsClientTask != theAppTask)
    {
        sendSppStopServiceCfm(theAppTask, spp_stop_invalid_app_task);
    }
    else
    {
        sppsUnregisterSdpServiceRecord();
    }
}

