#include "cortopy_objectInitExt.h"
#include "cortopy_serialize.h"


static int
cortopy_objectInitExtValidateParams(
    cortopy_object* self,
    corto_object parent,
    corto_string name,
    corto_type type,
    corto_bool define,
    corto_object* thisPtr)
{
    CORTO_UNUSED(self);
    CORTO_UNUSED(type);   
    if (define && thisPtr == NULL) {
        PyErr_SetString(PyExc_SystemError, "'thisPtr' must be non-NULL when define is TRUE");
        goto error;
    }
    if (thisPtr) {
        if (*thisPtr) {
            if (parent || type) {
                PyErr_Format(PyExc_SystemError, "only provide 'parent' and 'type' when *thisPtr is NULL (when no new Corto object should be declared)");
                goto error;
            }
        } else {
            if ((name && parent == NULL) || (name == NULL && parent)) {
                PyErr_Format(PyExc_SystemError, "'name' and 'parent' must be both NULL or non-NULL when *thisPtr is NULL (when declaring new anonymous or scoped Corto object)");
                goto error;
            }
        }
    }
    return 0;
error:
    return -1;
}


corto_int16
cortopy_objectInitExt(
    cortopy_object* self,
    corto_object parent,
    corto_string name,
    corto_type type,
    corto_bool define,
    corto_object* thisPtr)
{
    if (cortopy_objectInitExtValidateParams(self, parent, name, type, define, thisPtr)) {
        goto errorInvalidArguments;
    }
    if (thisPtr && *thisPtr) {
        parent = corto_parentof(*thisPtr);
        type = corto_typeof(*thisPtr);
    }
    corto_bool serialize = thisPtr ? TRUE : FALSE;
    if (thisPtr) {
        /* TODO release Corto object when Python object is garbage-collected */
        *thisPtr = corto_lookup(parent, name);
        if (*thisPtr == NULL) {
            serialize = FALSE;
            *thisPtr = corto_declareChild(parent, name, type);
            if (*thisPtr == NULL) {
                CORTOPY_LASTERR_GOTO(errorCreateChild);
            }
        }
        self->this = *thisPtr;
    } else {
        self->this = NULL;
    }

    if (name && (self->name = corto_strdup(name)) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupName);
    }
    if (parent && (self->parent = corto_strdup(corto_fullpath(NULL, parent))) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupParentName);
    }
    if ((self->type = corto_strdup(corto_fullpath(NULL, type))) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupType);
    }
    if (serialize && cortopy_serialize(*thisPtr, self)) {
        goto errorSerialize;
    }
    /* TODO Should we define regardless if the object was lookup'd or declared? */
    if (define) {
        if (corto_define(*thisPtr)) {
            corto_release(*thisPtr);
            *thisPtr = NULL;
            CORTOPY_LASTERR_GOTO(errorDefine);
        }
    }
    return 0;

errorDefine:
errorSerialize:
errorDupType:
    corto_dealloc((void*)self->parent);
    self->parent = NULL;
errorDupParentName:
    corto_dealloc((void*)self->name);
    self->name = NULL;
errorDupName:
errorCreateChild:
errorInvalidArguments:
    return -1;
}


corto_int16
cortopy_objectInitUnbound(cortopy_object* self, corto_type type)
{
    return cortopy_objectInitExt(self, NULL, NULL, type, FALSE, NULL);
}
