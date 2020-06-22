/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\defgroup   hdma HDMA
\ingroup    peer_service
\brief      Component is used to decide when handover is required between primary and secondary earbuds. It currently uses placement of earbud (in case, in ear, out ear) and critical battery level for making a handover decision.
Decision making based on battery level is configurable. The input will be sourced from the State Proxy module, which is responsible for communication between the buds.
To source these inputs, HDMA will register a task with State Proxy during initialization.
*/


#ifndef HDMA_H
#define HDMA_H

#include "message.h"
#include "domain_message.h"

/*\{*/

/*! Enumeration of messages the HDMA module can send to its clients */
typedef enum {
    /*! HDMA initialisation complete. */
    HDMA_EVENT = HDMA_MESSAGE_BASE,
    /*! HDMA handover notification. */
    HDMA_HANDOVER_NOTIFICATION,
    /*! HDMA handover cancel notification. */
    HDMA_CANCEL_HANDOVER_NOTIFICATION
}hdma_messages_t;

/*! Enum of HDMA handover reasons*/
typedef enum {
	/*! HDMA handover reason invalid. */
    HDMA_HANDOVER_REASON_INVALID=0,
	/*! HDMA handover reason battery. */
    HDMA_HANDOVER_REASON_BATTERY_LEVEL,
	/*! HDMA handover reason voice quality. */
    HDMA_HANDOVER_REASON_VOICE_QUALITY,
	/*! HDMA handover reason signal quality. */
    HDMA_HANDOVER_REASON_SIGNAL_QUALITY,
	/*! HDMA handover reason earbud is in case. */
    HDMA_HANDOVER_REASON_IN_CASE,
	/*! HDMA handover reason earbud is out of ear */
    HDMA_HANDOVER_REASON_OUT_OF_EAR,
	/*! HDMA handover reason external. */
    HDMA_HANDOVER_REASON_EXTERNAL,
}hdma_handover_reason_t;

/*! Enum describing handover urgency */
typedef enum {
    /*! HDMA handover urgency invalid. */
	HDMA_HANDOVER_URGENCY_INVALID=0,
	/*! HDMA handover urgency low. */
    HDMA_HANDOVER_URGENCY_LOW,
	/*! HDMA handover urgency high. */
    HDMA_HANDOVER_URGENCY_HIGH,
	/*! HDMA handover urgency critical. */
    HDMA_HANDOVER_URGENCY_CRITICAL,
}hdma_handover_urgency_t;

/*! Structure describing handover data */
typedef struct {
    /*! Time at which handover event is generated */
    uint32 timestamp;
    /*! Reason for the handover decision*/
    hdma_handover_reason_t reason;
    /*! Urgency indicator for the handover decision*/
    hdma_handover_urgency_t urgency;
}hdma_handover_decision_t;


/*! \brief Initialise the HDMA component.
    \param[in] client_task Task to register for #hdma_messages_t messages.

    \return bool TRUE if initialisation successful
                 FALSE Initialisation failed
*/
bool Hdma_Init(Task client_task);

/*! \brief De-Initialise the HDMA module
    \return bool TRUE if De-initialisation successful
                 FALSE De-initialisation failed
*/
bool Hdma_Destroy(void);

/*! \brief Trigger an external request to handover.
    \return bool TRUE if external handover request was sent, otherwise FALSE.

    \note This will cause a hdma_handover_decision_t to be sent to the client_task
    with reason HDMA_HANDOVER_REASON_EXTERNAL and HDMA_HANDOVER_URGENCY_CRITICAL.
*/
bool Hdma_ExternalHandoverRequest(void);

/*\}*/

#endif /* HDMA_H */

