/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 *
 * Header for the marshal object set (mobs).
 * The marshal object set stores unique marshal objects in the order in which
 * they are pushed onto the set.
 */

#ifndef MARSHAL_OBJECT_SET_H_
#define MARSHAL_OBJECT_SET_H_

#include "hydra/hydra_types.h"
#include "marshal.h"

/** Type definition for the marshal object.
 *  The address and type of the mob_t are used as the 'key' when searching
 *  the object sets (the disambiguator is not used).
*/
typedef struct marshal_object
{
    void *address;
    marshal_type_t type;
    /** This stores the number of elements in objects containing dynamic length
     *  arrays and the tagged member index of objects containing tagged unions.
     */
    uint8 disambiguator;
} mob_t;

/** Helper macro to zero instance of marshal objects */
#define MOB_ZERO() {NULL, 0, 0}

/** Maximum number of marshalled objects the set is capable of containing (based
 *  on the type of mob_index_t) and one code to indicate invalid objects.
 */
#define MOBS_MAX_OBJECTS UCHAR_MAX

/** Index for a marshalled object. This is the index of the index object
 *  in the marshal object set. Note the size of this type defines the header
 *  size for the marshal byte stream pointer indexes.
 */
typedef uint8 mob_index_t;

/** Opaque forward declaration of object block used in the object set */
typedef struct marshal_object_block mob_block_t;

/** The marshal object set stores unique marshal objects in a linked list of
 *  marshal object blocks.
 */
typedef struct marshal_object_set
{
    mob_block_t *head;
    mob_block_t *tail;
    mob_index_t elements;
} mobs_t;

/** Type definition of the callback function from the mobs_iterate function.
 *  \param set The set passed to the mobs_iterate function.
 *  \param index The index of the marshal object in the set.
 *  \param object The marshal object. Modifying the object does _not_ modify the
 *                state of the object stored in the list.
 *  \return If the callback returns FALSE, the iteration will stop.
 */
typedef bool (*mobs_callback_t)(mobs_t *set, mob_index_t index, mob_t *object);

/** Initialise an instance of the marshal object set.
 *  \param set The set to initialise.
 */
void mobs_init(mobs_t *set);

/** Destroy an instance of the marshal object set
 *  \param set The set to destroy.
 */
void mobs_destroy(mobs_t *set);

/** Query the number of objects in the set.
 *  \param set The set to query.
 *  \return The number of objects in the set.
 */
mob_index_t mobs_size(mobs_t *set);

/** Push an object to the tail of the set.
 *  \param set The set.
 *  \param object The object to push onto the set.
 *  \return TRUE if the object was pushed onto the set.
 *          FALSE if the object was not pushed because a duplicate object was
 *          already in the set.
 *
 *  Note that the set can only contain MOBS_MAX_OBJECTS.
 */
bool mobs_push(mobs_t *set, mob_t *object);

/** Pop an object from the tail of the set.
 *  \param set The set.
 *  \param object The popped object.
 *  \return TRUE if an object was popped from the set.
 *          FALSE if the set was empty.
 */
bool mobs_pop(mobs_t *set, mob_t *object);

/** Remove an object from the set.
 *  \param set The set.
 *  \param object The object to remove.
 *  \return TRUE if an object was present in the set and removed.
 *          FALSE if the object was not in the set.
 *
 *  \note The indexes of objects following the removed object are changed
 *        following the removal of an object from the set.
 */
bool mobs_remove(mobs_t *set, const mob_t *object);

/** Query if an object is in the set.
 *  \param set The set.
 *  \param object The object.
 *  \param index If not NULL, will be set to the index of the object in the set.
 *  \return TRUE if the object is in the set, otherwise FALSE.
 */
bool mobs_has_object(mobs_t *set, const mob_t *object, mob_index_t *index);

/** Get the object at the specified index.
 *  \param set The set.
 *  \param index The index of the object to get.
 *  \param object If not NULL, will be set to the object at the specified index.
 *  \return TRUE if the index is in the set, otherwise FALSE.
 */
bool mobs_get_object(mobs_t *set, mob_index_t index, mob_t *object);

/** Iterate through the objects in the set.
 *  \param set The set to iterate.
 *  \param start Index from which to start iterating.
 *  \param cb Function to call for each object. Must be set.
 *  \return TRUE if the iteration continued to completion.
 *          FALSE if the iteration stopped because the callback returned FALSE.
 *  \note Objects may be pushed to the set by the callback to extend the iteration.
 *        Objects may not be removed or popped from the set during the iteration.
 */
bool mobs_iterate(mobs_t *set, mob_index_t start, mobs_callback_t cb);

/** Update set, removing objects from set found in remove.
 *  \param set The set to update.
 *  \param remove Set of items to remove (if present) from set.
 */
void mobs_difference_update(mobs_t *set, const mobs_t *remove);

#endif
