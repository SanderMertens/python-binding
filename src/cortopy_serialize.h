#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"

#include "cortopy.h"


#ifndef CORTOPY_SERIALIZE_H
#define CORTOPY_SERIALIZE_H


corto_int16
cortopy_serialize(corto_object src, cortopy_object* dest);


#endif /* CORTOPY_SERIALIZE_H */
