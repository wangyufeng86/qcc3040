/****************************************************************************
Copyright (c) 2010 - 2020 Qualcomm Technologies International, Ltd.


FILE NAME
    spps_private.h
    
DESCRIPTION
	Header file for the SPP Server profile library containing private members
	
*/

#ifndef SPPS_PRIVATE_H_
#define SPPS_PRIVATE_H_

#include "spps.h"
#include "../spp_common/spp_common_private.h"

/*! Get the SDP service record to be used 

    \return Pointer to the service record
*/
extern const uint8 *spps_service_record(void);

/*! Get the size of the SDP service record

    \return The length of the service record

*/
extern const uint16 spps_service_record_size(void);


/*! Register the SDP service record.

    The RFCOMM channel in the record may be updated, based on the 
    registered channel.

    \param[in] response_handler_task Task that will receive the response from 
                ConnectionRegisterServiceRecord, the message CL_SDP_REGISTER_CFM.
 */
extern void sppsRegisterSdpServiceRecord(Task response_handler_task);

/*! Deregister the SDP service record.

    This tides up appropriately when the service finishes, or can be
    used on connection to stop additional connections.
 */
extern void sppsUnregisterSdpServiceRecord(void);


/* private SPP Server functions */
void sppsStoreServiceHandle(uint32 service_handle);
uint32 sppsGetServiceHandle(void);

#endif /* SPPS_PRIVATE_H_ */
