/*!
\copyright  Copyright (c) 2005 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       sdp.c
\brief	    SDP definitions and help functions
*/

#include <region.h>
#include <service.h>
#include <bdaddr.h>
#include <panic.h>

#include "sdp.h"
#include "bt_device.h"

/*! Offset of the peer Bluetooth address in a sink service record */
#define PEER_BDADDR_OFFSET (55)

static const uint8 tws_sink_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm TWS+ Sink */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_TWS_SINK_SERVICE),

    /*  BluetoothProfileDescriptorList (0x0009) */
    /* 22 */    SDP_ATTR_ID(UUID_BT_PROFILE_DESCRIPTOR_LIST),
    /* 25 */    SDP_DATA_EL_SEQ(22),
    /* 27 */        SDP_DATA_EL_SEQ(20),

    /* 29 */            SDP_DATA_EL_UUID128(UUID_TWS_PROFILE_SERVICE),

                        /* Version */
    /* 46 */            SDP_DATA_EL_UINT16(DEVICE_TWS_VERSION),

                /*  Bluetooth Address(0x0200) */
    /* 49 */    SDP_ATTR_ID(UUID_PEER_BDADDR),
    /* 52 */    SDP_DATA_EL_SEQ(10),

                    /* NAP */
    /* 54 */        SDP_DATA_EL_UINT16(0xA5A5),

                    /* UAP */
    /* 57 */        SDP_DATA_EL_UINT8(0x00),

                    /* LAP */
    /* 59 */        SDP_DATA_EL_UINT32(0x000000),

};

const uint8 *appSdpGetTwsSinkServiceRecord(void)
{
    return tws_sink_service_record;
}

uint16 appSdpGetTwsSinkServiceRecordSize(void)
{
    return sizeof(tws_sink_service_record);
}


static const uint8 tws_sink_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_TWS_SINK_SERVICE)
};

const uint8 *appSdpGetTwsSinkServiceSearchRequest(void)
{
    return tws_sink_service_search_request;
}

uint16 appSdpGetTwsSinkServiceSearchRequestSize(void)
{
    return sizeof(tws_sink_service_search_request);
}

/* TWS+ Sink attribute search request */
static const uint8 tws_sink_attribute_list[] =
{
     SDP_DATA_EL_SEQ(3),                               /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_BT_PROFILE_DESCRIPTOR_LIST),  /* Current Version Attribute ID */
};

const uint8 *appSdpGetTwsSinkAttributeSearchRequest(void)
{
    return tws_sink_attribute_list;
}

uint16 appSdpGetTwsSinkAttributeSearchRequestSize(void)
{
    return sizeof(tws_sink_attribute_list);
}



/* TWS+ Source UUID search request */
#ifndef DISABLE_TWS_PLUS
static const uint8 tws_source_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
       SDP_DATA_EL_UUID128(UUID_TWS_SOURCE_SERVICE)
};
#endif

const uint8 *appSdpGetTwsSourceServiceSearchRequest(void)
{
#ifdef DISABLE_TWS_PLUS
    Panic();
    return NULL;
#else
    return tws_source_service_search_request;
#endif
}

uint16 appSdpGetTwsSourceServiceSearchRequestSize(void)
{
#ifdef DISABLE_TWS_PLUS
    Panic();
    return 0;
#else
    return sizeof(tws_source_service_search_request);
#endif
}



/* TWS+ Source attribute search request */
static const uint8 tws_source_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_BT_PROFILE_DESCRIPTOR_LIST),  /* Current Version Attribute ID */
};

const uint8 *appSdpGetTwsSourceAttributeSearchRequest(void)
{
#ifdef DISABLE_TWS_PLUS
    Panic();
#endif
    return tws_source_attribute_list;
}

uint16 appSdpGetTwsSourceAttributeSearchRequestSize(void)
{
#ifdef DISABLE_TWS_PLUS
    Panic();
#endif
    return sizeof(tws_source_attribute_list);
}



bool appSdpFindTwsVersion(const uint8 *ptr, const uint8 *end, uint16 *tws_version)
{
    static const uint8 tws_profile_uuid[] = {UUID_TWS_PROFILE_SERVICE};
    ServiceDataType type;
    Region record, protocols, protocol;
    Region value;

    PanicNull(tws_version);

    record.begin = ptr;
    record.end   = end;

    while (ServiceFindAttribute(&record, UUID_BT_PROFILE_DESCRIPTOR_LIST, &type, &protocols))
    {
        if(type == sdtSequence)
        {
            while (ServiceGetValue(&protocols, &type, &protocol))
            {
                if (type == sdtSequence)
                {
                    if (ServiceGetValue(&protocol, &type, &value))
                    {
                        if (type == sdtUUID)
                        {
                            if (RegionMatchesUUID128(&value, tws_profile_uuid))
                            {
                                if (ServiceGetValue(&protocol, &type, &value))
                                {
                                    if (type == sdtUnsignedInteger)
                                    {
                                        *tws_version = (uint16)RegionReadUnsigned(&value);
                                        return TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return FALSE; /* Failed */
}


void appSdpSetTwsSinkServiceRecordPeerBdAddr(uint8 *record, const bdaddr *bd_addr)
{
    record[PEER_BDADDR_OFFSET + 0] = (bd_addr->nap >> 8) & 0xFF;
    record[PEER_BDADDR_OFFSET + 1] = (bd_addr->nap >> 0) & 0xFF;
    record[PEER_BDADDR_OFFSET + 3] = (bd_addr->uap) & 0xFF;
    record[PEER_BDADDR_OFFSET + 5] = 0x00;
    record[PEER_BDADDR_OFFSET + 6] = (bd_addr->lap >> 16) & 0xFF;
    record[PEER_BDADDR_OFFSET + 7] = (bd_addr->lap >> 8) & 0xFF;
    record[PEER_BDADDR_OFFSET + 8] = (bd_addr->lap >> 0) & 0xFF;
}



static const uint8 sco_fwd_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm SCO Forwarding (0000eb03-d102-11e1-9b23-00025b00a5a5) */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_SCO_FWD_SERVICE),

    /* 22 */    SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
    /* 25 */        SDP_DATA_EL_SEQ(8),
    /* 27 */            SDP_DATA_EL_SEQ(6),
    /* 29 */                SDP_DATA_EL_UUID16(UUID16_L2CAP),
    /* 32 */                SDP_DATA_EL_UINT16(0x9999),

    /* 35 */    SDP_ATTR_ID(UUID_BT_SUPPORTED_FEATURES),
    /* 38 */        SDP_DATA_EL_UINT16(0x0000)
};

void appSdpSetScoFwdPsm(uint8 *record, uint16 psm)
{
    record[33 + 0] = (psm >> 8) & 0xFF;
    record[33 + 1] = (psm >> 0) & 0xFF;
}

void appSdpSetScoFwdFeatures(uint8 *record, uint16 features)
{
    record[39 + 0] = (features >> 8) & 0xFF;
    record[39 + 1] = (features >> 0) & 0xFF;
}

const uint8 *appSdpGetScoFwdServiceRecord(void)
{
    return sco_fwd_service_record;
}

uint16 appSdpGetScoFwdServiceRecordSize(void)
{
    return sizeof(sco_fwd_service_record);
}


/* TWS+ Source attribute search request */
static const uint8 sco_fwd_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_SCO_FWD_SERVICE),
};

const uint8 *appSdpGetScoFwdServiceSearchRequest(void)
{
    return sco_fwd_service_search_request;
}

uint16 appSdpGetScoFwdServiceSearchRequestSize(void)
{
    return sizeof(sco_fwd_service_search_request);
}


/* TWS+ Source attribute search request */
static const uint8 sco_fwd_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
};


const uint8 *appSdpGetScoFwdAttributeSearchRequest(void)
{
    return sco_fwd_attribute_list;
}

uint16 appSdpGetScoFwdAttributeSearchRequestSize(void)
{
    return sizeof(sco_fwd_attribute_list);
}

/* TWS+ Source attribute search request */
static const uint8 sco_fwd_features_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_BT_SUPPORTED_FEATURES),
};

const uint8 *appSdpGetScoFwdFeaturesAttributeSearchRequest(void)
{
    return sco_fwd_features_attribute_list;
}

uint16 appSdpGetScoFwdFeaturesAttributeSearchRequestSize(void)
{
    return sizeof(sco_fwd_features_attribute_list);
}


bool appSdpGetScoFwdSupportedFeatures(const uint8 *begin, const uint8 *end, uint16 *supported_features)
{
    ServiceDataType type;
    Region record, value;
    record.begin = begin;
    record.end   = end;

    while (ServiceFindAttribute(&record, UUID_BT_SUPPORTED_FEATURES, &type, &value))
    {
        if (type == sdtUnsignedInteger)
        {
            *supported_features = (uint16)RegionReadUnsigned(&value);
            return TRUE;
        }
    }

    return FALSE;
}

/* Offset of the PSM in the Peer Signalling service record */
#define L2CAP_PSM_OFFSET (33)

static const uint8 peer_sig_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm Peer Signalling (0000eb04-d102-11e1-9b23-00025b00a5a5) */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_PEER_SYNC_SERVICE),

    /* 22 */    SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
    /* 25 */        SDP_DATA_EL_SEQ(8),
    /* 27 */            SDP_DATA_EL_SEQ(6),
    /* 29 */                SDP_DATA_EL_UUID16(UUID16_L2CAP),
    /* 32 */                SDP_DATA_EL_UINT16(0x9999)
};

void appSdpSetPeerSigPsm(uint8 *record, uint16 psm)
{
    record[L2CAP_PSM_OFFSET + 0] = (psm >> 8) & 0xFF;
    record[L2CAP_PSM_OFFSET + 1] = (psm >> 0) & 0xFF;
}

const uint8 *appSdpGetPeerSigServiceRecord(void)
{
    return peer_sig_service_record;
}

uint16 appSdpGetPeerSigServiceRecordSize(void)
{
    return sizeof(peer_sig_service_record);
}

/* Peer signalling service search request */
static const uint8 peer_sig_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_PEER_SYNC_SERVICE),
};

const uint8 *appSdpGetPeerSigServiceSearchRequest(void)
{
    return peer_sig_service_search_request;
}

uint16 appSdpGetPeerSigServiceSearchRequestSize(void)
{
    return sizeof(peer_sig_service_search_request);
}


/* Peer signalling attribute search request */
static const uint8 peer_sig_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
};


const uint8 *appSdpGetPeerSigAttributeSearchRequest(void)
{
    return peer_sig_attribute_list;
}

uint16 appSdpGetPeerSigAttributeSearchRequestSize(void)
{
    return sizeof(peer_sig_attribute_list);
}

#ifdef INCLUDE_DFU_PEER

static const uint8 device_upgrade_peer_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm SCO Forwarding (0000eb05-d102-11e1-9b23-00025b00a5a5) */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_DEVICE_UPGRADE_PEER_SERVICE),

    /* 22 */    SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
    /* 25 */        SDP_DATA_EL_SEQ(8),
    /* 27 */            SDP_DATA_EL_SEQ(6),
    /* 29 */                SDP_DATA_EL_UUID16(UUID16_L2CAP),
    /* 32 */                SDP_DATA_EL_UINT16(0x9999),

    /* 35 */    SDP_ATTR_ID(UUID_BT_SUPPORTED_FEATURES),
    /* 38 */        SDP_DATA_EL_UINT16(0x0000)
};


void appSdpSetDeviceUpgradePeerPsm(uint8 *record, uint16 psm)
{
    record[L2CAP_PSM_OFFSET + 0] = (psm >> 8) & 0xFF;
    record[L2CAP_PSM_OFFSET + 1] = (psm >> 0) & 0xFF;
}

uint16 appSdpGetDeviceUpgradePeerServiceRecordSize(void)
{
    return sizeof(device_upgrade_peer_service_record);
}

const uint8 *appSdpGetDeviceUpgradePeerServiceRecord(void)
{
    return device_upgrade_peer_service_record;
}

/* TWS+ Source attribute search request */
static const uint8 device_upgrade_peer_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_DEVICE_UPGRADE_PEER_SERVICE),
};

const uint8 *appSdpGetDeviceUpgradePeerServiceSearchRequest(void)
{
    return device_upgrade_peer_service_search_request;
}

uint16 appSdpGetDeviceUpgradePeerServiceSearchRequestSize(void)
{
    return sizeof(device_upgrade_peer_service_search_request);
}


/* TWS+ Source attribute search request */
static const uint8 device_upgrade_peer_fwd_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
};


const uint8 *appSdpGetDeviceUpgradePeerAttributeSearchRequest(void)
{
    return device_upgrade_peer_fwd_attribute_list;
}

uint16 appSdpGetDeviceUpgradePeerAttributeSearchRequestSize(void)
{
    return sizeof(device_upgrade_peer_fwd_attribute_list);
}

#endif

/* Offset of the PSM in the Handover Profile service record */
#define HANDOVER_PROFILE_L2CAP_PSM_OFFSET (33)

/* Handover Profile service record */
static const uint8 handover_profile_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm Handover Profile (0000eb06-d102-11e1-9b23-00025b00a5a5) */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_HANDOVER_PROFILE_SERVICE),

    /* 22 */    SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
    /* 25 */        SDP_DATA_EL_SEQ(8),
    /* 27 */            SDP_DATA_EL_SEQ(6),
    /* 29 */                SDP_DATA_EL_UUID16(UUID16_L2CAP),
    /* Place holder created to add the PSM value during SDP search */
    /* 32 */                SDP_DATA_EL_UINT16(0x9999)
};

/* Handover profile service search request */
static const uint8 handover_profile_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_HANDOVER_PROFILE_SERVICE),
};

/* Handover profile attribute search request */
static const uint8 handover_profile_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
};

/*! \brief Set Handover Profile L2CAP PSM into service record.

    \param[in] record       Pointer to SDP service record.
    \param[in] psm          PSM of Handover profile.
*/
void sdp_SetHandoverProfilePsm(uint8 *record, uint16 psm)
{
    record[HANDOVER_PROFILE_L2CAP_PSM_OFFSET + 0] = (psm >> 8) & 0xFF;
    record[HANDOVER_PROFILE_L2CAP_PSM_OFFSET + 1] = (psm >> 0) & 0xFF;
}

/*! \brief Get a pointer to the service record of Handover Profile.

    \returns A pointer to the service record of Handover Profile.
*/
const uint8 *sdp_GetHandoverProfileServiceRecord(void)
{
    return handover_profile_service_record;
}

/*! \brief Get the size of the Handover Profile service record.

    \returns Size of Handover Profile service record.
*/
uint16 sdp_GetHandoverProfileServiceRecordSize(void)
{
    return sizeof(handover_profile_service_record);
}

/*! \brief Get a pointer to an SDP search record that can be used to find Handover Profile.

    The pointer returned can be passed to ConnectionSdpServiceSearchAttributeRequest
    as the search_pattern parameter.

    \returns A pointer to an SDP search record that can be used to find a Handover Profile.
 */
const uint8 *sdp_GetHandoverProfileServiceSearchRequest(void)
{
    return handover_profile_service_search_request;
}

/*! \brief Get the size of Handover Profile search request record.

    \returns The size of Handover Profile search request record.
*/
uint16 sdp_GetHandoverProfileServiceSearchRequestSize(void)
{
    return sizeof(handover_profile_service_search_request);
}

/*! \brief Gets pointer to an attribute search record that can be used to find Handover Profile.

    The pointer returned can be passed to ConnectionSdpServiceSearchAttributeRequest
    as the search_pattern parameter.

    \returns A pointer to an attribute search record that can be used to find Handover Profile.
*/
const uint8 *sdp_GetHandoverProfileAttributeSearchRequest(void)
{
    return handover_profile_attribute_list;
}

/*! \brief Gets the size of attribute search request record.

    \returns The size of Handover Profile attribute search request record.
*/
uint16 sdp_GetHandoverProfileAttributeSearchRequestSize(void)
{
    return sizeof(handover_profile_attribute_list);
}

/* Offset of the PSM in the Mirror Profile service record */
#define MIRROR_PROFILE_L2CAP_PSM_OFFSET (33)

/* Mirror Profile service record */
static const uint8 mirror_profile_service_record[] =
{
   /* Offset */ /* ServiceClassIDList(0x0001), Data Element Sequence */
    /*  0 */    SDP_ATTR_ID(UUID_SERVICE_CLASS_ID_LIST),
    /*  3 */        SDP_DATA_EL_SEQ(17),

    /*  UUID Qualcomm Mirror Profile (0000eb07-d102-11e1-9b23-00025b00a5a5) */
    /*  5 */        SDP_DATA_EL_UUID128(UUID_MIRROR_PROFILE_SERVICE),

    /* 22 */    SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
    /* 25 */        SDP_DATA_EL_SEQ(8),
    /* 27 */            SDP_DATA_EL_SEQ(6),
    /* 29 */                SDP_DATA_EL_UUID16(UUID16_L2CAP),
    /* Place holder created to add the PSM value during SDP search */
    /* 32 */                SDP_DATA_EL_UINT16(0x9999)
};

/* Mirror profile service search request */
static const uint8 mirror_profile_service_search_request[] =
{
    SDP_DATA_EL_SEQ(17),                     /* type = DataElSeq, 17 bytes in DataElSeq */
        SDP_DATA_EL_UUID128(UUID_MIRROR_PROFILE_SERVICE),
};

/* Mirror profile attribute search request */
static const uint8 mirror_profile_attribute_list[] =
{
    SDP_DATA_EL_SEQ(3),                                /* Data Element Sequence of 3 */
        SDP_ATTR_ID(UUID_PROTOCOL_DESCRIPTOR_LIST),
};


/*! \brief Set Mirror Profile L2CAP PSM into service record.

    \param[in] record       Pointer to SDP service record.
    \param[in] psm          PSM of Mirror profile.
*/
void appSdpSetMirrorProfilePsm(uint8 *record, uint16 psm)
{
    record[MIRROR_PROFILE_L2CAP_PSM_OFFSET + 0] = (psm >> 8) & 0xFF;
    record[MIRROR_PROFILE_L2CAP_PSM_OFFSET + 1] = (psm >> 0) & 0xFF;
}

/*! \brief Get a pointer to the service record of Mirror Profile.

    \returns A pointer to the service record of Mirror Profile.
*/
const uint8 *appSdpGetMirrorProfileServiceRecord(void)
{
    return mirror_profile_service_record;
}

/*! \brief Get the size of the Mirror Profile service record.

    \returns Size of Mirror Profile service record.
*/
uint16 appSdpGetMirrorProfileServiceRecordSize(void)
{
    return sizeof(mirror_profile_service_record);
}

/*! \brief Get a pointer to an SDP search record that can be used to find Mirror Profile.

    The pointer returned can be passed to ConnectionSdpServiceSearchAttributeRequest
    as the search_pattern parameter.

    \returns A pointer to an SDP search record that can be used to find a Mirror Profile.
 */
const uint8 *appSdpGetMirrorProfileServiceSearchRequest(void)
{
    return mirror_profile_service_search_request;
}

/*! \brief Get the size of Mirror Profile search request record.

    \returns The size of Mirror Profile search request record.
*/
uint16 appSdpGetMirrorProfileServiceSearchRequestSize(void)
{
    return sizeof(mirror_profile_service_search_request);
}

/*! \brief Gets pointer to an attribute search record that can be used to find Mirror Profile.

    The pointer returned can be passed to ConnectionSdpServiceSearchAttributeRequest
    as the search_pattern parameter.

    \returns A pointer to an attribute search record that can be used to find Mirror Profile.
*/
const uint8 *appSdpGetMirrorProfileAttributeSearchRequest(void)
{
    return mirror_profile_attribute_list;
}

/*! \brief Gets the size of attribute search request record.

    \returns The size of Mirror Profile attribute search request record.
*/
uint16 appSdpGetMirrorProfileAttributeSearchRequestSize(void)
{
    return sizeof(mirror_profile_attribute_list);
}

#ifdef INCLUDE_DEVICE_TEST_SERVICE

static const uint8 spp_dts_service_record[] = {
            0x09, 0x00, 0x01,   // ServiceClassIDList(0x0001)
            0x35, 0x03,         // DataElSeq 3 bytes
            0x19, 0x11, 0x01,   // UUID SerialPort(0x1101)

            0x09, 0x00, 0x04,   // ProtocolDescriptorList(0x0004)
            0x35, 0x0c,         // DataElSeq 12 bytes
            0x35, 0x03,         // DataElSeq 3 bytes
            0x19, 0x01, 0x00,   // UUID L2CAP(0x0100)
            0x35, 0x05,         // DataElSeq 5 bytes
            0x19, 0x00, 0x03,   // UUID RFCOMM(0x0003)
            0x08, 0x00,         // uint8 0x00 (should be populated before use)

            0x09, 0x00, 0x06,   // LanguageBaseAttributeIDList(0x0006)
            0x35, 0x09,         // DataElSeq 9 bytes
            0x09,  'e',  'n',   // uint16 en
            0x09, 0x00, 0x6a,   // uint16 0x006a
            0x09, 0x01, 0x00,   // uint16 0x0100

            0x09, 0x00, 0x09,   /* BluetoothProfileDescriptorList(0x0009) */
            0x35, 0x08,         /* DataElSeq 8 bytes [The list] */
            0x35, 0x06,         /* DataElSeq 6 bytes [The list item] */
            0x19, 0x11, 0x01,   /* UUID SerialPort(0x1101) */
            0x09, 0x01, 0x02,   /* SerialPort Version (0x0102) */

            0x09, 0x01, 0x00,   // ServiceName(0x0100) = "DTS"
            0x25, 0x03,         // String length 3
            'D', 'T', 'S',
            };

const uint8 *sdp_GetDeviceTestServiceServiceRecord(uint16 *length)
{
    *length = sizeof(spp_dts_service_record);
    return spp_dts_service_record;
}

#endif

