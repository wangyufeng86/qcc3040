/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
\brief      Configuration / parameters used by the topology.
*/

#include "bredr_scan_manager.h"
#include "le_advertising_manager.h"

/*! Inquiry scan parameter set */
extern const bredr_scan_manager_parameters_t inquiry_scan_params;

/*! Page scan parameter set */
extern const bredr_scan_manager_parameters_t page_scan_params;

/*! LE Advertising parameter set */
extern const le_adv_parameters_t le_adv_params;

/*! Initial time for a peer find role command before notifying that a role
    has not yet been found. */
#define TwsTopologyConfig_InitialPeerFindRoleTimeoutS()     (3)

/*! Time for Secondary to wait for BR/EDR ACL connection to Primary following
    role selection, before falling back to retry role selection and potentially
    becoming an acting primary. */
#define TwsTopologyConfig_SecondaryPeerConnectTimeoutMs()   (5000)

/*! Time for Primary to wait for BR/EDR ACL connection to handset. */
#define TwsTopologyConfig_PrimaryHandsetConnectTimeoutMs()   (5000)

/*! Time for Primary to wait for BR/EDR ACL connection to be made by the Secondary
    following role selection, before falling back to retry role selection. */
#define TwsTopologyConfig_PrimaryPeerConnectTimeoutMs()     (10240)

/*! Time for Handover to be retried following a previous handover attempt. */
#define TwsTopologyConfig_HandoverRetryTimeoutMs()     (200)

/*! maximum number of retry attempts for Handover upon a handover imeout */
#define TwsTopologyConfig_HandoverMaxRetryAttempts()     (20)

/*! Configure accessor for dynamic handover support */
#ifdef ENABLE_DYNAMIC_HANDOVER
#define TwsTopologyConfig_DynamicHandoverSupported()    (TRUE)
#else
#define TwsTopologyConfig_DynamicHandoverSupported()    (FALSE)
#endif


/*! State proxy events to register */
#if defined(ENABLE_DYNAMIC_HANDOVER)

#if defined(INCLUDE_HDMA_MIC_QUALITY_EVENT) && defined(INCLUDE_HDMA_RSSI_EVENT)
#define TwsTopologyConfig_StateProxyRegisterEvents() (state_proxy_event_type_link_quality | state_proxy_event_type_mic_quality | state_proxy_event_type_sco_active | state_proxy_event_type_sco_inactive)
#elif defined(INCLUDE_HDMA_MIC_QUALITY_EVENT)
#define TwsTopologyConfig_StateProxyRegisterEvents() (state_proxy_event_type_mic_quality | state_proxy_event_type_sco_active | state_proxy_event_type_sco_inactive)
#elif defined(INCLUDE_HDMA_RSSI_EVENT)
#define TwsTopologyConfig_StateProxyRegisterEvents() state_proxy_event_type_link_quality
#else
#define TwsTopologyConfig_StateProxyRegisterEvents() 0
#endif

#else
#define TwsTopologyConfig_StateProxyRegisterEvents() 0
#endif


#ifdef INCLUDE_MIRRORING
#define TwsTopologyConfig_PeerProfiles() (DEVICE_PROFILE_PEERSIG | DEVICE_PROFILE_HANDOVER | DEVICE_PROFILE_MIRROR)
#else
#define TwsTopologyConfig_PeerProfiles() (DEVICE_PROFILE_A2DP | DEVICE_PROFILE_SCOFWD | DEVICE_PROFILE_PEERSIG)
#endif