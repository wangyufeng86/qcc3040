/*!
\copyright  Copyright (c) 2005 - 2020 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief	    Header file for the production test mode
*/

#ifndef EARBUD_PRODUCTION_TEST_H_
#define EARBUD_PRODUCTION_TEST_H_

#ifdef PRODUCTION_TEST_MODE

/*! \brief Handle request to enter FCC test mode.
*/
void appSmHandleInternalEnterProductionTestMode(void);

/*! \brief Handle request to enter DUT test mode.
*/
void appSmHandleInternalEnterDUTTestMode(void);

/*! \brief Request To enter Production Test mode
*/
void appSmEnterProductionTestMode(void);

#endif /*PRODUCTION_TEST_MODE*/

#endif /*EARBUD_PRODUCTION_TEST_H_*/
