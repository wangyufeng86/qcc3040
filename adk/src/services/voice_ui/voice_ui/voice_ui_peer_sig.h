/*!
\copyright  Copyright (c) 2020 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   voice_assistant peer signalling
\ingroup    Service
\brief      A component responsible for updating peer device with the active voice assistant

Responsibilities:
- A component responsible for updating the peer device with the active voice assistant
*/

#ifndef VOICE_UI_PEER_SIG_H_
#define VOICE_UI_PEER_SIG_H_

#include <csrtypes.h>

#define VOICE_UI_INTERNAL_REBOOT (0x2000)


/*! \brief Initialise the Voice UI Peer Signalling module
*/
void VoiceUi_PeerSignallingInit(void);

/*! \brief Update the peer's active Voice UI provider.

    \param voice_ui_provider ID of the Voice UI provider
    \param reboot if TRUE, reboot the peer when the update is complete
 */
void VoiceUi_UpdateSelectedPeerVaProvider(bool reboot);


#endif //VOICE_UI_PEER_SIG_H_
