/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Link policy manager for TWM builds.

When mirroring, the topology is fixed and role switches are not desirable
for any device in the topology.

The primary earbud is master of the link to the secondary earbud, and the roles
only switch when the devices perform handover. These roles are enforced by a call
to #appLinkPolicyAlwaysMaster in tws topology.

The handset is master of the link to the primary earbud. Mirroring is currently
only supported when the primary earbud is ACL slave. Since being ACL master is
preferable (from link scheduling point of view) compared to being slave, there
is no reason for the primary earbud to request a role switch (once slave) and no
good reason for the handset to request a role switch to become slave. Therefore,
calls to allow/prohibit or change roles (that were valid in non-mirroring
topology) are ignored here.
*/

#include "link_policy.h"
#include "logging.h"
#include "bdaddr.h"
#include "bt_device.h"
#include "app/bluestack/dm_prim.h"
#include "panic.h"

#ifdef INCLUDE_MIRRORING

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);

/*! Make and populate a bluestack DM primitive based on the type.

    \note that this is a multiline macro so should not be used after a
    control statement (if, while) without the use of braces
 */
#define MAKE_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;

void appLinkPolicyAllowRoleSwitch(const bdaddr *bd_addr)
{
    UNUSED(bd_addr);
    DEBUG_LOG("appLinkPolicyAllowRoleSwitch ignored when mirroring");
}

void appLinkPolicyAllowRoleSwitchForSink(Sink sink)
{
    UNUSED(sink);
    DEBUG_LOG("appLinkPolicyAllowRoleSwitchForSink ignored when mirroring");
}

void appLinkPolicyPreventRoleSwitch(const bdaddr *bd_addr)
{
    UNUSED(bd_addr);
    DEBUG_LOG("appLinkPolicyPreventRoleSwitch ignored when mirroring");
}

void appLinkPolicyPreventRoleSwitchForSink(Sink sink)
{
    UNUSED(sink);
    DEBUG_LOG("appLinkPolicyPreventRoleSwitchForSink ignored when mirroring");
}

void appLinkPolicyUpdateRoleFromSink(Sink sink)
{
    UNUSED(sink);
    DEBUG_LOG("appLinkPolicyUpdateRoleFromSink ignored when mirroring");
}

static void appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(const bdaddr *bd_addr, uint8 override_value)
{
    MAKE_PRIM_T(DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->host_support_override = override_value;
    VmSendDmPrim(prim);
    DEBUG_LOG("appLinkPolicyBredrSecureConnectionHostSupportOverrideSet 0x%x:%d", bd_addr->lap, override_value);
}

void appLinkPolicyBredrSecureConnectionHostSupportOverrideEnable(const bdaddr *bd_addr)
{
    appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(bd_addr, 0x01);
}

void appLinkPolicyBredrSecureConnectionHostSupportOverrideRemove(const bdaddr *bd_addr)
{
    appLinkPolicyBredrSecureConnectionHostSupportOverrideSet(bd_addr, 0xFF);
}

/*! \brief Send a prim directly to bluestack to role switch to slave.
    \param The address of remote device link.
    \param The role to request.
*/
static void role_switch(const bdaddr *bd_addr, hci_role new_role)
{
    ConnectionSetRoleBdaddr(&LinkPolicyGetTaskData()->task, bd_addr, new_role);
}

/*! \brief Prevent role switching

    Update the policy for the connection (if any) to the specified
    Bluetooth address, so as to prevent any future role switching.

    \param  bd_addr The Bluetooth address of the device
*/
static void prevent_role_switch(const bdaddr *bd_addr)
{
    MAKE_PRIM_C(DM_HCI_WRITE_LINK_POLICY_SETTINGS_REQ);
    BdaddrConvertVmToBluestack(&prim->bd_addr, bd_addr);
    prim->link_policy_settings = ENABLE_SNIFF;
    VmSendDmPrim(prim);
}

/*! \brief Discover the current role

    \param  bd_addr The Bluetooth address of the remote device
*/
static void discover_role(const bdaddr *bd_addr)
{
    ConnectionGetRoleBdaddr(&LinkPolicyGetTaskData()->task, bd_addr);
}

void appLinkPolicyHandleClDmAclOpendedIndication(const CL_DM_ACL_OPENED_IND_T *ind)
{
    const bool is_ble = !!(ind->flags & DM_ACL_FLAG_ULP);
    const bdaddr *bd_addr = &ind->bd_addr.addr;

    /*  For mirroring, the primary earbud must always be the slave of the handset
        ACL link and master of the secondary ACL link. */
    if (!is_ble)
    {
        if (appDeviceIsHandset(bd_addr))
        {
            discover_role(bd_addr);
        }
        else if (appDeviceIsSecondary(bd_addr))
        {
            discover_role(bd_addr);
        }
        else if (appDeviceIsPrimary(bd_addr))
        {
            /* The primary will initiate any role switches required */
        }
    }
}

/*! Handle a change in role, returning any further role switches required */
static hci_role appLinkPolicyHandleRole(const bdaddr *bd_addr, hci_role role)
{
    const bool is_handset = appDeviceIsHandset(bd_addr);
    const bool is_secondary = appDeviceIsSecondary(bd_addr);
    const bool is_master = (role == hci_role_master);
    hci_role new_role = hci_role_dont_care;

    if (is_handset)
    {
        if (is_master)
        {
            DEBUG_LOG("appLinkPolicyHandleRole, master of handset");
            new_role = hci_role_slave;
        }
        else
        {
            DEBUG_LOG("appLinkPolicyHandleRole, slave of handset");
            /* This is the desired topology, disallow role switches */
            prevent_role_switch(bd_addr);
        }
    }
    else if (is_secondary)
    {
        if (is_master)
        {
            DEBUG_LOG("appLinkPolicyHandleRole, master of secondary");
            appLinkPolicyUpdateLinkSupervisionTimeout(bd_addr);
        }
        else
        {
            DEBUG_LOG("appLinkPolicyHandleRole, slave of secondary");
            new_role = hci_role_master;
        }
    }
    return new_role;
}

/*! \brief Indication of link role

    This function is called to handle a CL_DM_ROLE_IND message, this message is sent from the
    connection library whenever the role of a link changes.

    \param  ind The received indication
*/
static void appLinkPolicyHandleClDmRoleIndication(const CL_DM_ROLE_IND_T *ind)
{
    const bdaddr *bd_addr = &(ind->bd_addr);
    hci_role new_role = hci_role_dont_care;

    DEBUG_LOG("appLinkPolicyHandleClDmRoleIndication %x, role:%d, status=%d", bd_addr->lap, ind->role, ind->status);

    switch (ind->status)
    {
    case hci_success:
        new_role = appLinkPolicyHandleRole(bd_addr, ind->role);
    break;

    case hci_error_role_change_not_allowed:
    case hci_error_role_switch_failed:
    case hci_error_controller_busy:
    case hci_error_instant_passed:
        discover_role(bd_addr);
    break;

    case hci_error_role_switch_pending:
    case hci_error_conn_timeout:
    case hci_error_no_connection:
    break;

    default:
        /* Unexpected status. */
        Panic();
    break;
    }

    if (new_role != hci_role_dont_care)
    {
        role_switch(bd_addr, new_role);
    }
}

static void appLinkPolicyHandleClDmRoleCfm(const CL_DM_ROLE_CFM_T *cfm)
{
    const bdaddr *bd_addr = &(cfm->bd_addr);
    hci_role new_role = hci_role_dont_care;

    PanicFalse(cfm->cfmtype == cl_role_get_bdaddr || cfm->cfmtype == cl_role_set_bdaddr);

    DEBUG_LOG("appLinkPolicyHandleClDmRoleCfm %x, status:%d, role:%d", bd_addr->lap, cfm->status, cfm->role);

    switch (cfm->status)
    {
    case hci_success:
        new_role = appLinkPolicyHandleRole(bd_addr, cfm->role);
    break;

    case hci_error_role_change_not_allowed:
    case hci_error_role_switch_failed:
    case hci_error_lmp_response_timeout:
    case hci_error_unrecognised:
    case hci_error_controller_busy:
        /* Unsuccessful role switch, restart by discovering the role */
        discover_role(bd_addr);
    break;

    case hci_error_role_switch_pending:
    break;

    default:
        /* Unexpected status. */
        DEBUG_LOG("appLinkPolicyHandleClDmRoleCfm, unexpected status %d", cfm->status);
    break;
    }

    if (new_role != hci_role_dont_care)
    {
        role_switch(bd_addr, new_role);
    }
}

static void appLinkPolicyHandleScHostSupportOverrideCfm(const DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM_T *cfm)
{
    if (cfm->status == hci_success)
    {
        DEBUG_LOG("appLinkPolicyHandleScHostSupportOverrideCfm, 0x%x:%d", cfm->bd_addr.lap, cfm->host_support_override);
    }
    else
    {
        Panic();
    }
}

bool appLinkPolicyHandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    switch (id)
    {
        case CL_DM_ROLE_CFM:
            appLinkPolicyHandleClDmRoleCfm(message);
        return TRUE;

        case CL_DM_ROLE_IND:
            appLinkPolicyHandleClDmRoleIndication(message);
            return TRUE;

        case DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_CFM:
            appLinkPolicyHandleScHostSupportOverrideCfm(message);
            return TRUE;

        case CL_DM_ULP_SET_PRIVACY_MODE_CFM:
        {
            CL_DM_ULP_SET_PRIVACY_MODE_CFM_T *cfm = (CL_DM_ULP_SET_PRIVACY_MODE_CFM_T *)message;
            PanicFalse(cfm->status == hci_success);
            return TRUE;
        }
    }
    return already_handled;
}

void appLinkPolicyRegisterTestTask(Task task)
{
    UNUSED(task);
}

#endif
