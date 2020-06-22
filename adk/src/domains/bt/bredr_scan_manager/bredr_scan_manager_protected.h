/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief	    Topology interface to control BREDR scanning
*/

#ifndef BREDR_SCAN_MANAGER_PROTECTED_H_
#define BREDR_SCAN_MANAGER_PROTECTED_H_

#include "bredr_scan_manager.h"

/*! @brief Disable page and inquiry scanning

    @param disabler The task requesting the disable.

    Any active scanning will be paused until #BredrScanManager_ScanEnable is 
    called. The disabler task will be sent a #BREDR_SCAN_MANAGER_DISABLE_CFM_T 
    when scanning has been disabled successfuly (the disabled field will 
    be set to TRUE).

    If #BredrScanManager_ScanEnable is called before active scanning has paused, a
    #BREDR_SCAN_MANAGER_DISABLE_CFM_T message will be sent to the disabler task
    with the disabled field set to FALSE.

    Only one client may use this function, it is not intended for general use.
    Generally, clients should use the #BredrScanManager_PageScanRelease or
    #BredrScanManager_InquiryScanRelease to release their request for scanning
    to be enabled.
*/
void BredrScanManager_ScanDisable(Task disabler);

/*! @brief Allow page and inquiry scan to resume.
*/
void BredrScanManager_ScanEnable(void);


#endif /* BREDR_SCAN_MANAGER_PROTECTED_H_ */
