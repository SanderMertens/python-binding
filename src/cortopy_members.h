#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


#ifndef CORTOPY_MEMBERS_H
#define CORTOPY_MEMBERS_H


/* TODO deprecate */
typedef struct cortopy_pyMemberType {
    int type;
    size_t size;
} cortopy_pyMemberType;


/* TODO deprecate */
cortopy_pyMemberType
cortopy_objectTypeMemberType(corto_type type);


size_t
cortopy_objectMemberSize(corto_type type);


int
cortopy_objectMemberType(corto_type type);


#endif /* CORTOPY_MEMBERS_H */
