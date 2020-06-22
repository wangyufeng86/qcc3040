/*!
\copyright  Copyright (c) 2018 - 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file
*/
#ifndef EARBUD_ANC_TUNING_H
#define EARBUD_ANC_TUNING_H


/*! Peer Sync module data. */
typedef struct
{
    TaskData task;                      /*!< Peer sync module Task. */
    unsigned enabled:1;
    unsigned active:1;
} ancTuningTaskData;

/*!< Task information for ANC tuning. */
extern ancTuningTaskData   app_anc_tuning;

/*! Get pointer to the ANC tuning data structure */
#define AncTuningGetTaskData()    (&app_anc_tuning)

extern void appAncTuningEarlyInit(void);
extern void appAncTuningEnable(bool enable);
extern void appAncTuningEnumerated(void);
extern void appAncTuningSuspended(void);


#endif // EARBUD_ANC_TUNING_H
