/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

#include <bdaddr.h>
#include <app/bluestack/dm_prim.h>

void BdaddrTpFromTypedAndFlags(tp_bdaddr* out, const typed_bdaddr* in, const uint16 flags)
{
    if(out && in)
    {
        out->taddr = *in;
        
        if((flags & DM_ACL_FLAG_ULP) == DM_ACL_FLAG_ULP)
            out->transport = TRANSPORT_BLE_ACL;
        else
            out->transport = TRANSPORT_BREDR_ACL;
    }
}
