#ifndef __INQUIRY_H__
#define __INQUIRY_H__
#include <app/bluestack/types.h>

/*! file  @brief Configure Bluetooth inquiry procedure.
**
**
These functions can be used to change the details of how BlueCore
schedules Bluetooth inquiry relative to other Bluetooth activity.
They are equivalent to the Inquiry_Priority BCCMD.
*/

#if TRAPSET_BLUESTACK

/**
 *  \brief Sets the priority level of Bluetooth inquiry.
 *           Trap unsupported on QCC514x and QCC304x and newer devices, if called
 *  it will return FALSE. 
 *     
 *  \param priority The desired priority level.
 *  \return TRUE if the level was successfully set, FALSE otherwise.
 * 
 * \ingroup trapset_bluestack
 */
bool InquirySetPriority(InquiryPriority priority);

/**
 *  \brief Gets the current priority level of Bluetooth inquiry.
 *           Trap unsupported on QCC514x and QCC304x and newer devices, if called
 *  it will return 0. 
 *     
 *  \return The current priority level.
 * 
 * \ingroup trapset_bluestack
 */
InquiryPriority InquiryGetPriority(void );
#endif /* TRAPSET_BLUESTACK */
#endif
