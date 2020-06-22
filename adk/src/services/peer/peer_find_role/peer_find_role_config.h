/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Configuration items for the find role service.
*/

#ifndef PEER_FIND_ROLE_CONFIG_H_
#define PEER_FIND_ROLE_CONFIG_H_


/*! Time after which advertising will be started when starting find role

    A backoff is applied when a connection for find role fails. This
    is effectively the initial backoff.

    This is specified in milliseconds
*/
#define PeerFindRoleConfigInitialAdvertisingBackoffMs()     0


/*! Time after which to disconnect the find role link from the server side

    A timeout is needed here to reduce the risk of one device determining
    a role, while the other side does not get the opposite role.

    A long timeout will block the server device from changing its 
    Bluetooth address as it is required there are no active links.
 */
#define PeerFindRoleConfigGattDisconnectTimeout()           100


/*! Time after which the server will stop waiting for the client role selection indication

    A timeout is needed here to workaround the case when the server is waiting
    for the client to inform it of the result of the role selection, but for
    whatever reason the client indication never arrives.
 */
#define PeerFindRoleConfigServerRoleSelectedTimeoutMs()     5000


/*! Time delay to allow scanning after an activity finishes.

    Scanning is blocked by activities that use Bluetooth bandwidth, although
    advertising will still happen.

    When the activity stops, a short delay is recommended as another
    activity could start - for instance a call disconnecting and music
    restarting.
 */
#define PeerFindRoleConfigAllowScanAfterActivityDelayMs()   D_SEC(2)


/*! Time delay to wait for figure of merit notification from the server

    If no notification is received within this timeframe we probably
    will not get one. If we reach this timeout we go and ask for the
    characteristic value
*/
#define PeerFindRoleConfigWaitForFomNotificationDelayMs()   D_SEC(1)


/*! The MTU value needed to optimise the operation of GATT for
    peer find role

    This value is selected so that the initial discovery can be 
    completed using a single request/response (although this may
    use several packets).
 */

/*! The MTU value needed to optimise the operation of GATT for
    peer find role

    This value is selected so that the initial discovery can be 
    completed using a single request/response (although this may
    use several packets).
 */
#define PeerFindRoleConfigMtu()     65

/*! User PsKey to use for fixed role setting.
 */
#define FIXED_ROLE_PSKEY 2


#endif /* PEER_FIND_ROLE_CONFIG_H_ */
