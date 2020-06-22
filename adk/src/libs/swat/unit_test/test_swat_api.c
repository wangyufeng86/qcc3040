/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 
*******************************************************************************/

#include "unity.h"
#include <stdlib.h>
#include "test_swat_api.h"
#include "../swat_api.h"
#include "../swat_private.h"

#define SWAT_MAX_REMOTE_DEVICES_DEFAULT 2

/* Called by RUN_TEST macro before actual test function is invoked */
static void setUp (void)
{
}

/* Called by RUN_TEST macro after actual test function has completed */
static void tearDown (void)
{
}


/*************************************************************************
NAME    
    test_SwatGetMediaState
        
DESCRIPTION
    Test SwatGetMediaState function

PRE-CONDITIONS
    swat Media State has been set to known value
    
POST-CONDITIONS
    
    
**************************************************************************/
static void test_SwatGetMediaState(void)
{
    uint16 dev;

    for (dev=0; dev< SWAT_MAX_REMOTE_DEVICES_DEFAULT; dev++)
    {
        /* swat_media_closed */
        swat->remote_devs[dev].media_state = swat_media_closed;
        TEST_ASSERT_EQUAL(0, SwatGetMediaState(dev));
        
        /* swat_media_opening */
        swat->remote_devs[dev].media_state = swat_media_opening;
        TEST_ASSERT_EQUAL(1, SwatGetMediaState(dev));

        /* swat media closing */
        swat->remote_devs[dev].media_state = swat_media_closing;
        TEST_ASSERT_EQUAL(6, SwatGetMediaState(dev));
        
        /*invalid value for state */
        swat->remote_devs[dev].media_state = 10;
        TEST_ASSERT_EQUAL(swat_media_invalid, SwatGetMediaState(dev));
        
        /*invalid value for state */
        swat->remote_devs[dev].media_state = 65535;
        TEST_ASSERT_EQUAL(swat_media_invalid, SwatGetMediaState(dev));
    }
    
}

/*************************************************************************
NAME    
    test_SwatGetMediaLLState
        
DESCRIPTION
    Test SwatGetMediaLLState function

PRE-CONDITIONS
    swat Media LL State has been set to known value
    
POST-CONDITIONS
    
    
**************************************************************************/
static void test_SwatGetMediaLLState(void)
{
    uint16 dev;
    
    for (dev=0; dev< SWAT_MAX_REMOTE_DEVICES_DEFAULT; dev++)
    {
        swat->remote_devs[dev].media_ll_state = swat_media_closed;
        TEST_ASSERT_EQUAL(0, SwatGetMediaLLState(dev));
        
        swat->remote_devs[dev].media_ll_state = swat_media_opening;
        TEST_ASSERT_EQUAL(1, SwatGetMediaLLState(dev));
        
        swat->remote_devs[dev].media_ll_state = swat_media_closing;
        TEST_ASSERT_EQUAL(6, SwatGetMediaLLState(dev));
        
        swat->remote_devs[dev].media_ll_state = 16;
        TEST_ASSERT_EQUAL(swat_media_invalid, SwatGetMediaLLState(dev));
        
        swat->remote_devs[dev].media_ll_state = 65535;
        TEST_ASSERT_EQUAL(swat_media_invalid, SwatGetMediaLLState(dev));
    }    
}

/*************************************************************************
NAME    
    test_swat_data_block
    
DESCRIPTION
    Test runner for swat library API module unit tests

PRE-CONDITIONS
    SWAT data structure has not been allocated
    
POST-CONDITIONS
    SWAT data structure is deallocated
    
**************************************************************************/
void test_swat_api (void)
{
    /* intialise memory for SWAT structure */
    swat = PanicUnlessNew(swatTaskData);
    memset( swat, 0, sizeof(swatTaskData) );
    
    /* initialise memory for remote devices */
    swat->remote_devs = (remoteDevice *)PanicNull(malloc(SWAT_MAX_REMOTE_DEVICES_DEFAULT * sizeof(remoteDevice)));
    memset(swat->remote_devs, 0, (SWAT_MAX_REMOTE_DEVICES_DEFAULT * sizeof(remoteDevice)));
        
        
    UNITY_BEGIN();

    RUN_TEST(test_SwatGetMediaState);
    RUN_TEST(test_SwatGetMediaLLState);
    
    
    UNITY_END();
    
    
    /* Release allocated swat structures */
    free(swat->remote_devs);
    free(swat);
}
