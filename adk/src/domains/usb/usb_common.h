/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       usb_common.h
\brief      Header file for usb
            This file serves as header file for USB domain. It mainly provides API 
            for time crtical USB operations as well as API's for other domain to register
            /Unregister USB notifications 
*/

#ifndef USB_COMMON_H_
#define USB_COMMON_H_

#include <message.h>
#include <task_list.h>

/*! \brief structure holding usb task data and task list*/
typedef struct
{
    TaskData task;                      /*!< usb module Task. */
    task_list_t clients;               /*!< List of client tasks */
} usbTaskData;

/*! \brief Provides an API for calling time critical USB operation
          This API is called from application processor P0, even before the
          P1 is initialized
    \param     void
    \return    void.
*/
void Usb_TimeCriticalInit(void);

/*! \brief Provides an API for other domains so that they can register
           to receive USB notifications
    \param     task The client's task.
    \return    void.
*/
void Usb_ClientRegister(Task task);

/*! \brief Provides an API for other domains so that they can Un-register
           themselves from recieving further notifications.
    \param     task The client's task.
    \return    void.
*/
void Usb_ClientUnRegister(Task task);

/*! \Configuration for USB ANC tuning */
#define usbConfigAncTuningEnabled()             (TRUE)

/*! \Making globalusbTaskData visible to other domain*/
extern usbTaskData globalusbTaskData;

/*! \Get pointer to the Usb Task data structure */
#define UsbGetTaskData()    (&globalusbTaskData)

/*! \Get the clients data registered for nitifications */
#define Usb_GetClients()     (&globalusbTaskData.clients)

/*! \brief This act as a message handler for USB domain.
     All the USB message should be handelled here.
    \param    task the calling task
    \param    id the message id
    \param    message the message containg additional data required.
    \return void.
*/
void Usb_HandleMessage(Task task, MessageId id, Message message);

/*! \brief structure holding usb audio sample rate*/
typedef struct
{
    uint16 sample_rate;
}MESSAGE_USB_ENUMERATED_T;

/*! \brief Provides an API for attaching the USB device
    This API allows other domain to call the USB library for attaching the USB device.
    \param    void
    \return   void.
*/
void Usb_AttachtoHub(void);

/*! \brief Provides an API for detaching the USB device
    This API allows other domain to call the USB library for detaching the USB device.
    \param    void
    \return   void.
*/
void Usb_DetachFromHub(void);

#endif
