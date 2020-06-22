/*!
\copyright  Copyright (c) 2015 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       phy_state.h
\brief	    Definition of the physical state of an Earbud.
*/

#ifndef PHY_STATE_H
#define PHY_STATE_H

#include <task_list.h>

#include "domain_message.h"

#include <marshal.h>

/*! Clients will receive state update messages via #PHY_STATE_CHANGED_IND
    conforming to the following state sequence.
    @startuml

    note "Transitions/events not shown are\n\
not supported and have no effect\n\
Earbud will remain in the current state.\
\n\n\
Dashed arrows illustrate the start of a\n\
multiple state transition via intermediate state.\n\
All activity associated with the intermediate state\n\
will be performed during the transition." as N1

    IN_CASE : Earbud is in the case
    IN_CASE -right-> OUT_OF_EAR : Remove from case\n appPhyStateOutOfCaseEvent()

    OUT_OF_EAR : Earbud is not in the case nor in the ear.
    OUT_OF_EAR -right-> IN_EAR : Put in ear\n appPhyStateInEarEvent()
    OUT_OF_EAR -left-> IN_CASE : Put in case\n appPhyStateInCaseEvent()
    OUT_OF_EAR -down-> OUT_OF_EAR_AT_REST : No motion for N seconds\n appPhyStateNotInMotionEvent()

    OUT_OF_EAR_AT_REST : Earbud is not moving
    OUT_OF_EAR_AT_REST -up-> OUT_OF_EAR : Motion detected\n appPhyStateMotionEvent()
    OUT_OF_EAR_AT_REST .up.> OUT_OF_EAR : Put in case\n appPhyStateInCaseEvent()\n Final state will be IN_CASE\nOR\nPut in Ear\nappPhyStateInEarEvent()\n Final state will be IN_EAR

    IN_EAR : Earbud is in the ear
    IN_EAR : Usable as a microphone and speaker.
    IN_EAR -right-> OUT_OF_EAR : Remove from ear\n appPhyStateOutOfEarEvent()
    IN_EAR .left.> OUT_OF_EAR : Put in case\n appPhyStateInCaseEvent()\n Final state will be IN_CASE

    @enduml
*/

/*! Defines physical state client task list initial capacity */
#define PHY_STATE_CLIENT_TASK_LIST_INIT_CAPACITY 6

/*! \brief Enumeration of the physical states an Earbud can be in.
 */
typedef enum
{
    /*! The earbud physical state is unknown.
        This state value will not be reported to clients. */
    PHY_STATE_UNKNOWN,
    /*! The earbud is in the case. */
    PHY_STATE_IN_CASE,
    /*! The earbud is not in the case nor is it in the ear.
     *  It *may* be in motion or at rest. */
    PHY_STATE_OUT_OF_EAR,
    /*! The earbud is not in the case not is it in the ear, and no motion
     * has been detected for configurable period of time */
    PHY_STATE_OUT_OF_EAR_AT_REST,
    /*! The earbud is in the ear and usuable as a microphone and speaker. */
    PHY_STATE_IN_EAR
} phyState;
/*! \brief Define phyState as a uint8 for marshalling use. */
#define MARSHAL_TYPE_phyState   MARSHAL_TYPE_uint8

/*! \brief Enumeration of physical state event changes. */
typedef enum
{
    phy_state_event_in_case,
    phy_state_event_out_of_case,
    phy_state_event_in_ear,
    phy_state_event_out_of_ear,
    phy_state_event_in_motion,
    phy_state_event_not_in_motion,
} phy_state_event;
/*! Define phy_state_event as a uint8 for marshalling use. */
#define MARSHAL_TYPE_phy_state_event   MARSHAL_TYPE_uint8

/*! \brief Physial State module state. */
typedef struct
{
    /*! Physical State module message task. */
    TaskData task;
    /*! Current physical state of the device. */
    phyState state;
    /*! List of tasks to receive #PHY_STATE_CHANGED_IND notifications. */
    TASK_LIST_WITH_INITIAL_CAPACITY(PHY_STATE_CLIENT_TASK_LIST_INIT_CAPACITY)   client_tasks;
    /*! Stores the motion state */
    bool in_motion;
    /*! Stores the proximity state */
    bool in_proximity;
    /*! Lock used to conditionalise sending of PHY_STATE_INIT_CFM. */
    uint16 lock;
    /*! Last state reported to clients. */
    phyState reported_state;
} phyStateTaskData;

/*! \brief Messages which may be sent by the Physical State module. */
typedef enum phy_state_messages
{
    /*! Initialisation of phy state is complete. */
    PHY_STATE_INIT_CFM = PHY_STATE_MESSAGE_BASE,
    /*! Indication of a changed physical state */
    PHY_STATE_CHANGED_IND,
} PHY_STATE_MSG;

/*! \brief Definition of #PHY_STATE_CHANGED_IND message. */
typedef struct
{
    /*! The physical state which the device is now in. */
    phyState new_state;
    phy_state_event event;
} PHY_STATE_CHANGED_IND_T;
/*! \brief Type descriptor for use by marshalling. */
extern const marshal_type_descriptor_t marshal_type_descriptor_PHY_STATE_CHANGED_IND_T;

/*! \brief Internal messages the physical state module can send itself. */
typedef enum
{
    PHY_STATE_INTERNAL_IN_CASE_EVENT,
    PHY_STATE_INTERNAL_OUT_OF_CASE_EVENT,
    PHY_STATE_INTERNAL_IN_EAR_EVENT,
    PHY_STATE_INTERNAL_OUT_OF_EAR_EVENT,
    PHY_STATE_INTERNAL_MOTION,
    PHY_STATE_INTERNAL_NOT_IN_MOTION,
    /*! Timer used to limit rate of PHY_STATE_CHANGED_IND messages generated. */
    PHY_STATE_INTERNAL_TIMEOUT_NOTIFICATION_LIMIT,
} PHY_STATE_INTERNAL_MSG;

/*!< Physical state of the Earbud. */
extern phyStateTaskData app_phy_state;

/*! Get pointer to physical state data structure */
#define PhyStateGetTaskData()   (&app_phy_state)

/*! Get physical state task used for messaging. */
#define PhyStateGetTask()       (&app_phy_state.task)

/*! Get pointer to physical state client tasks */
#define PhyStateGetClientTasks()   (task_list_flexible_t *)(&app_phy_state.client_tasks)

/*! \brief Register a task for notification of changes in state.
    @param[in] client_task Task to receive PHY_STATE_CHANGED_IND messages.
 */
void appPhyStateRegisterClient(Task client_task);

/*! \brief Unregister a task for notification of changes in state.
    @param[in] client_task Task to unregister.
 */
void appPhyStateUnregisterClient(Task client_task);

/*! \brief Get the current physical state of the device.
    \return phyState Current physical state of the device.
*/
phyState appPhyStateGetState(void);

/*! \brief Check whether in/out of ear detection is supported.
    \return bool TRUE if in/out of ear detection is supported, else FALSE. */
bool appPhyStateIsInEarDetectionSupported(void);

/*! \brief Indicate if the physical state of the device is out of case

    \return TRUE if out of case
*/
bool appPhyStateIsOutOfCase(void);

/*! \brief Handle notification that Earbud is now in the case. */
void appPhyStateInCaseEvent(void);

/*! \brief Handle notification that Earbud is now out of the case. */
void appPhyStateOutOfCaseEvent(void);

/*! \brief Handle notification that Earbud is now in ear. */
void appPhyStateInEarEvent(void);

/*! \brief Handle notification that Earbud is now out of the ear. */
void appPhyStateOutOfEarEvent(void);

/*! \brief Handle notification that Earbud is now moving */
void appPhyStateMotionEvent(void);

/*! \brief Handle notification that Earbud is now not moving. */
void appPhyStateNotInMotionEvent(void);

/*! \brief Tell the phy state module to prepare for entry to dormant.
           Phy state unregisters itself as a client of all sensors which (if
           phy state is the only remaining client), will cause the sensors to
           switch off or enter standby.
 */
void appPhyStatePrepareToEnterDormant(void);

/*! \brief Initialise the module.
    \note #PHY_STATE_INIT_CFM is sent when the phy state is known.
*/
bool appPhyStateInit(Task init_task);

#endif /* PHY_STATE_H */

