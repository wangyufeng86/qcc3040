/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Functions to send & process MDM prims to / from the firmware.
*/


#ifdef INCLUDE_MIRRORING

#include <bdaddr.h>
#include <sink.h>
#include <stream.h>

#include "bt_device.h"

#include "mirror_profile_private.h"
#include "mirror_profile_mdm_prim.h"
#include "mirror_profile_audio_source.h"
#include "handover_profile.h"
#include "kymera.h"
/*! Construct a MDM prim of the given type */
#define MAKE_MDM_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;


/*
    Functions to send MDM prims to firmware.
*/

void MirrorProfile_MirrorRegisterReq(void)
{
    MAKE_MDM_PRIM_T(MDM_REGISTER_REQ);
    prim->phandle = 0;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorConnectReq(link_type_t type)
{
    bdaddr bd_addr;

    MAKE_MDM_PRIM_T(MDM_LINK_CREATE_REQ);
    prim->phandle = 0;

    MIRROR_LOG("MirrorProfile_MirrorConnectReq type 0x%x", type);

    appDeviceGetHandsetBdAddr(&bd_addr);
    MIRROR_LOG("MirrorProfile_MirrorConnectReq handset addr {%04x %02x %06lx}",
                bd_addr.nap, bd_addr.uap, bd_addr.lap);
    BdaddrConvertVmToBluestack(&prim->mirror_bd_addr.addrt.addr, &bd_addr);
    prim->mirror_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->mirror_bd_addr.tp_type = BREDR_ACL;

    appDeviceGetSecondaryBdAddr(&bd_addr);
    MIRROR_LOG("MirrorProfile_MirrorConnectReq peer addr {%04x %02x %06lx}",
                bd_addr.nap, bd_addr.uap, bd_addr.lap);
    BdaddrConvertVmToBluestack(&prim->secondary_bd_addr.addrt.addr, &bd_addr);
    prim->secondary_bd_addr.addrt.type = TBDADDR_PUBLIC;
    prim->secondary_bd_addr.tp_type = BREDR_ACL;

    prim->link_type = type;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorDisconnectReq(hci_connection_handle_t conn_handle,
                                              hci_reason_t reason)
{
    MAKE_MDM_PRIM_T(MDM_LINK_DISCONNECT_REQ);

    MIRROR_LOG("mirrorProfile_MirrorDisconnectReq handle 0x%x reason 0x%x", conn_handle, reason);

    prim->phandle = 0;
    prim->conn_handle = conn_handle;
    prim->reason = reason;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capConnectReq(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_CREATE_REQ);

    MIRROR_LOG("MirrorProfile_MirrorL2capConnectReq handle 0x%x cid 0x%x", conn_handle, cid);

    prim->phandle = 0;
    prim->flags = 0;
    prim->connection_handle = conn_handle;
    prim->cid = cid;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capConnectRsp(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid)
{
    static const uint16 l2cap_conftab[] =
    {
        /* Configuration Table must start with a separator. */
        L2CAP_AUTOPT_SEPARATOR,
        /* Local MTU exact value (incoming). */
        L2CAP_AUTOPT_MTU_IN, 0x037F,
        /* Configuration Table must end with a terminator. */
        L2CAP_AUTOPT_TERMINATOR
    };

    MAKE_MDM_PRIM_T(MDM_L2CAP_CREATE_RSP);

    MIRROR_LOG("MirrorProfile_MirrorL2capConnectRsp handle 0x%x cid 0x%x", conn_handle, cid);

    prim->phandle = 0;
    prim->connection_handle = conn_handle;
    prim->cid = cid;
    prim->conftab_length = CONFTAB_LEN(l2cap_conftab);
    prim->conftab = (uint16*)l2cap_conftab;
    prim->response = L2CA_CONNECT_SUCCESS;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capDisconnectReq(l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_DISCONNECT_REQ);

    MIRROR_LOG("MirrorProfile_MirrorL2capDisconnectReq cid 0x%x", cid);

    prim->phandle = 0;
    prim->cid = cid;
    VmSendMdmPrim(prim);
}

void MirrorProfile_MirrorL2capDisconnectRsp(l2ca_cid_t cid)
{
    MAKE_MDM_PRIM_T(MDM_L2CAP_DISCONNECT_RSP);

    MIRROR_LOG("MirrorProfile_MirrorL2capDisconnectRsp cid 0x%x", cid);

    prim->cid = cid;
    VmSendMdmPrim(prim);
}


/*
    Functions to handle MDM prims sent by firmware.
*/

/*! \brief Handle MDM_REGISTER_CFM

    This is common to both Primary & Secondary
*/
static void mirrorProfile_HandleMdmRegisterCfm(const MDM_REGISTER_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmRegisterCfm result 0x%x", cfm->result);

    if (cfm->result == MDM_RESULT_SUCCESS)
    {
        mirror_profile_task_data_t *sp = MirrorProfile_Get();

        /* Send init confirmation to init module */
        MessageSend(sp->init_task, MIRROR_PROFILE_INIT_CFM, NULL);
    }
    else
    {
        Panic();
    }
}

/*! \brief Handle MDM_ACL_LINK_CREATE_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror ACL connection.
*/
static void mirrorProfile_HandleMdmAclLinkCreateCfm(const MDM_ACL_LINK_CREATE_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateCfm status 0x%x handle 0x%x role 0x%x",
                    cfm->status, cfm->connection_handle, cfm->role);

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateCfm buddy_addr {%04x %02x %06lx}",
                cfm->buddy_bd_addr.addrt.addr.nap,
                cfm->buddy_bd_addr.addrt.addr.uap,
                cfm->buddy_bd_addr.addrt.addr.lap);

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateCfm mirror_addr {%04x %02x %06lx}",
                cfm->mirror_bd_addr.addrt.addr.nap,
                cfm->mirror_bd_addr.addrt.addr.uap,
                cfm->mirror_bd_addr.addrt.addr.lap);

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
        if (cfm->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_PRIMARY == cfm->role);

            sp->acl.conn_handle = cfm->connection_handle;
            BdaddrConvertBluestackToVm(&sp->acl.bd_addr, &cfm->mirror_bd_addr.addrt.addr);

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        else
        {
            /* Re-connect the mirror ACL after entering DISCONNECTED */
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_ACL_LINK_CREATE_IND

    Only Secondary should receive this, because the Primary always
    initiates the mirror ACL connection.
*/
static void mirrorProfile_HandleMdmAclLinkCreateInd(const MDM_ACL_LINK_CREATE_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmAclLinkCreateInd status 0x%x handle 0x%x",
                    ind->status, ind->connection_handle);

    assert(!MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_DISCONNECTED:
        if (ind->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_SECONDARY == ind->role);

            sp->acl.conn_handle = ind->connection_handle;
            BdaddrConvertBluestackToVm(&sp->acl.bd_addr, &ind->mirror_bd_addr.addrt.addr);

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_LINK_CREATE_CFM

    Only Primary should receive this, because the Primary always
    initiates the mirror eSCO connection.
*/
static void mirrorProfile_HandleMdmEscoLinkCreateCfm(const MDM_ESCO_LINK_CREATE_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateCfm status 0x%x handle 0x%x",
                    cfm->status, cfm->connection_handle);

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ESCO_CONNECTING:
        if (cfm->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_PRIMARY == cfm->role);

            sp->esco.conn_handle = cfm->connection_handle;

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ESCO_CONNECTED);
        }
        else
        {
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

    case MIRROR_PROFILE_STATE_DISCONNECTED:
        /* This can occur on mirror ACL disconnect crossover with attempt
           to connect mirror eSCO. The MDM_LINK_DISCONNECT_IND will be received
           first then the MDM_ESCO_LINK_CREATE_CFM(failure) */
        break;

    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
        /* This occurs rarely if a mirror eSCO connection attempt is pending
           when the mirror eSCO/ACL disconnect and the mirror profile attempts to
           reconnect the mirror ACL. In this case, the MDM_ESCO_LINK_CREATE_CFM_T
           from the original eSCO connect request is received when the state
           has transitioned to MIRROR_PROFILE_STATE_ACL_CONNECTING. The status
           is not expected to be success in this case. */
           assert(cfm->status != HCI_SUCCESS);
        break;

    default:
        MirrorProfile_StateError(MDM_ESCO_LINK_CREATE_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_LINK_CREATE_IND

    Only Secondary should receive this, because the Primary always
    initiates the mirror eSCO connection.
*/
static void mirrorProfile_HandleMdmEscoLinkCreateInd(const MDM_ESCO_LINK_CREATE_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd status 0x%x handle 0x%x role %d type %d",
                    ind->status, ind->connection_handle, ind->role, ind->link_type);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd buddy addr {%04x %02x %06lx}",
                    ind->buddy_bd_addr.addrt.addr.nap,
                    ind->buddy_bd_addr.addrt.addr.uap,
                    ind->buddy_bd_addr.addrt.addr.lap);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd mirror addr {%04x %02x %06lx}",
                    ind->mirror_bd_addr.addrt.addr.nap,
                    ind->mirror_bd_addr.addrt.addr.uap,
                    ind->mirror_bd_addr.addrt.addr.lap);
    MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd tx_interval %d wesco %d rx_len 0x%x tx_len 0x%x air_mode 0x%x",
                    ind->tx_interval, ind->wesco, ind->rx_packet_length, ind->tx_packet_length, ind->air_mode);

    assert(!MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        if (ind->status == HCI_SUCCESS)
        {
            assert(ROLE_TYPE_SECONDARY == ind->role);

            sp->esco.conn_handle = ind->connection_handle;
            sp->esco.wesco = ind->wesco;

            MIRROR_LOG("mirrorProfile_HandleMdmEscoLinkCreateInd handle=0x%x", sp->esco.conn_handle);

            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ESCO_CONNECTED);
        }
        break;

    default:
        MirrorProfile_StateError(MDM_ACL_LINK_CREATE_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_ESCO_RENEGOTIATED_IND

    The eSCO parameters have changed but the app is not required to do
    anything in reply as it has been handled by the lower layers.

    Handle it here to keep things tidy.
*/
static void mirrorProfile_HandleMdmEscoRenegotiatedInd(const MDM_ESCO_RENEGOTIATED_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmEscoRenegotiatedInd tx_interval %d wesco %d rx_len 0x%x tx_len 0x%x",
                    ind->tx_interval, ind->wesco, ind->rx_packet_length, ind->tx_packet_length);
}

/*! \brief Handle MDM_LINK_DISCONNECT_CFM

    Only Primary should receive this, in response to Primary sending
    a MDM_LINK_DISCONNECT_REQ.
*/
static void mirrorProfile_HandleMdmLinkDisconnectCfm(const MDM_LINK_DISCONNECT_CFM_T *cfm)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm handle 0x%x status 0x%x type 0x%x role 0x%x",
                cfm->conn_handle, cfm->status, cfm->link_type, cfm->role);

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
        if (cfm->link_type == LINK_TYPE_ACL && cfm->conn_handle == sp->acl.conn_handle)
        {
            switch (cfm->status)
            {
            case HCI_ERROR_CONN_TERM_LOCAL_HOST:
            case HCI_ERROR_OETC_USER:
            case HCI_ERROR_LMP_RESPONSE_TIMEOUT:
            case HCI_ERROR_UNSPECIFIED:
            case HCI_ERROR_CONN_TIMEOUT:
            {
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
                /* Reset state after setting state, so state change handling
                can use this state. */
                sp->acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;
                BdaddrSetZero(&sp->acl.bd_addr);
            }
            break;
            default:
                /* Failure to disconnect typically means the ACL is already
                disconnected and a disconnection indication is in-flight.
                Do nothing and wait for the indication. */
            break;
            }
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_ESCO_DISCONNECTING:
        if (cfm->link_type == LINK_TYPE_ESCO && cfm->conn_handle == sp->esco.conn_handle)
        {
            switch (cfm->status)
            {
            case HCI_ERROR_CONN_TERM_LOCAL_HOST:
            case HCI_ERROR_OETC_USER:
            case HCI_ERROR_LMP_RESPONSE_TIMEOUT:
            case HCI_ERROR_UNSPECIFIED:
            case HCI_ERROR_CONN_TIMEOUT:
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
                MirrorProfile_ResetEscoConnectionState();
            break;
            default:
                /* Failure to disconnect normally means the eSCO is already
                disconnected and a disconnection indication is in-flight.
                Do nothing and wait for the indication. */
            break;
            }
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_DISCONNECTED:
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectCfm invalid state 0x%x", MirrorProfile_GetState());
        break;

    default:
        MirrorProfile_StateError(MDM_LINK_DISCONNECT_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_LINK_DISCONNECT_IND

    Both Primary & Secondary can get this at any time.
    For example, if there is a link-loss between Primary & Secondary.
*/
static void mirrorProfile_HandleMdmLinkDisconnectInd(const MDM_LINK_DISCONNECT_IND_T *ind)
{
    mirror_profile_task_data_t *sp = MirrorProfile_Get();

    MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd handle 0x%x reason 0x%x type 0x%x role 0x%x",
                ind->conn_handle, ind->reason, ind->link_type, ind->role);

    switch (MirrorProfile_GetState())
    {
    case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
    case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
    case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
    case MIRROR_PROFILE_STATE_ACL_CONNECTING:
    case MIRROR_PROFILE_STATE_ACL_CONNECTED:
    case MIRROR_PROFILE_STATE_ESCO_CONNECTING:
    case MIRROR_PROFILE_STATE_DISCONNECTED:
        if (ind->link_type == LINK_TYPE_ACL
            && ind->conn_handle == sp->acl.conn_handle)
        {
            /* \todo If this is a link-loss, should we keep acl.conn_handle? */
            sp->acl.conn_handle = MIRROR_PROFILE_CONNECTION_HANDLE_INVALID;

            /* If we are the Primary, we should retry creating the mirror ACL connection
               after a short delay */
            if (MirrorProfile_IsPrimary())
            {
                /* Re-connect the mirror ACL after entering DISCONNECTED */
                MirrorProfile_SetDelayKick();
            }
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_DISCONNECTED);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd Unrecognised mirror link; ignoring");
        }
        break;

    case MIRROR_PROFILE_STATE_ESCO_CONNECTED:
    case MIRROR_PROFILE_STATE_ESCO_DISCONNECTING:
        if (ind->link_type == LINK_TYPE_ESCO
            && ind->conn_handle == sp->esco.conn_handle)
        {
            MirrorProfile_ResetEscoConnectionState();
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        else
        {
            MIRROR_LOG("mirrorProfile_HandleMdmLinkDisconnectInd Unrecognised mirror link; ignoring");
        }
        break;

    default:
        MirrorProfile_StateError(MDM_LINK_DISCONNECT_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_CREATE_IND.
*/
static void mirrorProfile_HandleMdmMirrorL2capCreateInd(const MDM_L2CAP_CREATE_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateInd cid 0x%x handle 0x%x",
                ind->cid, ind->connection_handle);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        {
            MirrorProfile_GetA2dpState()->cid = ind->cid;
            PanicFalse(ind->connection_handle == MirrorProfile_GetAclState()->conn_handle);
            MirrorProfile_MirrorL2capConnectRsp(
                    MirrorProfile_GetAclState()->conn_handle,
                    MirrorProfile_GetA2dpState()->cid);
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_A2DP_CONNECTING);
        }
        break;
        default:
            MirrorProfile_StateError(MDM_L2CAP_CREATE_IND, NULL);
        break;
    }
}


/*! \brief Handle MDM_L2CAP_CREATE_CFM.
*/
static void mirrorProfile_HandleMdmMirrorL2capCreateCfm(const MDM_L2CAP_CREATE_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateCfm cid 0x%x handle 0x%x result 0x%x",
                cfm->cid, cfm->connection_handle, cfm->result);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        {
            if (cfm->result == L2CA_CONNECT_SUCCESS)
            {
                PanicFalse(cfm->cid == MirrorProfile_GetA2dpState()->cid);
                PanicFalse(cfm->connection_handle == MirrorProfile_GetAclState()->conn_handle);

                MirrorProfile_SetState(MIRROR_PROFILE_STATE_A2DP_CONNECTED);

            }
            else
            {
                MirrorProfile_SetDelayKick();
                MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);

            }
        }
        break;
        case MIRROR_PROFILE_STATE_DISCONNECTED:
        case MIRROR_PROFILE_STATE_ACL_CONNECTING:
            MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capCreateCfm invalid state 0x%x", MirrorProfile_GetState());
        break;
        default:
            MirrorProfile_StateError(MDM_L2CAP_CREATE_CFM, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_DISCONNECT_IND.
*/
static void mirrorProfile_HandleMdmMirrorL2capDisconnectInd(const MDM_L2CAP_DISCONNECT_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectInd cid 0x%x", ind->cid);

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_ACL_CONNECTED:
        case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
        case MIRROR_PROFILE_STATE_DISCONNECTED:
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
        break;

        case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
            /* Handle disconnect crossover. Accept the disconnection, but don't
               change state, just wait for the MDM_L2CAP_DISCONNECT_CFM_T.  */
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
        break;

        case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        {
            MirrorProfile_MirrorL2capDisconnectRsp(ind->cid);
            MirrorProfile_SetDelayKick();
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

        default:
            MirrorProfile_StateError(MDM_L2CAP_DISCONNECT_IND, NULL);
        break;
    }
}

/*! \brief Handle MDM_L2CAP_DISCONNECT_CFM.
*/
static void mirrorProfile_HandleMdmMirrorL2capDisconnectCfm(const MDM_L2CAP_DISCONNECT_CFM_T *cfm)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectCfm cid 0x%x, status 0x%x",
                cfm->cid, cfm->status);

    assert(MirrorProfile_IsPrimary());

    switch (MirrorProfile_GetState())
    {
        case MIRROR_PROFILE_STATE_DISCONNECTED:
        {
            /* When the L2CAP connection between peers disconnects the state
               transitions to MIRROR_PROFILE_STATE_DISCONNECTED. Shortly afterwards
               the mirror connection disconnects resulting in this message which
               is ignored. */
        }
        break;

        case MIRROR_PROFILE_STATE_A2DP_CONNECTED:
        case MIRROR_PROFILE_STATE_A2DP_CONNECTING:
        case MIRROR_PROFILE_STATE_A2DP_DISCONNECTING:
        {
            PanicFalse(cfm->cid == MirrorProfile_GetA2dpState()->cid);
            MirrorProfile_SetState(MIRROR_PROFILE_STATE_ACL_CONNECTED);
        }
        break;

        case MIRROR_PROFILE_STATE_ACL_DISCONNECTING:
            MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDisconnectCfm invalid state 0x%x", MirrorProfile_GetState());
        break;

        default:
            MirrorProfile_StateError(MDM_L2CAP_DISCONNECT_CFM, NULL);
        break;
    }
}

static void mirrorProfile_HandleMdmMirrorL2capDataSyncInd(const MDM_L2CAP_DATA_SYNC_IND_T *ind)
{
    MIRROR_LOG("mirrorProfile_HandleMdmMirrorL2capDataSyncInd cid 0x%x", ind->cid);
    if (ind->cid == MirrorProfile_GetA2dpState()->cid)
    {
        appKymeraA2dpHandleDataSyncInd(StreamL2capSink(ind->cid), ind->clock);
    }
}

/*! \brief Handle MESSAGE_BLUESTACK_MDM_PRIM payloads. */
void MirrorProfile_HandleMessageBluestackMdmPrim(const MDM_UPRIM_T *uprim)
{
    switch (uprim->type)
    {
    case MDM_SET_BREDR_SLAVE_ADDRESS_IND:
        HandoverProfile_HandleMdmSetBredrSlaveAddressInd((const MDM_SET_BREDR_SLAVE_ADDRESS_IND_T *)uprim);
        break;

    case MDM_REGISTER_CFM:
        mirrorProfile_HandleMdmRegisterCfm((const MDM_REGISTER_CFM_T *)uprim);
        break;

    case MDM_ACL_LINK_CREATE_CFM:
        mirrorProfile_HandleMdmAclLinkCreateCfm((const MDM_ACL_LINK_CREATE_CFM_T *)uprim);
        break;

    case MDM_ACL_LINK_CREATE_IND:
        mirrorProfile_HandleMdmAclLinkCreateInd((const MDM_ACL_LINK_CREATE_IND_T *)uprim);
        break;

    case MDM_ESCO_LINK_CREATE_CFM:
        mirrorProfile_HandleMdmEscoLinkCreateCfm((const MDM_ESCO_LINK_CREATE_CFM_T *)uprim);
        break;

    case MDM_ESCO_LINK_CREATE_IND:
        mirrorProfile_HandleMdmEscoLinkCreateInd((const MDM_ESCO_LINK_CREATE_IND_T *)uprim);
        break;

    case MDM_LINK_DISCONNECT_CFM:
        mirrorProfile_HandleMdmLinkDisconnectCfm((const MDM_LINK_DISCONNECT_CFM_T *)uprim);
        break;

    case MDM_LINK_DISCONNECT_IND:
        mirrorProfile_HandleMdmLinkDisconnectInd((const MDM_LINK_DISCONNECT_IND_T *)uprim);
        break;

    case MDM_ESCO_RENEGOTIATED_IND:
        mirrorProfile_HandleMdmEscoRenegotiatedInd((const MDM_ESCO_RENEGOTIATED_IND_T *)uprim);
        break;

    case MDM_L2CAP_CREATE_IND:
        mirrorProfile_HandleMdmMirrorL2capCreateInd((const MDM_L2CAP_CREATE_IND_T *)uprim);
        break;

    case MDM_L2CAP_CREATE_CFM:
        mirrorProfile_HandleMdmMirrorL2capCreateCfm((const MDM_L2CAP_CREATE_CFM_T *)uprim);
        break;

    case MDM_L2CAP_DISCONNECT_IND:
        mirrorProfile_HandleMdmMirrorL2capDisconnectInd((const MDM_L2CAP_DISCONNECT_IND_T *)uprim);
        break;

    case MDM_L2CAP_DISCONNECT_CFM:
        mirrorProfile_HandleMdmMirrorL2capDisconnectCfm((const MDM_L2CAP_DISCONNECT_CFM_T *)uprim);
        break;

    case MDM_L2CAP_DATA_SYNC_IND:
        mirrorProfile_HandleMdmMirrorL2capDataSyncInd((const MDM_L2CAP_DATA_SYNC_IND_T *)uprim);
        break;

    default:
        MIRROR_LOG("MirrorProfile_HandleMessageBluestackMdmPrim: Unhandled MDM prim 0x%x", uprim->type);
        break;
    }
}

#endif /* INCLUDE_MIRRORING */
