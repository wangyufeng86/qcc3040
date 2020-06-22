/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Main application header file

\mainpage   Earbuds application
 
Legal information
=================
<CENTER><b>For additional information or to submit technical questions, go to: https://createpoint.qti.qualcomm.com</b></CENTER>

<CENTER><b>Confidential and Proprietary – Qualcomm Technologies International, Ltd.</b></CENTER>

<b>NO PUBLIC DISCLOSURE PERMITTED:</b> Please report postings of this document on public servers or websites to: DocCtrlAgent@qualcomm.com.

<b>Restricted Distribution:</b> Not to be distributed to anyone who is not an employee of either Qualcomm Technologies International, Ltd. or its affiliated companies without the express approval of Qualcomm Configuration Management.

Not to be used, copied, reproduced, or modified in whole or in part, nor its contents revealed in any manner to others without the express written permission of Qualcomm Technologies International, Ltd.

All Qualcomm products mentioned herein are products of Qualcomm Technologies, Inc. and/or its subsidiaries.

Qualcomm is a trademark of Qualcomm Technologies Incorporated, registered in the United States and other countries. Other product and brand names may be trademarks or registered trademarks of their respective owners.
This technical data may be subject to U.S. and international export, re-export, or transfer ("export") laws. Diversion contrary to U.S. and international law is strictly prohibited.

Qualcomm Technologies International, Ltd. (formerly known as Cambridge Silicon Radio Limited) is a company registered in England and Wales with a registered office at: Churchill House, Cambridge Business Park, Cowley Road, Cambridge, CB4 0WZ, United Kingdom. Registered Number: 3665875 | VAT number: GB787433096.

<CENTER><b>© 2018 Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.</b></CENTER>

Overview
========
This is a working sample application for earbuds.

The application is usable as delivered, but expectation is that
customers will adapt the application. Options for doing this
include
\li Build defines to control features included
\li Changes to the configuration (\ref earbud_config.h)
\li Software changes

            @startuml

            class init << (T,Green) >> #paleGreen {
                Initialise the application
                ....
                Initialises all other modules
                at startup
            }
            class led << (T,lightblue) >> {
                Manages the LEDs
                }
            class hfp << (T,lightblue) >> #Orange {
                Manages HFP connections
                }
            class ui << (T,lightblue) >> #Orange {
                Manages the UI
                }
            class sm << (T,lightblue) >> #lightBlue {
                Main application state machine
                }
            class link_policy << (T,lightblue) >> {
                Manages the device link policy settings
                }
            class av << (T,lightblue) >> #Orange {
                Manages A2DP and AVRCP connections
                }
            class charger << (T,lightblue) >> {
                Manages the charger
                }
            class battery << (T,lightblue) >> {
                Manages the battery status
                }
            class temperature << (T,lightblue) >> {
                Temperature measurement
                }
            class power << (T,lightblue) >> #Orange {
                Manages power, shutdown, sleep
                }
            class pairing << (T,lightblue) >> #Orange {
                Manages peer and handset pairing
                }
            class scanning << (T,lightblue) >> {
                Manages the device scanning state
                }
            class device << (T,lightblue) >> {
                Paired devices database
                }
            class con_manager << (T,lightblue) >> {
                Manages the device connection state machine
                }
            class peer_sig << (T,lightblue) >> {
                Manages peer signalling messages
                }
            class handset_sig << (T,lightblue) >> {
                Manages the handset signalling messages
                }
            class phy_state << (T,lightblue) >> #Orange {
                Manages physcial state of the Earbud
                }
            class kymera << (T,lightblue) >> {
                Manages the audio subsystem
                }
            class upgrade << (T,red) >> #lightGrey {
                Manages the device upgrade
                }
            class proximity << (T,lightblue) >> {
                Manages the proximity sensor
                }
            class accelerometer << (T,lightblue) >> {
                Manages the accelerometer
                }
            class rules << (.,red) >> #yellow {
                Application rules engine
                }
            class chains << (.,red) >> #pink {
                User defined audio chains
                }
            class buttons << (.,red) >> #pink {
                User defined button inputs
                }
            class peer_sync << (T,lightblue) >> {
                Peer Earbud state synchronisation
                }
            class scofwd << (T,lightblue) >> {
                SCO and MIC forwarding
                }

            class A << (T,lightblue) >> {
            }

            class B << (T,lightblue) >> {
            }

            init -[hidden]right->upgrade
            upgrade -[hidden]right->device
                        device -[hidden]right-> A
                        A -[hidden]d-> charger

                        A -right-> B : Function Call
                        A -right[#blue]-> B : Registered Event
                        A <-right[#green]-> B : Function and Registered Event

                        rules-d[#blue]->sm : Actions
                        sm -u-> rules : Events
                        chains-l->kymera
                        buttons-l->ui

                        ' Physical Inputs
                        temperature -d[#blue]-> power : temperature status
                        temperature -r[#blue]-> charger : temperature status
                        charger -d[#blue]-> phy_state : charger status (in/out case)
                        'charger -[hidden]r->accelerometer
                        battery -[hidden]r->temperature
                        charger -d[#blue]-> power : charger status
                        battery -d[#blue]-> power : battery status
                        proximity -d[#blue]-> phy_state : in/out ear
                        proximity -[hidden]r->accelerometer
                        accelerometer -d[#blue]-> phy_state :  motion status
                        power -d[#blue]-> sm : power events
                        phy_state -d[#blue]-> sm : phy state
                        phy_state -[#blue]-> handset_sig : phy state

                        'UI control
                        sm -r-> ui : ui indications
                        ui -u-> led : enable/disable
                        ui -d-> kymera : tones

                        'Registered con_manager tasks
                        con_manager -[#blue]> sm : con status
                        con_manager -r[hidden]-> link_policy
                        av <-[#green]-> con_manager : con/dis/status
                        hfp <-[#green]-> con_manager : con/dis/status
                        peer_sig -r-> con_manager : con/dis

                        sm -l-> pairing : Pair handset and earbud
                        pairing -d-> scanning : enable/disable
                        sm -d-> scanning : enable/disable

                        sm <-d[#green]-> hfp : con/dis/status
                        hfp -d[#blue]-> scofwd : hfp status
                        scofwd -r-> kymera : voice call
                        hfp <-u[#green]-> handset_sig : con/dis/sig
                        hfp -d-> link_policy

                        sm <-d[#green]-> av : con/dis/status
                        av <-u[#green]-> handset_sig : con/dis/sig
                        av -d-> link_policy
                        av -r-> kymera : stereo music

                        peer_sig -d[#blue]-> sm
                        peer_sig -u-> scanning : enable/disable

                        peer_sync -r[hidden]-> pairing
                        peer_sync -r[#blue]->sm : sync status
                        peer_sync <-d[#green]-> peer_sig : peer sync sig

                        sm -d-> scofwd : con/dis
                        scofwd -l-> peer_sig : scofwd sig
                        scofwd -u-> link_policy

                        @enduml
 
*/

#ifndef EARBUD_DOC_H_
#define EARBUD_DOC_H_


#endif /* EARBUD_DOC_H_ */
