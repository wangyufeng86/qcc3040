#ifndef __TEST2_H__
#define __TEST2_H__
#include <bdaddr_.h>
#include <test2_.h>

        /*! file  @brief New set of functions to enter Bluetooth test modes 
        for QCC514x QCC304x and later devices. */
      
#if TRAPSET_TEST2

/**
 *  \brief Radio transmit continuously on the specified channel. 
 *  \param channel The channel used for the transmission. Between 0 and 78. 
 *  \param power The transmit power table lookup value to use. Between 0 and 9. 
 *  \return TRUE on success, else FALSE. 
 * 
 * \ingroup trapset_test2
 */
bool Test2CwTransmit(uint16 channel, uint16 power);

/**
 *  \brief Stop the current test mode. 
 *  \return TRUE on success, else FALSE. 
 * 
 * \ingroup trapset_test2
 */
bool Test2RfStop(void );

/**
 *  \brief The device transmit the specified data on the specified channels
 *  \param channel               This is an array of five channels to hop between.
 *               To transmit on only one channel, write the same channel five
 *  times.
 *               Channel numbers between 0 and 78.
 *             
 *  \param power The transmit power table lookup value to use. Between 0 and 9. 
 *  \param hopping Allows or disallows the hop.
 *             0x00 Transmit on the specified five channels
 *             0x01 Start transmit at channel(0) then hops over the full range of
 *  channels
 *             
 *  \param payload Bits pattern sent.
 *               0x00 All zeros
 *               0x01 All ones
 *               0x02 Alternating bit
 *               0x03 Alternating nibble
 *               0x04 Pseudo-random
 *               Others Reserved
 *             
 *  \param packet_type Type of packet sent.
 *               0x00 NULL packets
 *               0x01 POLL packets
 *               0x02 FHS packets
 *               0x03 DM1 packets
 *               0x04 DH1 packets
 *               0x0A DM3 packets
 *               0x0B DH3 packets
 *               0x0E DM5 packets
 *               0x0F DH5 packets
 *               0x09 AUX1 packets
 *               0x24 2-DH1 packets
 *               0x2A 2-DH3 packets
 *               0x2E 2-DH5 packets
 *               0x28 3-DH1 packets
 *               0x2B 3-DH3 packets
 *               0x2F 3-DH5 packets
 *               0x05 HV1 packets
 *               0x06 HV2 packets
 *               0x07 HV3 packets
 *               0x08 DV packets
 *               0x17 EV3 packets
 *               0x1C EV4 packets
 *               0x1D EV5 packets
 *               0x36 2-EV3 packets
 *               0x3C 2-EV5 packets
 *               0x37 3-EV3 packets
 *               0x3D 3-EV5 packets
 *               Others Reserved
 *             
 *  \param packet_length Length of the packet, in bytes. Depending of the packet_type. 
 *  \param bt_addr Bluetooth target device address. 
 *  \param lt_addr Logical transport address. Between 0x00 and 0x07. 
 *  \return TRUE on success, else FALSE. 
 * 
 * \ingroup trapset_test2
 */
bool Test2TxData(HopChannels * channel, uint16 power, uint16 hopping, uint16 payload, uint16 packet_type, uint16 packet_length, const bdaddr * bt_addr, uint16 lt_addr);

/**
 *  \brief Starts to receive the specified data on the specified channels. 
 *  \param channel               This is an array of five channels to hop between.
 *               To receive on only one channel, write the same channel five times.
 *               Channel number between 0 and 78.
 *             
 *  \param hopping Allows or disallows the hop. See Test2TxData. 
 *  \param payload Bits pattern sent. See Test2TxData. 
 *  \param packet_type Type of packet sent. See Test2TxData. 
 *  \param packet_length Length of the packet, in bytes. Depending of the packet_type. 
 *  \param bt_addr Bluetooth target device address. 
 *  \param lt_addr Logical transport address. Between 0x00 and 0x07. 
 *  \return TRUE on success, else FALSE. 
 * 
 * \ingroup trapset_test2
 */
bool Test2RxStart(HopChannels * channel, uint16 hopping, uint16 payload, uint16 packet_type, uint16 packet_length, const bdaddr * bt_addr, uint16 lt_addr);
#endif /* TRAPSET_TEST2 */
#endif
