/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
*/

#ifdef INCLUDE_QCOM_CON_MANAGER

#include <message.h>
#include <task_list.h>
#include <panic.h>
#include <logging.h>
#include <app/bluestack/vsdm_prim.h>
#include "init.h"
#include "connection_manager.h"

#include "qualcomm_connection_manager.h"
#include "qualcomm_connection_manager_private.h"

/* Static functions */

static bool qcomConManagerCheckIfQhsIsSupported(uint8 * qlmp_supp_features)
{
    PanicNull(qlmp_supp_features);

    /* Check if qhs is supported or not */
    if ((qlmp_supp_features[0] & QHS_ENABLE_BIT_MASKS_OCTET0) != 0 ||
        (qlmp_supp_features[1] & QHS_ENABLE_BIT_MASK_OCTET1) != 0)
    {
        return TRUE;
    }
    return FALSE;
}

static void qcomConManagerSendQhsConnectedIndicationToClients(const VSDM_QCM_PHY_CHANGE_IND_T * ind)
{
    qcomConManagerTaskData *sp = QcomConManagerGet();
    MESSAGE_MAKE(qhs_connected_ind, QCOM_CON_MANAGER_QHS_CONNECTED_T);
    memcpy(&qhs_connected_ind->bd_addr,&ind->bd_addr,sizeof(qhs_connected_ind->bd_addr));
    TaskList_MessageSend(&sp->client_tasks, QCOM_CON_MANAGER_QHS_CONNECTED, qhs_connected_ind);
}

static void qcomConManagerHandleVsdmQlmPhyChangeInd(const VSDM_QCM_PHY_CHANGE_IND_T * ind)
{
    DEBUG_LOG("qcomConManagerHandleVsdmQlmPhyChangeInd  qhs status=%d bdaddr lap=%06lX",
              ind->status,ind->bd_addr.lap);

    if(ind->status == HCI_SUCCESS)
    {
        ConManagerSetQhsConnectStatus((const bdaddr*)&ind->bd_addr,TRUE);
        qcomConManagerSendQhsConnectedIndicationToClients(ind);
    }
}

static void qcomConManagerHandleVsdmRemoteQlmSuppFeaturesCfm(const VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T *cfm)
{
    /* Check if qhs is supported or not */
    if ((cfm->status == HCI_SUCCESS) && qcomConManagerCheckIfQhsIsSupported(cfm->qlmp_supp_features))
    {
        /* set qhs support status in connection manager and wait now for QLM_PHY_IND*/
        ConManagerSetQhsSupportStatus((const bdaddr*)&cfm->bd_addr,TRUE);
    }
    else
    {
        DEBUG_LOG("qcomConManagerHandleVsdmRemoteQlmSuppFeaturesCfm qhs not supported "
                  "status=%X bdaddr lap=%06lX",cfm->status,cfm->bd_addr.lap);
       /* set qhs not supported status in connection manager*/
        ConManagerSetQhsSupportStatus((const bdaddr*)&cfm->bd_addr,FALSE);
    }
}

static void qcomConManagerSendReadRemoteQlmSuppFeaturesReq(const VSDM_QLM_CONNECTION_COMPLETE_IND_T *ind)
{
    /*send VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ  to find if qhs is supported or not in remote device*/
    MAKE_VSDM_PRIM_T(VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ);
    prim->phandle = 0;
    prim->handle= ind->handle;
    memcpy(&prim->bd_addr,&ind->bd_addr,sizeof(prim->bd_addr));
    VmSendVsdmPrim(prim);
}

static void qcomConManagerHandleVsdmQlmConnectionCompleteInd(const VSDM_QLM_CONNECTION_COMPLETE_IND_T *ind)
{
    DEBUG_LOG("qcomConManagerHandleVsdmQlmConnectionCompleteInd bdaddr status=%x lap=%06lX",
               ind->status,ind->bd_addr.lap);

    if(ind->status == HCI_SUCCESS)
    {
        qcomConManagerSendReadRemoteQlmSuppFeaturesReq(ind);
        ConManagerSetQlmpConnectStatus((const bdaddr*)&ind->bd_addr,TRUE);
    }
    else
    {
        /* Qlm connection failed. Record status in connection manager and carry on non qhs connection*/
        ConManagerSetQlmpConnectStatus((const bdaddr*)&ind->bd_addr,FALSE);
    }
    
}

static void qcomConManagerHandleVsdmScHostSuppOverrideCfm(const VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T *cfm)
{
    PanicFalse(cfm->status == HCI_SUCCESS);
}

static void qcomConManagerSendScHostSuppOverrideReq(void)
{
    uint8 count =0;

    MAKE_VSDM_PRIM_T(VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ);
    prim->phandle = 0;
    prim->num_compIDs = count;

    while(vendor_info[count].comp_id != 0)
    {
        prim->compID[count] = vendor_info[count].comp_id;
        prim->min_lmpVersion[count] = vendor_info[count].min_lmp_version;
        prim->min_lmpSubVersion[count] = vendor_info[count].min_lmp_sub_version;
        prim->num_compIDs = ++count;

        /* Check if input number of comp ids are not exceeding
           maximum number of comp ids supported by vs prims */
        if(prim->num_compIDs == VSDM_MAX_NO_OF_COMPIDS)
        {
            Panic();
        }
    }

    if(prim->num_compIDs != 0)
    {
        VmSendVsdmPrim(prim);
    }

    /* Send init confirmation to init module */
    MessageSend(Init_GetInitTask(), QCOM_CON_MANAGER_INIT_CFM, NULL);
}

static void qcomConManagerHandleVsdmLocalQlmSuppFeaturesCfm(const VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T *cfm)
{
    PanicFalse(cfm->status == HCI_SUCCESS);

    /* Check if qhs is supported or not */
    if (qcomConManagerCheckIfQhsIsSupported(cfm->qlmp_supp_features))
    {
        /* Send SC host override req as local device supports qhs*/
        qcomConManagerSendScHostSuppOverrideReq();
    }
    else
    {
        DEBUG_LOG("qcomConManagerHandleVsdmLocalQlmSuppFeaturesCfm: Qhs not supported");
        /* Inform application that qcom con manager init competed */
        MessageSend(Init_GetInitTask(), QCOM_CON_MANAGER_INIT_CFM, NULL);
    }
}


static void qcomConManagerSendReadLocalQlmSuppFeaturesReq(void)
{
    /*send VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ  to find if qhs is supported or not*/
    MAKE_VSDM_PRIM_T(VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ);
    prim->phandle = 0;
    VmSendVsdmPrim(prim);
}

static void qcomConManagerHandleVsdmRegisterCfm(const VSDM_REGISTER_CFM_T *cfm)
{
    PanicFalse(cfm->result == VSDM_RESULT_SUCCESS);
    /*Check if local device supports qhs or not and proceed to create qhs with remote
      handset if local supports qhs */
    qcomConManagerSendReadLocalQlmSuppFeaturesReq();
}

static void qcomConManagerRegisterReq(void)
{
    MAKE_VSDM_PRIM_T(VSDM_REGISTER_REQ);
    prim->phandle = 0;
    VmSendVsdmPrim(prim);
}

static void qcomConManagerHandleBluestackVsdmPrim(const VSDM_UPRIM_T *vsprim)
{
    switch (vsprim->type)
    {
    case VSDM_REGISTER_CFM:
        qcomConManagerHandleVsdmRegisterCfm((const VSDM_REGISTER_CFM_T *)vsprim);
        break;
    case VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM:
        qcomConManagerHandleVsdmLocalQlmSuppFeaturesCfm((VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T *)vsprim);
        break;
    case VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM:
        qcomConManagerHandleVsdmScHostSuppOverrideCfm((const VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T *)vsprim);
        break;
     case VSDM_QLM_CONNECTION_COMPLETE_IND:
        qcomConManagerHandleVsdmQlmConnectionCompleteInd((const VSDM_QLM_CONNECTION_COMPLETE_IND_T *)vsprim);
        break;
    case VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM:
       qcomConManagerHandleVsdmRemoteQlmSuppFeaturesCfm((VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T *)vsprim);
       break;
    case VSDM_QCM_PHY_CHANGE_IND:
        qcomConManagerHandleVsdmQlmPhyChangeInd((const VSDM_QCM_PHY_CHANGE_IND_T *)vsprim);
        break;
    default:
        DEBUG_LOG("qcomConManagerHandleBluestackVsdmPrim : unhandled vsprim type %x", vsprim->type);
        break;
    }
}

static void qcomConManagerHandleMessage(Task task, MessageId id, Message message)
{
   UNUSED(task);

   switch (id)
   {
     /* VSDM prims from firmware */
    case MESSAGE_BLUESTACK_VSDM_PRIM:
        qcomConManagerHandleBluestackVsdmPrim((const VSDM_UPRIM_T *)message);
        break;
    default:
        break;
    }
}
/* Global functions */

bool QcomConManagerInit(Task task)
{
    UNUSED(task);

    DEBUG_LOG("QcomConManagerInit");
    memset(&qcom_con_manager, 0, sizeof(qcom_con_manager));
    qcom_con_manager.task.handler = qcomConManagerHandleMessage;
    TaskList_Initialise(&qcom_con_manager.client_tasks);

    /* Register with the firmware to receive MESSAGE_BLUESTACK_VSDM_PRIM messages */
    MessageVsdmTask(QcomConManagerGetTask());

    /* Register with the VSDM service */
    qcomConManagerRegisterReq();

    return TRUE;
}

void QcomConManagerRegisterClient(Task client_task)
{
    DEBUG_LOG("QcomConManagerRegisterClient");
    TaskList_AddTask(&qcom_con_manager.client_tasks, client_task);
}

void QcomConManagerUnRegisterClient(Task client_task)
{
    TaskList_RemoveTask(&qcom_con_manager.client_tasks, client_task);
}

#endif /* INCLUDE_QCOM_CON_MANAGER */
