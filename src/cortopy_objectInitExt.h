#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


#include "cortopy.h"

#ifndef CORTOPY_OBJECTINITEXT_H
#define CORTOPY_OBJECTINITEXT_H


/*
 * TODO change objectInitExt by more specific calls.
 * TODO maybe we don't want the corto parent object because resolve will determine the parent.
 * TODO Support the following use cases:
 * - Do not declare any object in the store, "unbound" object (`thisPtr` is NULL)
 * - Declare a bound object
 *   - Declare a named object (`name` and `parent` not NULL)
 *     - Define the object (define TRUE)
 *     - Define the object (define FALSE)
 *   - Declare anonymous object (`name` and `parent` are NULL)
 *     - Define the object (define TRUE)
 *     - Define the object (define FALSE)
 * Note: if the object with name `name` already exists under `parent`, the object
 * is not declared, but it is serialized (from Corto to Python).
 */
/*
 * If `thisPtr` is NULL, shall not declare a new object;
 * if it is not NULL, then
 *     if *thisPtr is NULL
 *         a new object shall be declared and assigned to location given by the pointer.
 *     if *thisPtr is not NULL
 *         the object is assumed to be in this location
 * Returns 0 on success, -1 on error.
 */
corto_int16
cortopy_objectInitExt(
    cortopy_object* self,
    corto_object parent,
    corto_string name,
    corto_type type,
    corto_bool define,
    corto_object* thisPtr);


/*
 * Equivalent to a call of cortopy_objectInitExt with:
 * - thisPtr NULL,
 * - name NULL,
 * - parent NULL,
 * - define FALSE,
 * - thisPtr NULL.
 * This is useful for values that are not objects with a lifecycle e.g.
 * value-type members of a composite.
 */
corto_int16
cortopy_objectInitUnbound(cortopy_object* self, corto_type type);


#endif /* CORTOPY_OBJECTINITEXT_H */
