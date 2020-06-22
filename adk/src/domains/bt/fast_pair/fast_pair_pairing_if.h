/*!
\copyright  Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file        fast_pair_pairing_if.h
\brief      Header file for the Fast Pairing Interface
*/

#ifndef FAST_PAIR_PAIRING_IF_H_
#define FAST_PAIR_PAIRING_IF_H_

#include <bdaddr.h>
#include "pairing.h"
#include "pairing_plugin.h"

typedef struct
{
    bool isAccept;
    uint32 seeker_passkey;    
}fast_pair_if_t;


/*! \brief FastPair pairing interface data reference.

    This is used to retrieve fast pair pairing interface common data

    \return  pointer to fastpair pairing interface datablock
*/
fast_pair_if_t * fastPair_GetIFData(void);

/*! \brief FastPair pairing interface initialisation.

    This is used to initialise fastpair module's pairing interface
*/
void fastPair_PairingInit(void);

/*! \brief Start fast pair pairing interface activities.

    Once fast pair process has started,state manager can call this.
*/
void fastPair_StartPairing(void);

/*! \brief API to make device discoverable or not.

    \param  set discoverability TRUE/FALSE
*/
bool  fastPair_EnterDiscoverable(bool set);

/*! \brief API to start pairing .

    Initiate Pairing to seeker.

    \param  bd_addr         The address of the remote device
*/
void fastPair_InitiatePairing(const bdaddr *bd_addr);

/*! \brief To check the passkey received from seeker and start next steps with Paiirng Manager.

    Once seeker passkey is received,state manager can call this.

    \param  passkey        remote device generated passkey 
*/
void fastPair_PairingPasskeyReceived(uint32 passkey);

/*! \brief To reset the pairing interface.

    On a fast pair process completion(success/failure/timeout), state manager can call this
*/
void fastPair_PairingReset(void);
#endif /* FAST_PAIR_PAIRING_IF_H_ */
