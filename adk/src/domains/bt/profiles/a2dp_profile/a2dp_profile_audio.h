/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   a2dp_profile_audio A2DP Profile Audio
\ingroup    a2dp_profile
\brief      The audio source audio interface implementation for A2DP sources
*/

#ifndef A2DP_PROFILE_AUDIO_H_
#define A2DP_PROFILE_AUDIO_H_

#include "audio_sources_audio_interface.h"
#include "source_param_types.h"

/*\{*/

/*! \brief Gets the A2DP handset audio interface.

    \return The audio source audio interface for an A2DP handset source
 */
const audio_source_audio_interface_t * A2dpProfile_GetHandsetSourceAudioInterface(void);

/*! \brief Gets the A2DP Peer audio interface.

    \return The audio source audio interface for an A2DP peer source
 */
const audio_source_audio_interface_t * A2dpProfile_GetPeerSourceAudioInterface(void);

/*! \brief Gets the A2DP forwarding connect parameters.

    \return TRUE if the parameters are valid, else FALSE
 */
bool A2dpProfile_GetForwardingConnectParameters(source_defined_params_t * source_params);

/*! \brief Free the A2DP forwarding connect parameters.

 */
void A2dpProfile_FreeForwardingConnectParameters(source_defined_params_t * source_params);

/*! \brief Gets the A2DP forwarding disconnect parameters.

    \return TRUE if the parameters are valid, else FALSE
 */
bool A2dpProfile_GetForwardingDisconnectParameters(source_defined_params_t * source_params);

/*! \brief Free the A2DP forwarding disconnect parameters.

 */
void A2dpProfile_FreeForwardingDisconnectParameters(source_defined_params_t * source_params);

/*\}*/

#endif /* A2DP_PROFILE_AUDIO_H_ */
