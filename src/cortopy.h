#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


#ifndef CORTOPY_H
#define CORTOPY_H


extern PyObject* cortopy_CortoError;


#define CORTOPY_LASTERR_GOTO(errorLabel) do { PyErr_SetString(cortopy_CortoError, corto_lasterr()); goto errorLabel; } while(0)
#define CORTOPY_IF_NULL_GOTO(expr, errorLabel) do { if ((expr) == NULL) goto errorLabel; } while(0)

typedef struct {
    PyObject_HEAD
    const char* parent;
    const char* name;
    const char* type;
    corto_object this;
} cortopy_object;


PyObject *
cortopy_tryBuildType(corto_type type);


#endif /* CORTOPY_H */
