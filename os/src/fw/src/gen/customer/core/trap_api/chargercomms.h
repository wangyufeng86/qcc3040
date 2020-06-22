#ifndef __CHARGERCOMMS_H__
#define __CHARGERCOMMS_H__
/** \file */
#if TRAPSET_CHARGERCOMMS

/**
 *  \brief         Transmit a message to a charger using 2-wire Charger Comms.
 *         A transmission can only be made as a response to a
 *  MESSAGE_CHARGERCOMMS_IND message.
 *         
 *         Every message must contain a one octet header, followed by up to 31
 *  octets extra payload.
 *         The maximum packet length is therefore 32 octets.
 *         The header must contain the length of extra payload as described below:
 *         L: Length (5 bits)
 *         S: Spare  (3 bits)
 *         
 *            msb    lsb
 *             |      |
 *             SSSLLLLL
 *              ^   ^
 *              |   |
 *              | length
 *              |   
 *            spare
 *         Every transmit request will be followed with a
 *  MESSAGE_CHARGERCOMMS_STATUS containing transmission status.
 *         
 *  \param length             The total length of the packet (including the header) to be
 *  transmitted in octets.
 *             The minimum length is therefore 1, to hold the header and the
 *  maximum is 32 octets
 *             (1 octet header, 31 octet payload).
 *             
 *  \param data             The data to be transmitted, including the header.
 *             
 *  \return           Boolean to indicate whether the request was successful. 
 *           
 * 
 * \ingroup trapset_chargercomms
 */
bool ChargerCommsTransmit(uint16 length, uint8 * data);
#endif /* TRAPSET_CHARGERCOMMS */
#endif
