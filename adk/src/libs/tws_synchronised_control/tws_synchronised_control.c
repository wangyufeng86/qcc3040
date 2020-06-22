/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.


FILE NAME
    tws_synchronised_control.c
*/

#include <vmtypes.h>

#include "tws_synchronised_control.h"

void twsSynchronisedControlEnable(Sink avrcp_sink)
{
    UNUSED(avrcp_sink);
}

void twsSynchronisedControlDisable(void)
{

}

bool twsSynchronisedControlIsEnabled(void)
{
    return FALSE;
}

tws_timestamp_t twsSynchronisedControlGetFutureTimestamp(uint16 milliseconds_in_future)
{
    UNUSED(milliseconds_in_future);
    return 0;
}

int32 twsSynchronisedControlConvertTimeStampToMilliseconds(tws_timestamp_t timestamp)
{
    UNUSED(timestamp);
    return 0;
}
