/* Copyright (c) 2018 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * \file
 * Implementations of marshal trap wrappers.
 */

#if TRAPSET_MARSHAL

#include "marshal/marshal.h"
#include <marshal.h>

/**@{ */
/** Initialise a marshaller / unmarshaller.  */
marshaller_t MarshalInit(const marshal_type_descriptor_t * const *type_desc_list,
                         size_t type_desc_list_elements)
{
    return marshal_init(type_desc_list, type_desc_list_elements);
}
unmarshaller_t UnmarshalInit(const marshal_type_descriptor_t * const *type_desc_list,
                             size_t type_desc_list_elements)
{
    return unmarshal_init(type_desc_list, type_desc_list_elements);
}
/*@}*/

/**@{ */
/** Set the buffer and space / data. */
void MarshalSetBuffer(marshaller_t m, void *buf, size_t space)
{
    marshal_set_buffer(m, buf, space);
}
void UnmarshalSetBuffer(unmarshaller_t u, const void *buf, size_t data_size)
{
    unmarshal_set_buffer(u, buf, data_size);
}
/*@}*/

/**@{ */
/** Marshal / unmarshal. */
bool Marshal(marshaller_t m, void *addr, marshal_type_t type)
{
    return marshal(m, addr, type);
}
bool Unmarshal(unmarshaller_t u, void **addr, marshal_type_t *type)
{
    return unmarshal(u, addr, type);
}
/*@}*/

/**@{ */
/** Get data produced / remaining / consumed. */
size_t MarshalProduced(marshaller_t m)
{
    return marshal_produced(m);
}
size_t MarshalRemaining(marshaller_t m)
{
    return marshal_remaining(m);
}
size_t UnmarshalConsumed(unmarshaller_t u)
{
    return unmarshal_consumed(u);
}
/*@}*/

/**@{ */
/** Clear the internal store of marshalled / unmarshalled object references. */
void MarshalClearStore(marshaller_t m)
{
    marshal_clear_store(m);
}
void UnmarshalClearStore(unmarshaller_t u)
{
    unmarshal_clear_store(u);
}
/*@}*/

/**@{ */
/** Destroy the marshaller / unmarshaller. */
void MarshalDestroy(marshaller_t m, bool free_all_objects)
{
    marshal_destroy(m, free_all_objects);
}
void UnmarshalDestroy(unmarshaller_t u, bool free_all_objects)
{
    unmarshal_destroy(u, free_all_objects);
}
/*@}*/

#endif /* TRAPSET_MARSHAL */
