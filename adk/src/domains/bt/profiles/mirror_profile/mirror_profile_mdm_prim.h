/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Functions to send & process MDM prims to / from the firmware.
*/

#ifndef MIRROR_PROFILE_MDM_PRIM_H_
#define MIRROR_PROFILE_MDM_PRIM_H_

#ifdef INCLUDE_MIRRORING

#include <app/bluestack/mdm_prim.h>
#include <hfp.h>


/*! \brief Register with the mirroring (MDM) sub-system.

    The firmware will reply with a MDM_REGISTER_CFM sent to the task
    registered with MessageMdmTask().
*/
void MirrorProfile_MirrorRegisterReq(void);

/*! \brief Create a mirror ACL or eSCO link from the Primary

    Only the Primary can send this message. The Secondary must leave the
    creation of mirror ACL/eSCO links to the Primary.

    The handset bdaddr is the addr of the link to mirror.
    The peer earbud is the addr to use as the Secondary.

    \param type The type of mirror link to request; either ACL or eSCO.
*/
void MirrorProfile_MirrorConnectReq(link_type_t type);

/*! \brief Disconnect a mirror ACL or eSCO link from the Primary.

    Only the Primary can send this message. The Secondary must leave the
    disconnect of mirror ACL/eSCO links to the Primary.

    \param conn_handle The mirror connection handle to disconnect.
    \param reason The reason for the disconnect.
*/
void MirrorProfile_MirrorDisconnectReq(hci_connection_handle_t conn_handle, hci_reason_t reason);


/*! \brief Connect a mirror L2CAP.

    Only the Primary can send this message. The Secondary must leave the
    creation of mirror ACL/eSCO links to the Primary.
    This function sends a MDM_L2CAP_CREATE_REQ.

    \param conn_handle The connection handle of the mirror ACL.
    \param cid The L2CAP CID (from primary to handset) to mirror.
*/
void MirrorProfile_MirrorL2capConnectReq(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid);

/*! \brief Respond to a MDM_L2CAP_CREATE_IND.

    Only the secondary should call this.
    This function sends a MDM_L2CAP_CREATE_RSP.

    \param conn_handle The connection handle of the mirror ACL.
    \param cid The mirror L2CAP CID.
*/
void MirrorProfile_MirrorL2capConnectRsp(hci_connection_handle_t conn_handle,
                                         l2ca_cid_t cid);

/*! \brief Disconnect a mirror L2CAP.

    Only the Primary should call this.
    This function sends a MDM_L2CAP_DISCONNECT_REQ.

    \param cid The mirror L2CAP CID.
*/
void MirrorProfile_MirrorL2capDisconnectReq(l2ca_cid_t cid);

/*! \brief Respond to a MDM_L2CAP_DISCONNECT_IND.

    Only the secondary should call this.
    This function sends a MDM_L2CAP_DISCONNECT_RSP.

    \param cid The mirror L2CAP CID.
*/
void MirrorProfile_MirrorL2capDisconnectRsp(l2ca_cid_t cid);

/*! \brief Handle MESSAGE_BLUESTACK_MDM_PRIM payloads sent from firmware.

    #uprim is a union of all the MDM prim types and this function determines
    which MDM prim it is and passes it onto the relevant handler.

    \param uprim The MDM prim payload to be processed.
*/
void MirrorProfile_HandleMessageBluestackMdmPrim(const MDM_UPRIM_T *uprim);

#endif /* INCLUDE_MIRRORING */

#endif /* MIRROR_PROFILE_MDM_PRIM_H_ */
