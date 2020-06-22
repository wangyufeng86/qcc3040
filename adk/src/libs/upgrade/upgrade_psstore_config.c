/****************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_psstore.c

DESCRIPTION

    Implementation of an interface to Persistent Storage to get
    details of the file system and anything else related to the
    possibilities of upgrade.

NOTES
    Errors. Cause panics. End of. This behaviour in itself is problematic
            but if we are trying to switch applications then an error
            should indicate a reason to restart. We can't really
            delegate this to the VM app. We can't wind back to a previous
            application.
    Caching. Persistent store keys are not cached. There isn't a mechanism
            to subscribe to PSKEY changes. Since we don't actually expect
            to be called that frequently it makes sense to access the keys
            we need when we need them.
*/


#include <stdlib.h>
#include <string.h>
#include <csrtypes.h>
#include <panic.h>
#include <ps.h>

#include <print.h>
#include <upgrade.h>

#include "upgrade_ctx.h"
#include "upgrade_fw_if.h"
#include "upgrade_psstore.h"
#include "upgrade_psstore_priv.h"
#include "upgrade_partitions.h"

/* details for accessing and storing the FSTAB from persistent store.
 *
 * we expect the FSTAB to consist of
 *      Entry 0 - partition for app to run on boot
 *      Entry ... - Other partitions
 *
 * The rest of the contents are not considered at present and
 * differences beyond the two application entries are ignored as they are
 * not important to upgrade. This would also allow for the application
 * revising the FS in future, by changing its size.
 *
 * As of writing, there is an issue with including the partition
 * to be used to store the upgraded app in the FSTAB. During boot all
 * partitions get mounted before checking if they are executable.
 * However, a mounted partition cannot be opened for writing, thus
 * blocking us from using any partition listed in the FSTAB for
 * storing the upgraded app in.
 *
 * So for now we workaround this by saying that the two app partitions
 * are "1001" and "1002" (SQIF partitions 1 and 2). The code below flips
 * the value of entry 0 in FSTAB to one or the other depending on which
 * partition the app is to be run from. This leaves the other partition
 * un-mounted and available for writing.
 */
#define PSKEY_FSTAB             (0x25E6)

#define VM_APP_INDEX_CURRENT  0

/* Minimum size is set as all known use cases of FSTAB require an
  entry of 0000. If that is the only entry (size 1) then there is 
  nothing for us to do so why bother loading. */
#define FSTAB_MINIMUM_SIZE    2

/* Utility functions */
static bool loadFstab(FSTAB_COPY *fstab,PsStores store);
static void loadUpgradeKey(UPGRADE_LIB_PSKEY *key_data, uint16 key, uint16 key_offset);


/****************************************************************************
NAME
    UpgradeLoadPSStore  -  Load PSKEY on boot

DESCRIPTION
    Save the details of the PSKEY and offset that we were passed on 
    initialisation, and retrieve the current values of the key.

    In the unlikely event of the storage not being found, we initialise
    our storage to 0x00 rather than panicking.
*/
void UpgradeLoadPSStore(uint16 dataPskey,uint16 dataPskeyStart)
{
    FSTAB_COPY  fstab;

    UpgradeCtxGet()->upgrade_library_pskey = dataPskey;
    UpgradeCtxGet()->upgrade_library_pskeyoffset = dataPskeyStart;
    
    loadUpgradeKey(UpgradeCtxGetPSKeys(), dataPskey, dataPskeyStart);

    /* Load the implementation FSTAB. */
    loadFstab(&fstab, ps_store_implementation);
}

/****************************************************************************
NAME
    loadFstab  -  Load FSTAB data from the requested store

DESCRIPTION
    Checks whether the store requested is one of those that we support, then
    finds the length of the allocated data (if any), and loads the data from 
    Persistent store into the supplied structure (fixed size).

RETURNS
    TRUE if store was supported and an entry was found.
*/
static bool loadFstab(FSTAB_COPY *fstab,PsStores store)
{
PsStores    old_store;

    if (   (store == ps_store_implementation)
        || (store == ps_store_transient))
    {
        /* Since we are a library, get the old persistent store
         * so we can restore it  */
        old_store = PsGetStore();
        PsSetStore(store);

        /* Find size of storage */
        fstab->length = PsFullRetrieve(PSKEY_FSTAB,NULL,0);
        if (   (FSTAB_MINIMUM_SIZE <= fstab->length) 
            && (fstab->length <= sizeof(fstab->ram_copy)))
        {
            fstab->length = PsFullRetrieve(PSKEY_FSTAB,fstab->ram_copy,fstab->length);
            PsSetStore(old_store);
            return TRUE;
        }
        PsSetStore(old_store);
    }

    return FALSE;
}


/****************************************************************************
NAME
    UpgradeSetToTryUpgrades

DESCRIPTION
    Swaps the application entries in the temporary FSTAB so that the upgraded
    partitions are used on the next reboot.

    Ensures that the new temporary FSTAB is otherwise a copy of the
    implementation version.

RETURNS
    TRUE if the temporary FSTAB was updated with changed values.
*/
bool UpgradeSetToTryUpgrades(void)
{
    FSTAB_COPY fstab;
    bool changed = FALSE;

    PRINT(("UpgradeSetToTryUpgrades\n"));

    if (loadFstab(&fstab, ps_store_implementation))
    {
        uint16 i;

        for (i = 0; i < fstab.length;i++)
        {
            uint16 old = fstab.ram_copy[i];
            fstab.ram_copy[i] = UpgradePartitionsNewPhysical(fstab.ram_copy[i]);
            PRINT(("FSTAB[%d] = %04x (was %04x)\n",i,fstab.ram_copy[i],old));

            if (fstab.ram_copy[i] != old)
            {
                changed = TRUE;
            }
        }

        if (!PsStoreFsTab(fstab.ram_copy,fstab.length,FALSE))
        {
            changed = FALSE;
        }
    }
    
    return changed;
}


/****************************************************************************
NAME
    UpgradeCommitUpgrades

DESCRIPTION
    Writes the implementation store entry of the FSTAB so that the correct
    partitions are used after a power cycle.


    @TODO: This code MAY NOT BE good enough to deal with all the errors that can 
            happen.  Now includes synchronisation with the partitions table, but
            ...

RETURNS
    n/a
*/
void UpgradeCommitUpgrades(void)
{
    FSTAB_COPY fstab;

    if (   loadFstab(&fstab,ps_store_transient)
        && PsStoreFsTab(fstab.ram_copy,fstab.length,TRUE))
    {
        UpgradePartitionsCommitUpgrade();
        return;
    }
}


/****************************************************************************
NAME
    UpgradeRevertUpgrades

DESCRIPTION
    Sets the transient store entry of the FSTAB to match the implementation so 
    that the original partitions are used after a warm reset or power cycle.

    Note that the panic in the case of a failure to write will cause a reboot
    in normal operation - and a reboot after panic will clear the transient
    store.

RETURNS
    n/a
*/
void UpgradeRevertUpgrades(void)
{
    FSTAB_COPY fstab;

    if (loadFstab(&fstab,ps_store_implementation))
    {
        if (!PsStoreFsTab(fstab.ram_copy,fstab.length,FALSE))
        {
            Panic();    /** @todo Panic's bad */
        }
    }
}

/****************************************************************************
NAME
    UpgradePsRunningNewApplication
    
DESCRIPTION
    Query the upgrade ps key to see if we are part way through an upgrade.

    This is used by the application during early boot to check if the
    running application is the upgraded one but it hasn't been committed yet.

    Note: This should only to be called during the early init phase, before
          UpgradeInit has been called.
    
RETURNS
    TRUE if the upgraded application is running but hasn't been
    committed yet. FALSE otherwise, or in the case of an error.
*/
bool UpgradePsRunningNewApplication(uint16 dataPskey, uint16 dataPskeyStart)
{
    UPGRADE_LIB_PSKEY ps_key;

    loadUpgradeKey(&ps_key, dataPskey, dataPskeyStart);

    /* Return true if:
       1) Upgrade is currently at the post-reboot resume point.
       2) The running application is newer than the previous one. 

       Note: If the application version is the same then the upgrade
             must contain only data, e.g. voice prompts, etc. */
    if (ps_key.upgrade_in_progress_key == UPGRADE_RESUME_POINT_POST_REBOOT
        && (ps_key.version_in_progress.major > ps_key.version.major
            || (ps_key.version_in_progress.major == ps_key.version.major
                && ps_key.version_in_progress.minor > ps_key.version.minor)))
    {
        return TRUE;
    }
    
    return FALSE;
}

static void loadUpgradeKey(UPGRADE_LIB_PSKEY *key_data, uint16 key, uint16 key_offset)
{
    uint16 lengthRead;
    uint16 keyCache[PSKEY_MAX_STORAGE_LENGTH];

    /* Worst case buffer is used, so confident we can read complete key 
     * if it exists. If we limited to what it should be, then a longer key
     * would not be read due to insufficient buffer
     * Need to zero buffer used as the cache is on the stack.
     */
    memset(keyCache, 0, sizeof(keyCache));
    lengthRead = PsRetrieve(key, keyCache, PSKEY_MAX_STORAGE_LENGTH);
    if (lengthRead)
    {
        memcpy(key_data, &keyCache[key_offset], sizeof(*key_data));
    }
    else
    {
        memset(key_data, 0x0000, sizeof(*key_data));
    }
}

/****************************************************************************
NAME
    UpgradeIsRunningNewImage
    
DESCRIPTION
    See if we are have rebooted for an image upgrade.
    
RETURNS
    TRUE if the relevant PS keys contain the appropriate information, else FALSE.
*/
bool UpgradeIsRunningNewImage(void)
{
    bool retval = FALSE;
    if (UpgradeCtxGetPSKeys()->upgrade_in_progress_key == UPGRADE_RESUME_POINT_POST_REBOOT
        && (UpgradeCtxGetPSKeys()->version_in_progress.major > UpgradeCtxGetPSKeys()->version.major
            || (UpgradeCtxGetPSKeys()->version_in_progress.major == UpgradeCtxGetPSKeys()->version.major
                && UpgradeCtxGetPSKeys()->version_in_progress.minor > UpgradeCtxGetPSKeys()->version.minor)))
    {
        retval = TRUE;
    }

    return retval;
}
