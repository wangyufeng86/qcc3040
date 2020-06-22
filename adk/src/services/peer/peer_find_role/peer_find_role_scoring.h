/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       
\brief      Header file for score calcuation of the find role service
*/

#ifndef PEER_FIND_ROLE_SCORING_H_
#define PEER_FIND_ROLE_SCORING_H_

#include <phy_state.h>
#include <gatt_role_selection_service.h>

/*! Collection of information used in scoring peer devices.

    The values here are populated based on messages received from
    other system modules.
*/
typedef struct
{
        /*! The current physical state of this device */
    phyState                phy_state;
        /*! Does this device have a charger */
    bool                    charger_present;
        /*! Is the device moving ? 
            An accelerometer message will indicate when we are no longer moving */
    bool                    accelerometer_moving;
        /*! What is the current battery level (%age) */
    uint8                   battery_level_percent;
        /*! The score that has most recently been calculated for this device */
    grss_figure_of_merit_t  last_local_score;
        /*! Test override of score. */
    grss_figure_of_merit_t  score_override;
        /* most recently reported FOM of the peer device */
    grss_figure_of_merit_t  peer_score;
        /*! Is the handset connected to this device. */
    bool                    handset_connected;
} peer_find_role_scoring_t;


/*! Make any initialisation required for scoring 

    This functions registers for indications from different modules.
*/
void peer_find_role_scoring_setup(void);


/*! Calculate a score

    Use information cached from autonomous indications to calculate
    a "score". Cache the new score in the peer_find_role context.

    Note: Does not update the gatt server score value.
*/
void peer_find_role_calculate_score(void);

/*! Get the local score to use

    This can be overriden by a test command.

    \returns The local score to be used in calculations 
 */
grss_figure_of_merit_t peer_find_role_score(void);


/*! \brief Update the figure of merit value in the gatt server

    Updates the figure of merit score for the local device and updates
    the gatt server figure of merit characteristic.
*/
void peer_find_role_update_server_score(void);

/*! \brief Reset the figure of merit value in the gatt server

    Set the figure of merit characteristic to GRSS_FIGURE_OF_MERIT_INVALID.
*/
void peer_find_role_reset_server_score(void);

/*! \brief Store the figure of merit of the peer device locally.

    \param[in]  score The figure of merit of the peer device to be stored.
*/
void PeerFindRole_SetPeerScore(grss_figure_of_merit_t score);

/*! \brief Get the locally stored figure of merit of the peer device.

    \returns The locally stored figure of merit of the peer device.
 */
grss_figure_of_merit_t PeerFindRole_GetPeerScore(void);

/*! \name Calculation of scores for bitmask 

    The 'figure of merit' or score used to determine which device should take
    the primary role is made up of a number of bit-fields.

    Macros are defined that calculate enumerated values that can then 
    be used in other macros. The score for a particular element can be
    retrieved using PEER_FIND_ROLE_SCORE() and passing the name of the
    element and the current value.

    The macros allow for scaling such that a battery level of 0-100 can
    be packed into any number of bits. If the maximum value can be
    represented exactly, then no scaling takes place.

    \example 
    \li If placed in 6 bits, battery will scale to values from 0-63
    \li If placed in 7 bits, battery will have values 0-100 only
*/
/*! @{ */


/*! Macro that creates enumerated values for a scoring item.
    The enumerated values are created using token pasting and the 
    name of the score elemement

    \param name         The name of the element in the score
    \param max_input    The maximum value for the element
    \param offset       The bit offset. 0 means starts at right
    \param num_bits     Maximum number of bits the element will use
 */
#define SCORING_DEFINE_MACRO(name, max_input, offset, num_bits) \
    PEER_FIND_ROLE_SCORING_##name##_INPUT_MAX = (max_input), \
    PEER_FIND_ROLE_SCORING_##name##_OFFSET = (offset), \
    PEER_FIND_ROLE_SCORING_##name##_NUM_BITS = (num_bits), \
    PEER_FIND_ROLE_SCORING_##name##_SCALED = ((max_input) > ((1 << (num_bits))-1)),


/*! Define an item in the score.

    This is made up of the name, maximum value, offset within the score
    and the number of bits.

    The allocation is defined in a secondary macro SCORING_DEFINE_MACRO in
    order to take advantage of the C pre-processor.
    */
#define SCORE_WEIGHTS(XFUNC) \
    XFUNC(ROLE_CP,              1,                 1,  1) \
    XFUNC(BATTERY,              100,               2,  7) \
    XFUNC(POWERED,              1,                 9,  1) \
    XFUNC(PHY_STATE,            PHY_STATE_IN_EAR,  10, 3) \
    XFUNC(HANDSET_CONNECTED,    1,                 13, 1)


/*! The list of enumerated values that are used in scoring. These
    are generated entirely by the pre-processor 

    \note Do not call directly
 */
typedef enum {
    PEER_FIND_ROLE_SCORE_INVALID = 0,
    PEER_FIND_ROLE_SCORE_LOWEST = 1,
    SCORE_WEIGHTS(SCORING_DEFINE_MACRO)
} peer_find_role_scoring_defines_t;


/*! Helper macro that will return a value scaled to fit into the 
    required number of bits. This should only be called when the score
    item requires scaling.

    \note Do not call directly

    \param value    The input value
    \param max      The maximum value that the score item can take
    \param num_bits The number of bits to use 

    \return value scaled to a value that fits into num_bits bits
 */
#define PEER_FIND_ROLE_SCALED_VALUE(value, max, num_bits) (((value) * ((1 << (num_bits))-1)) / (max))


/*! Helper macro that returns the value required to fit into the 
    score item.

    As used the pre-processor ensures that the value supplied is 
    either scaled or returned. The parameter scaled will be
    a constant.

    \param value    The value to be processed
    \param max      The maximum value possible for value
    \param scaled   Whether the output value is scaled
    \param num_bits The number of bits for the value

    \return Value required to fit into num_bits bits
 */
#define PEER_FIND_ROLE_VALUE(value, max, scaled, num_bits) ((scaled) ? PEER_FIND_ROLE_SCALED_VALUE(value, max, num_bits) : (value))


/*! Macro to find the value for a score item. This uses helper 
    macros to return a value that fits within the required number
    of bits specified in the #SCORE_WEIGHTS macro.

    \param name     name of the score item
    \param value    value of the score item to produce

    \returns    Value suitable for use in the score. This value should be ORed into
                the score 
 */
#define PEER_FIND_ROLE_SCORE(name, value)   (PEER_FIND_ROLE_VALUE((value), PEER_FIND_ROLE_SCORING_##name##_INPUT_MAX, \
                                                    PEER_FIND_ROLE_SCORING_##name##_SCALED, \
                                                    PEER_FIND_ROLE_SCORING_##name##_NUM_BITS) \
                                                 << PEER_FIND_ROLE_SCORING_##name##_OFFSET)

/*! @} */

#endif /* PEER_FIND_ROLE_SCORING_H_ */
