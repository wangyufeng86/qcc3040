/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Header file for setting up SDP record for TWS topology
*/

#ifndef TWS_TOPOLOGY_SDP_H_
#define TWS_TOPOLOGY_SDP_H_

#include <connection.h>
#include <bdaddr.h>

/*! \brief Register the TWS service record.

    Starts the registration of the SDP record for TWS.
    \param[in] response_task  Task to send #CL_SDP_REGISTER_CFM response message to.
 */
void TwsTopology_RegisterServiceRecord(Task response_task);

/*! \brief Handle the response from the call to ConnectionRegisterServiceRecord.

    Handle the response from the call to ConnectionRegisterServiceRecord within the
    function TwsTopology_RegisterServiceRecord.

    \param[in] task  currently unused.
    \param[in] cfm  response from the connection library.

 */
void TwsTopology_HandleClSdpRegisterCfm(Task task, const CL_SDP_REGISTER_CFM_T *cfm);


/*! \brief Handle the response from the call to ConnectionUnRegisterServiceRecord.

    Handle the response from the call to ConnectionUnRegisterServiceRecord within the
    function TwsTopology_RegisterServiceRecord. This will go on to register a new
    service record.

    \param[in] task Task to send the response from ConnectionRegisterServiceRecord.
    \param[in] cfm  response from the connection library.

 */
void TwsTopology_HandleClSdpUnregisterCfm(Task task,
                                          const CL_SDP_UNREGISTER_CFM_T *cfm);

/*! \brief Check if device with bd_addr supports TWS+
    \param[in] bd_addr Address of device to check
 */
void TwsTopology_SearchForTwsPlusSdp(bdaddr bd_addr);

/*! \brief Handle response from connection library.
    Handle response from connection library result of calling TwsTopology_SearchForTwsPlusSdp().
    \param[in] cfm Response from connection library.
 */
void TwsTopology_HandleClSdpServiceSearchAttributeCfm(const CL_SDP_SERVICE_SEARCH_ATTRIBUTE_CFM_T *cfm);

#endif /* TWS_TOPOLOGY_SDP_H_ */

