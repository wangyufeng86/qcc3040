/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\defgroup   media_player Media Player
\ingroup    services
\brief      A component responsible for controlling audio (music) sources.

Responsibilities:
- Media control (like play, pause, fast forward) - Commands are passed to the audio source
  media control API in response to UI inputs.

The Media Player uses \ref audio_domain Audio domain and \ref ui_domain UI domain.

*/

#ifndef MEDIA_PLAYER_H_
#define MEDIA_PLAYER_H_

/*\{*/

/*! \brief Initialise the media player service

    \param init_task Unused
 */
bool MediaPlayer_Init(Task init_task);

/*\}*/

#endif /* MEDIA_PLAYER_H_ */
