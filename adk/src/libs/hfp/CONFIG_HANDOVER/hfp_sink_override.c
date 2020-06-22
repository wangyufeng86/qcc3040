/****************************************************************************
Copyright (c) 2019 Qualcomm Technologies International, Ltd.

FILE NAME
    hfp_sink_override.c

DESCRIPTION
    Allows Audio Sink to be overridden on a given link.
    

NOTES
    Builds requiring this should include CONFIG_HANDOVER in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_HANDOVER
*/


/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_private.h"
#include "hfp_link_manager.h"

#include <panic.h>
#include <sink.h>

/****************************************************************************
NAME    
    HfpOverideSinkBdaddr

DESCRIPTION
    Overrides the Audio Sink of the specified link and 'stitches' 
    it in. I.e. Initializes the DM Sync connection context for
    the relevant SCO handle, stitch the SCO sink and the task
    and configure SCO sink messages.

    bd_addr address of the link to override the audio sink
    sink new sink.

RETURNS
    TRUE if sink successfully changed.
*/
bool HfpOverideSinkBdaddr(const bdaddr *bd_addr, Sink sink)
{
    /* Get the link */
    hfp_link_data* link = hfpGetLinkFromBdaddr(bd_addr);
    
    if(link)
    {
        uint16 scoHandle;
        /* Update the Audio Sink */
        link->audio_sink = sink;
        /* Initialize the connection context for the relevant connection id */        
        scoHandle = SinkGetScoHandle(link->audio_sink);
        PanicZero(scoHandle);  /* Invalid SCO Handle */
        PanicFalse(VmOverrideSyncConnContext(scoHandle, (conn_context_t)&theHfp->task));
        return TRUE;
    }

    return FALSE;
}

