/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */

#include "trap_api/trap_api_private.h"

#include "trap_api/message.h"
#include "trap_api/vm.h"

#include "bluestack/dm_prim.h"
#include "bluestack/att_prim.h"
#include "bluestack/l2cap_prim.h"
#include "bluestack/rfcomm_prim.h"
#include "bluestack/sds_prim.h"
#include "bluestack/mdm_prim.h"
#include "bluestack/vsdm_prim.h"

#include "ipc/ipc.h"
#include "hydra_log/hydra_log.h"

#ifdef CHIP_DEF_P1_SQIF_SHALLOW_SLEEP_WA_B_195036
#include "memory_map.h"
#endif

PRESERVE_TYPE_FOR_DEBUGGING(ATT_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(DM_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(DM_SYNC_CONFIG_T)
PRESERVE_TYPE_FOR_DEBUGGING(L2CAP_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(RFC_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(SDC_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(SDS_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(MDM_PRIM_T)
PRESERVE_TYPE_FOR_DEBUGGING(VSDM_PRIM_T)

PRESERVE_TYPE_FOR_DEBUGGING(RFC_CLIENT_CONNECT_CFM_T)
PRESERVE_TYPE_FOR_DEBUGGING(RFC_SERVER_CONNECT_CFM_T)
PRESERVE_TYPE_FOR_DEBUGGING(RFC_RESULT_T)

#if TRAPSET_BLUESTACK
Task MessageBlueStackTask(Task task)
{
    return trap_api_register_message_task(task, IPC_MSG_TYPE_BLUESTACK);
}
#endif

#if TRAPSET_ATT
Task MessageAttTask(Task task)
{
    return trap_api_register_message_task(task, IPC_MSG_TYPE_ATT);
}
#endif

#if TRAPSET_MIRRORING
Task MessageMdmTask(Task task)
{
    return trap_api_register_message_task(task, IPC_MSG_TYPE_MDM);
}
#endif

#if TRAPSET_BLUESTACK
void VmSendDmPrim(void * prim)
{
    L3_DBG_MSG1("DM PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(DM_PRIM, prim);
}

void VmSendL2capPrim(void * prim)
{
    L3_DBG_MSG1("L2CAP PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(L2CAP_PRIM, prim);
}


void VmSendRfcommPrim(void * prim)
{
    L3_DBG_MSG1("RFCOMM PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(RFCOMM_PRIM, prim);
}

void VmSendSdpPrim(void * prim)
{
    L3_DBG_MSG1("SDP PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(SDP_PRIM, prim);
}
#endif

#if TRAPSET_MIRRORING
void VmSendMdmPrim(void * prim)
{
    L3_DBG_MSG1("MDM PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(MDM_PRIM, prim);
}
#endif

#if TRAPSET_VSDM
Task MessageVsdmTask(Task task)
{
    return trap_api_register_message_task(task, IPC_MSG_TYPE_VSDM);
}

void VmSendVsdmPrim(void * prim)
{
    L3_DBG_MSG1("VSDM PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(VSDM_PRIM, prim);
}
#endif

#if TRAPSET_ATT
void VmSendAttPrim(void * prim)
{
    L3_DBG_MSG1("ATT PRIM: ID 0x%x",((uint16*)prim)[0]);
    ipc_send_bluestack(ATT_PRIM, prim);
}
#endif

#if TRAPSET_BLUESTACK
void *VmGetPointerFromHandle(void *hdl)
{
    return hdl;
}

void *VmGetHandleFromPointer(void *ptr)
{
#ifdef CHIP_DEF_P1_SQIF_SHALLOW_SLEEP_WA_B_195036
    /* Work around the invisibility of P1's const space from P0 by pointing at
     * P0's view of the same values */
    return MEMORY_MAP_ADJUST_CONST_P1_P0(ptr, void *);
#else
    return ptr;
#endif /* CHIP_DEF_P1_SQIF_SHALLOW_SLEEP_WA_B_195036 */
}

bool VmGetBdAddrtFromCid(uint16 cid, tp_bdaddr * tpaddr)
{
    IPC_VM_GET_BD_ADDRT_FROM_CID send;
    IPC_VM_GET_BD_ADDRT_FROM_CID_RSP rsp;
    send.cid = cid;
    ipc_transaction(IPC_SIGNAL_ID_VM_GET_BD_ADDRT_FROM_CID, &send, sizeof(send),
                    IPC_SIGNAL_ID_VM_GET_BD_ADDRT_FROM_CID_RSP, &rsp);
    if (rsp.ret && tpaddr != NULL)
    {
        tpaddr->transport = (TRANSPORT_T)rsp.tpaddr.tp_type;
        tpaddr->taddr.type = rsp.tpaddr.addrt.type;
        tpaddr->taddr.addr.lap = rsp.tpaddr.addrt.addr.lap;
        tpaddr->taddr.addr.uap = rsp.tpaddr.addrt.addr.uap;
        tpaddr->taddr.addr.nap = rsp.tpaddr.addrt.addr.nap;
    }
    return rsp.ret;
}

bool VmGetPublicAddress(const tp_bdaddr *random_addr, tp_bdaddr *public_addr)
{
    IPC_VM_GET_PUBLIC_ADDRESS ipc_send_prim;
    IPC_VM_GET_PUBLIC_ADDRESS_RSP ipc_recv_prim;
    ipc_send_prim.random_addr = (const TP_BD_ADDR_T *)random_addr;
    ipc_transaction(IPC_SIGNAL_ID_VM_GET_PUBLIC_ADDRESS, &ipc_send_prim, sizeof(ipc_send_prim),
                    IPC_SIGNAL_ID_VM_GET_PUBLIC_ADDRESS_RSP, &ipc_recv_prim);

    if (ipc_recv_prim.ret && public_addr != NULL)
    {
        public_addr->transport = (TRANSPORT_T)ipc_recv_prim.public_addr.tp_type;
        public_addr->taddr.type = ipc_recv_prim.public_addr.addrt.type;
        public_addr->taddr.addr.lap = ipc_recv_prim.public_addr.addrt.addr.lap;
        public_addr->taddr.addr.uap = ipc_recv_prim.public_addr.addrt.addr.uap;
        public_addr->taddr.addr.nap = ipc_recv_prim.public_addr.addrt.addr.nap;
    }
    return ipc_recv_prim.ret;
}

bool VmGetLocalIrk(packed_irk *irk)
{
    IPC_VM_GET_LOCAL_IRK ipc_send_prim;
    IPC_VM_GET_LOCAL_IRK_RSP ipc_recv_prim;
    /*lint -e(603) allow uninitialised*/
    ipc_transaction(IPC_SIGNAL_ID_VM_GET_LOCAL_IRK, &ipc_send_prim, sizeof(ipc_send_prim),
                    IPC_SIGNAL_ID_VM_GET_LOCAL_IRK_RSP, &ipc_recv_prim);
    memcpy(irk->irk, ipc_recv_prim.irk.irk, ARRAY_DIM(irk->irk) * sizeof(uint16_t));
    return ipc_recv_prim.ret;
}

uint8 VmGetAclMode(const tp_bdaddr *acl_address)
{
    IPC_VM_GET_ACL_MODE send;
    IPC_UINT8_RSP rsp;
    send.acl_address = (const TP_BD_ADDR_T *)acl_address;
    ipc_transaction(IPC_SIGNAL_ID_VM_GET_ACL_MODE, &send, sizeof(send),
                    IPC_SIGNAL_ID_VM_GET_ACL_MODE_RSP, &rsp);
    return rsp.ret;
}

uint8 VmGetAclRole(const tp_bdaddr *acl_address)
{
    IPC_VM_GET_ACL_ROLE send;
    IPC_UINT8_RSP rsp;
    send.acl_address = (const TP_BD_ADDR_T *)acl_address;
    ipc_transaction(IPC_SIGNAL_ID_VM_GET_ACL_ROLE, &send, sizeof(send),
                    IPC_SIGNAL_ID_VM_GET_ACL_ROLE_RSP, &rsp);
    return rsp.ret;
}

bool VmBdAddrGetRssi(const tp_bdaddr *tpaddr, int16 *rssi)
{
    IPC_VM_BD_ADDR_GET_RSSI send_prim;
    IPC_VM_BD_ADDR_GET_RSSI_RSP rsp_prim;
    TP_BD_ADDR_T tpaddr_fw;

    /* Copy the supplied App-space structure into a temporary fw-space
     * structure that's the right shape to go to firmware */
    tpaddr_fw.tp_type = (PHYSICAL_TRANSPORT_T)tpaddr->transport;
    tpaddr_fw.addrt.type = tpaddr->taddr.type;
    tpaddr_fw.addrt.addr.lap = tpaddr->taddr.addr.lap;
    tpaddr_fw.addrt.addr.uap = tpaddr->taddr.addr.uap;
    tpaddr_fw.addrt.addr.nap = tpaddr->taddr.addr.nap;
    send_prim.tpaddr = &tpaddr_fw;

    ipc_transaction(IPC_SIGNAL_ID_VM_BD_ADDR_GET_RSSI, &send_prim, sizeof(send_prim),
                    IPC_SIGNAL_ID_VM_BD_ADDR_GET_RSSI_RSP, &rsp_prim);
    if (rssi != NULL)
    {
      *rssi = rsp_prim.rssi;
    }
    return rsp_prim.ret;
}
#endif /* TRAPSET_BLUESTACK */


void trap_api_send_bluestack_message(uint16 protocol, void *prim)
{
    Task hdlr;

    switch (protocol)
    {
        case ATT_PRIM:
            hdlr = registered_hdlrs[IPC_MSG_TYPE_ATT];
        break;
        case MDM_PRIM:
            hdlr = registered_hdlrs[IPC_MSG_TYPE_MDM];
        break;
        case VSDM_PRIM:
            hdlr = registered_hdlrs[IPC_MSG_TYPE_VSDM];
        break;
        default:
            hdlr = registered_hdlrs[IPC_MSG_TYPE_BLUESTACK];
        break;
    }

    if (hdlr)
    {
        switch (protocol)
        {
            case ATT_PRIM:
                L3_DBG_MSG1("ATT EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case DM_PRIM:
                L3_DBG_MSG1("DM EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case L2CAP_PRIM:
                L3_DBG_MSG1("L2CAP EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case RFCOMM_PRIM:
                L3_DBG_MSG1("RFCOMM EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case SDP_PRIM:
                L3_DBG_MSG1("SDP EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case MDM_PRIM:
                L3_DBG_MSG1("MDM EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            case VSDM_PRIM:
                L3_DBG_MSG1("VSDM EVT: ID 0x%x",((uint16*)prim)[0]);
            break;
            default:
               L3_DBG_MSG1("UNKNOWN EVT: ID 0x%x",((uint16*)prim)[0]);
        }

        /* We need to give this the system message ID corresponding to its
         * protocol ID by offsetting it by MESSAGE_BLUESTACK_BASE_ */
        MessageSend(hdlr, (uint16)(MESSAGE_BLUESTACK_BASE_ + protocol),
                           prim);
    }
    else
    {
        L1_DBG_MSG1("WARNING: dropping Bluestack message with ID 0x%x because no "
                    "handler set", protocol);
        pfree(prim);
    }
}


