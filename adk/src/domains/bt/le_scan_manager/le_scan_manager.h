/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   le_scan_manager LE Scan Manager
\ingroup    bt_domain
\brief      LE scanning component

LE scanning component with support for multiple clients.
Clients can request its own scan interval and advertising filter.

Each advertising filter request is applied.
Which means that a set of passed-through adverts is a combination of all client's requests.

Whereas the scan interval is worked out by the LE Scan Manager.
If at least one client requests fast scan interval then the scan interval will be fast.
Otherwise the scan interval will be slow.

Scan window is not an explicit parameter but it is derived from the scan interval.

*/

#ifndef LE_SCAN_MANAGER_H_
#define LE_SCAN_MANAGER_H_

#include <connection.h>
#include "domain_message.h"

/*! Slow scan interval = 125ms
    \note Slow scan parameters of 25/125ms have been chosen to match 100ms advertising interval */
#define SCAN_INTERVAL_SLOW      0xc8
/*! Slow scan window = 25ms */
#define SCAN_WINDOW_SLOW        0x28

/*! Fast scan interval = 90ms
    \note Using the same interval and window allows all supported bluetooth devices to schedule effectively */
#define SCAN_INTERVAL_FAST      0x90
/*! Fast scan window = 90ms  */
#define SCAN_WINDOW_FAST        0x90


#define MAX_ACTIVE_SCANS        2



/*! \brief LE scan interval types. */
typedef enum
{
    le_scan_interval_slow = 0,
    le_scan_interval_fast = 1,
} le_scan_interval_t;

/*! \brief LE advertising report filter. */
typedef struct
{
    ble_ad_type ad_type;
    uint16 interval;
    tp_bdaddr* find_tpaddr;
    uint16 size_pattern;
    uint8* pattern;
} le_advertising_report_filter_t;

/*! \brief Confirmation Messages to be sent to ccanning clients */
typedef enum scan_manager_messages
{
    /*! Scan Manager Enabled */
    LE_SCAN_MANAGER_ENABLE_CFM = LE_SCAN_MANAGER_MESSAGE_BASE,
    /*! Scan Manager Disabled */
    LE_SCAN_MANAGER_DISABLE_CFM,
    /*! Scanning in progress */
    LE_SCAN_MANAGER_START_CFM,
    /*! Scanning has been stopped */
    LE_SCAN_MANAGER_STOP_CFM,
    /*! Scanning has been paused */
    LE_SCAN_MANAGER_PAUSE_CFM,
    /*! Scanning has resumed */
    LE_SCAN_MANAGER_RESUME_CFM,
    /*! Scan Advertisment Report */
    LE_SCAN_MANAGER_ADV_REPORT_IND,
}scanMessages;

/*! \brief result of le scan operations */
typedef enum {
    LE_SCAN_MANAGER_RESULT_SUCCESS,
    LE_SCAN_MANAGER_RESULT_FAILURE,
    LE_SCAN_MANAGER_RESULT_BUSY
}le_scan_result_t;

/*! \brief Data sent with LE_SCAN_MANAGER_ENABLE_CFM message. */
typedef struct {
    /*! Scan Enable Status */
    le_scan_result_t status;
}LE_SCAN_MANAGER_ENABLE_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_DISABLE_CFM message. */
typedef struct {
    /*! Scan Disable Status */
    le_scan_result_t status;
}LE_SCAN_MANAGER_DISABLE_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_START_CFM message. */
typedef struct {
    le_scan_result_t status;
} LE_SCAN_MANAGER_START_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_STOP_CFM message. */
typedef struct {
    /*! Scan Stop Status */
    le_scan_result_t status;
}LE_SCAN_MANAGER_STOP_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_PAUSE_CFM message. */
typedef struct {
    /*! Scan Stop Status */
    le_scan_result_t status;
}LE_SCAN_MANAGER_PAUSE_CFM_T;

/*! \brief Data sent with LE_SCAN_MANAGER_RESUME_CFM message. */
typedef struct {
    /*! Scan Stop Status */
    le_scan_result_t status;
}LE_SCAN_MANAGER_RESUME_CFM_T;



/*! Advertisement Report to be sent to Application. */
typedef CL_DM_BLE_ADVERTISING_REPORT_IND_T LE_SCAN_MANAGER_ADV_REPORT_IND_T;

/*! \brief Function to handle the Events from Connecion Dispatcher */
extern bool LeScanManager_HandleConnectionLibraryMessages(MessageId id, Message message,
                                                          bool already_handled);


/*! \brief Init the LE Scan Manager Module. This API is called first before any
           other Scan Manager API.
           This function initializes the Task ID for Scan Manager Module.
           Inovacation of any APIs prior to Init, causes a Panic.

    \param[in] Requester's Task \ref Task
*/
bool LeScanManager_Init(Task init_task);

/*! \brief Enables the LE Scan Manager Module.
           LE Scans can only be started if the component is enabled.
           Once enabled, Scan Manager registers with Conection Dispatcher.

           This message is responded with LE_SCAN_MANAGER_STATE_ENABLED_CFM.
           \ref LE_SCAN_MANAGER_STATE_ENABLED_CFM_T for message data.

    \param[in] Requester's Task \ref Task
*/
void LeScanManager_Enable(Task task);

/*! \brief Disables the LE Scan Manager Module. Disable command stops ongoing scan
           and removes all ongoing/pending scan requests.

           This messages is responded with LE SCAN_MANAGER_DISABLE_CFM. No data is
           associated with this response. This message is sent to all active
           clients.

    \param[in] Requester's Task \ref Task
*/
void LeScanManager_Disable(Task task);

/*! \brief Start scanning for an LE device using the provided scan parameters.
           If there is already an ongoing scan, Scan Manager selects fastest
           scan interval among the requests.

           Once scan is started, LE SCAN_MANAGER_START_CFM message is sent
           as response to requester. \ref LE_SCAN_MANAGER_START_CFM_T for
           corresponding message data.

           Scan client's task can be later used by client to stop the scan.

           If scan cannot be started, LE SCAN_MANAGER_START_CFM is
           sent with status LE_SCAN_MANAGER_RESULT_FAILURE.
           This could happen if scan has been disabled.

           If Scan Manager has been Paused (using LeScanManager_Pause()), the
           request is queued. Confirmation would be sent to requester once the
           scanning resumes.

    \param[in] fast   scan interval \ref le_scan_interval_t
    \param[in] filter pointer to advertising filter \ref le_advertising_report_filter_t
    \param[in] task   Requester's Task \ref Task
*/
void LeScanManager_Start(Task task, le_scan_interval_t scan_interval, le_advertising_report_filter_t * filter);

/*! \brief Stops scanning for an LE device.
           If scanning is ongoing for multiple clients, scanning filter corresponding
           to the callee is removed and scanning continues. Scanning is stopped
           only when all the clients have requested Stop. This request is honored
           even when scanning has been paused by any client.

           This request is responded with LE_SCAN_MANAGER_STOP_CFM message.

           The LE_SCAN_MANAGER_STOP_CFM will be sent with status LE_SCAN_MANAGER_RESULT_SUCCESS
           when the scan is stopped, or if a scanning is already stopped and
           LeScanManager_Stop is called again.

    \param[in] task   Requester's Task \ref Task
*/
void LeScanManager_Stop(Task task);

/*! \brief Pauses the scan.
           This request stops the scan for all active clients. However no indication
           is sent to the clients. The message is useful when a client wants to
           temporarily pause the scanning activity for a connection request.
           Subsequent Resume request causes scanning to re-start.

           This request is responded with LE SCAN_MANAGER_PAUSE_CFM. No data is
           associated with this response. This response is sent even in a case
           when scanning has already been stopped.

           Any Start requests received during Pause are queued up.
    \param[in] task Requester's Task \ref Task
*/
void LeScanManager_Pause(Task task);

/*! \brief Resume the scan.
           This request resumes the scan for all active clients. However no indication
           is sent to the active clients.

           This request is responded with LE SCAN_MANAGER_RESUME_CFM. No data is
           associated with this response. This response is sent even in a case
           when there are no active clients to resume the scanning.

    \param[in] task Requester's Task \ref Task
*/
void LeScanManager_Resume(Task task);

/*! \brief Query if a task has enabled scanning.

    \param[in] task The task to query.

    \return TRUE if the task has enabled scanning, otherwise FALSE.
*/
bool LeScanManager_IsTaskScanning(Task task);

/*\}*/

#endif /* LE_SCAN_MANAGER_H_ */
