#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


#ifndef CORTOPY_H
#define CORTOPY_H


extern PyObject* cortopy_CortoError;


#define CORTOPY_LASTERR_GOTO(errorLabel) do { PyErr_SetString(cortopy_CortoError, corto_lasterr()); goto errorLabel; } while(0)


typedef struct {
    PyObject_HEAD
    const char* parent;
    const char* name;
    const char* type;
    corto_object this;
} cortopy_object;


#endif /* CORTOPY_H */
