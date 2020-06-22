/*!
\copyright  Copyright (c) 2019-2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      State machine transitions and logic for mirror_profile.
*/

#ifndef MIRROR_PROFILE_SM_H_
#define MIRROR_PROFILE_SM_H_

/*! \brief The bitmasks for the sub-states of the mirror_profile state machine. */
typedef enum
{
    MIRROR_PROFILE_SUB_STATE_DISCONNECTED       = 0x010, /*!< No peer connected; mirror links will not be created. */
    MIRROR_PROFILE_SUB_STATE_ACL_CONNECTED      = 0x040, /*!< Mirror ACL link connected. */
    MIRROR_PROFILE_SUB_STATE_ESCO_CONNECTED     = 0x080, /*!< Mirror eSCO link connected. */
    MIRROR_PROFILE_SUB_STATE_A2DP_CONNECTED     = 0x100, /*!< Mirror A2DP link connected. */
} mirror_profile_sub_state_t;

/*! \brief Mirror Profile States

    The main states of a mirror profile link depend on the previous state.
    For example, a mirror eSCO connection must first have the peer earbud
    connected and a mirror ACL link to the earbud before it can be created.

    The state enum values below represent this by using bitmasks to group
    states based on whether the peer is disconnected, mirror ACL is connected,
    and finally if mirror eSCO or mirror A2DP is connected.

    Mirror eSCO and mirror A2DP are mutually exclusive operations.

    Transition States:
    The state machine has stable and transition states. A transition state is
    one where it is waiting for a reply from the firmware only. Other
    messages should be blocked until the reply has been received.

    A stable state is one where it is ok to process messages from any origin,
    e.g. internal messages (see #mirror_profile_internal_msg_t).

    The transition lock is set when going into a transition state. Any
    internal messages should be sent conditional on this lock.

    Stable states
    * MIRROR_PROFILE_STATE_DISCONNECTED
    * MIRROR_PROFILE_STATE_ACL_CONNECTED
    * MIRROR_PROFILE_STATE_ESCO_CONNECTED
    * MIRROR_PROFILE_STATE_A2DP_CONNECTED

    Transition states
    * MIRROR_PROFILE_STATE_ACL_CONNECTING
    * MIRROR_PROFILE_STATE_ESCO_CONNECTING
    * MIRROR_PROFILE_STATE_ACL_DISCONNECTING
    * MIRROR_PROFILE_STATE_ESCO_DISCONNECTING
    * MIRROR_PROFILE_STATE_A2DP_DISCONNECTING

    \note The stable states are also the main sub-states of the state machine.


    Pseudo States:
    The state machine has a concept of pseudo-states that group together states
    to represent a sub-state of the overall state machine. In the enum below
    the pseudo-states are marked by the top 4 bits in the first byte of the
    enum value.

    The pseudo-states are:
    ACL_CONNECTED       (MIRROR_PROFILE_SUB_STATE_ACL_CONNECTED)
    ESCO_CONNECTED      (MIRROR_PROFILE_SUB_STATE_ESCO_CONNECTED | ACL_CONNECTED)
    A2DP_CONNECTED     (MIRROR_PROFILE_SUB_STATE_A2DP_CONNECTED | ACL_CONNECTED)

    These are mainly used for testing what the sub-state is when the state
    machine is in a transition state.

    @startuml

    state DISCONNECTED : No mirror connection
    state ACL_CONNECTING : Primary initiated mirror ACL connect in progress
    state ACL_CONNECTED : Mirror ACL connected
    state ACL_DISCONNECTING : Primary initiated mirror ACL disconnect in progress
    state ESCO_CONNECTING : Primary initiated mirror eSCO connect in progress
    state ESCO_CONNECTED : Mirror eSCO connected
    state ESCO_DISCONNECTING : Primary initiated mirror eSCO disconnect in progress
    state A2DP_CONNECTING: Primary initated mirror A2DP connect in progress
    state A2DP_CONNECTED : Mirror A2DP connected
    state A2DP_DISCONNECTING : Primary initiated mirror A2DP disconnect in progress

    [*] -d-> DISCONNECTED

    DISCONNECTED --> ACL_CONNECTING : Create ACL locally
    DISCONNECTED --> ACL_CONNECTED : ACL created remotely

    ACL_CONNECTING --> DISCONNECTED : Fail
    ACL_CONNECTING --> ACL_CONNECTED : Success

    ACL_CONNECTED --> ESCO_CONNECTING : Create eSCO locally
    ACL_CONNECTED --> ESCO_CONNECTING : Link-loss retry
    ACL_CONNECTED --> ESCO_CONNECTED : eSCO created remotely
    ACL_CONNECTED --> ACL_DISCONNECTING : Disconnect ACL locally
    ACL_CONNECTED --> DISCONNECTED : Disconnect ACL remotely
    ACL_CONNECTED --> DISCONNECTED : ACL connection timeout

    ACL_DISCONNECTING --> DISCONNECTED : ACL disconnected

    ESCO_CONNECTING --> ACL_CONNECTED : Fail
    ESCO_CONNECTING --> ESCO_CONNECTED : Success

    ESCO_CONNECTED --> ACL_CONNECTED : Disconnect (remote or timeout)
    ESCO_CONNECTED --> ESCO_DISCONNECTING : Disconnect eSCO locally

    ESCO_DISCONNECTING --> ACL_CONNECTED : eSCO disconnected

    ACL_CONNECTED --> A2DP_CONNECTING : Create mirror A2DP local/remote
    A2DP_CONNECTING --> A2DP_CONNECTED : Success
    A2DP_CONNECTING --> ACL_CONNECTED : Fail
    A2DP_CONNECTED --> A2DP_DISCONNECTING : Local/remote disconnect
    A2DP_DISCONNECTING --> ACL_CONNECTED : A2DP disconnected


    @enduml
*/
typedef enum
{
    /*! No mirror connections and peer not connected. */
    MIRROR_PROFILE_STATE_DISCONNECTED               = MIRROR_PROFILE_SUB_STATE_DISCONNECTED,

        /*! Locally initiated mirror ACL connection in progress. */
        MIRROR_PROFILE_STATE_ACL_CONNECTING         = MIRROR_PROFILE_STATE_DISCONNECTED + 1,

            /* ACL_CONNECTED sub-state */

            /*! Mirror ACL connected. */
            MIRROR_PROFILE_STATE_ACL_CONNECTED      = MIRROR_PROFILE_SUB_STATE_ACL_CONNECTED,
            /*! Locally initiated mirror eSCO connection in progress. */
            MIRROR_PROFILE_STATE_ESCO_CONNECTING    = MIRROR_PROFILE_STATE_ACL_CONNECTED + 1,

                /* ESCO_CONNECTED sub-state */

                /*! Mirror eSCO connected. */
                MIRROR_PROFILE_STATE_ESCO_CONNECTED = (MIRROR_PROFILE_SUB_STATE_ESCO_CONNECTED | MIRROR_PROFILE_STATE_ACL_CONNECTED),

            /*! Locally initiated mirror eSCO disconnect in progress. */
            MIRROR_PROFILE_STATE_ESCO_DISCONNECTING  = MIRROR_PROFILE_STATE_ACL_CONNECTED + 2,

            /*! Local or remote mirror A2DP connection in progress. */
            MIRROR_PROFILE_STATE_A2DP_CONNECTING = MIRROR_PROFILE_STATE_ACL_CONNECTED + 3,

                /* A2DP_CONNECTED sub-state */

                /*! Mirror A2DP connected. */
                MIRROR_PROFILE_STATE_A2DP_CONNECTED = (MIRROR_PROFILE_SUB_STATE_A2DP_CONNECTED | MIRROR_PROFILE_STATE_ACL_CONNECTED),

            /*! Local or remote mirror A2DP disconnection in progress. */
            MIRROR_PROFILE_STATE_A2DP_DISCONNECTING = MIRROR_PROFILE_STATE_ACL_CONNECTED + 4,


        /*! Locally initiated mirror ACL disconnect in progress. */
        MIRROR_PROFILE_STATE_ACL_DISCONNECTING       = MIRROR_PROFILE_STATE_DISCONNECTED + 2,

} mirror_profile_state_t;

/*!@{ \name Masks used to check for the sub-state of the state machine. */
#define MIRROR_PROFILE_STATE_MASK_ACL_CONNECTED         (MIRROR_PROFILE_STATE_ACL_CONNECTED)
#define MIRROR_PROFILE_STATE_MASK_ESCO_CONNECTED        (MIRROR_PROFILE_STATE_ESCO_CONNECTED)
#define MIRROR_PROFILE_STATE_MASK_A2DP_CONNECTED        (MIRROR_PROFILE_STATE_A2DP_CONNECTED)
/*!@} */

/*! \brief Is mirror_profile sub-state 'ACL connected' */
#define MirrorProfile_IsSubStateAclConnected(state) \
    (((state) & MIRROR_PROFILE_STATE_MASK_ACL_CONNECTED) == MIRROR_PROFILE_STATE_ACL_CONNECTED)

/*! \brief Is mirror_profile sub-state 'ESCO connected' */
#define MirrorProfile_IsSubStateEscoConnected(state) \
    (((state) & MIRROR_PROFILE_STATE_MASK_ESCO_CONNECTED) == MIRROR_PROFILE_STATE_ESCO_CONNECTED)

/*! \brief Is mirror_profile sub-state 'A2DP connected' */
#define MirrorProfile_IsSubStateA2dpConnected(state) \
    (((state) & MIRROR_PROFILE_STATE_MASK_A2DP_CONNECTED) == MIRROR_PROFILE_STATE_A2DP_CONNECTED)

/*! If no other bits are set than those defined in this mask, the state is steady. */
#define STEADY_STATE_MASK  (MIRROR_PROFILE_SUB_STATE_DISCONNECTED   | \
                            MIRROR_PROFILE_SUB_STATE_ACL_CONNECTED  | \
                            MIRROR_PROFILE_SUB_STATE_ESCO_CONNECTED | \
                            MIRROR_PROFILE_SUB_STATE_A2DP_CONNECTED)

/*! \brief Is state a steady state? */
#define MirrorProfile_IsSteadyState(state) (((state) | STEADY_STATE_MASK) == STEADY_STATE_MASK)

/*! \brief Is mirror profile in a steady state? */
#define MirrorProfile_IsInSteadyState() MirrorProfile_IsSteadyState(MirrorProfile_GetState())


/*! \brief Tell the mirror_profile state machine to go to a new state.

    Changing state always follows the same procedure:
    \li Call the Exit function of the current state (if it exists)
    \li Call the Exit function of the current psuedo-state if leaving it.
    \li Change the current state
    \li Call the Entry function of the new psuedo-state (if necessary)
    \li Call the Entry function of the new state (if it exists)

    \param state New state to go to.
*/
void MirrorProfile_SetState(mirror_profile_state_t state);

/*! \brief Set a new target state for the state machine.

    \param state New state to target.

    The target state should be a steady state as described above.
*/
void MirrorProfile_SetTargetState(mirror_profile_state_t target_state);

/*! \brief Assess the target state vs the current state and transition to a state
    that will achieve (or be a step towards) the target state.

    The state machine will only transition if it is in a stable state.

    If the delay_kick flag is set, the kick will be deferred.
 */
void MirrorProfile_SmKick(void);

/*! \brief Handle mirror_profile error

    Some error occurred in the mirror_profile state machine.

    Currently the behaviour is to log the error and panic the device.
*/
void MirrorProfile_StateError(MessageId id, Message message);

#endif /* MIRROR_PROFILE_SM_H_ */
