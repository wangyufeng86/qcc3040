/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       qualcomm_connection_manager.h
\brief      Header file for Qualcomm Connection Manager
*/

#ifndef __QCOM_CON_MANAGER_H
#define __QCOM_CON_MANAGER_H


#include <message.h>
#include <bdaddr.h>
#include "domain_message.h"

/*! \brief vendor information */

typedef struct{

     /*! Company/vendor specific id. */
    uint16 comp_id;
    /*! Minimum lmp version for sc/qhs support. */
    uint8 min_lmp_version;
    /*! Minimum lmp sub version for sc/qhs support. */
    uint16 min_lmp_sub_version;

} QCOM_CON_MANAGER_SC_OVERRIDE_VENDOR_INFO_T;

/*! \brief Vendor information array buffer to be provided by application.
     This array will be terminated by setting company id(comp_id) to 0.
     Maximum number of company ids supported are VSDM_MAX_NO_OF_COMPIDS.
*/
extern const QCOM_CON_MANAGER_SC_OVERRIDE_VENDOR_INFO_T vendor_info[];

/*! \brief Events sent by qualcomm connection manager module to other modules. */
typedef enum
{
    /*! Module initialisation complete */
    QCOM_CON_MANAGER_INIT_CFM = QCOM_CON_MANAGER_MESSAGE_BASE,

    /*! QHS Established Indication to mirroring profile */
    QCOM_CON_MANAGER_QHS_CONNECTED,

}qcm_msgs_t;


typedef struct
{
    /*! BT address of qhs connected device. */
    bdaddr bd_addr;

} QCOM_CON_MANAGER_QHS_CONNECTED_T;


#ifdef INCLUDE_QCOM_CON_MANAGER

/*! \brief Initialise the qualcomm connection manager module.
 */
bool QcomConManagerInit(Task init_task);

/*! \brief Register a client task to receive notifications of qualcomm connection manager.

    \param[in] client_task Task which will receive notifications from qualcomm conenction manager.
 */
void QcomConManagerRegisterClient(Task client_task);

/*! \brief Unregister a client task to stop receiving notifications from qualcomm conenction manager.

    \param[in] client_task Task to unregister.
 */
void QcomConManagerUnRegisterClient(Task client_task);

#else

#define QcomConManagerRegisterClient(task) UNUSED(task)

#define QcomConManagerUnRegisterClient(task) UNUSED(task)

#endif /* INCLUDE_QCOM_CON_MANAGER */

#endif /*__QCOM_CON_MANAGER_H*/
