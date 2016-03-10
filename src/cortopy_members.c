#include "cortopy_members.h"


/* TODO deprecate */
cortopy_pyMemberType
cortopy_objectTypeMemberType(corto_type type)
{
    size_t size = sizeof(PyObject*);
    int mtype = T_OBJECT;
    switch (type->kind) {
    case CORTO_PRIMITIVE:
        switch (corto_primitive(type)->kind) {
        case CORTO_INTEGER:
            if (type->size == sizeof(char)) {
                mtype = T_BYTE;
                size = sizeof(char);
                break;
            } else if (type->size == sizeof(short)) {
                mtype = T_SHORT;
                size = sizeof(short);
                break;
            } else if (type->size == sizeof(int)) {
                mtype = T_INT;
                size = sizeof(int);
                break;
            } else if (type->size == sizeof(long)) {
                mtype = T_LONG;
                size = sizeof(long);
                break;
            } else if (type->size == sizeof(long long)) {
                mtype = T_LONGLONG;
                size = sizeof(long long);
                break;
            }
            break;
        case CORTO_UINTEGER:
            mtype = T_ULONGLONG;
            size = sizeof(corto_uint64);
            break;
        default:
            break;
        }
    default:
        break;
    }
    return (struct cortopy_pyMemberType){mtype, size};
}


size_t
cortopy_objectMemberSize(corto_type type)
{
    size_t size;
    switch (type->kind) {
    case CORTO_PRIMITIVE:
        switch (corto_primitive(type)->kind) {
        case CORTO_INTEGER:
        case CORTO_UINTEGER:
            size = type->size;
            break;
        default:
            size = sizeof(PyObject*);
            break;
        }
    default:
        break;
    }
    return size;
}


int
cortopy_objectMemberType(corto_type type)
{
    int mtype = T_OBJECT;
    switch (type->kind) {
    case CORTO_PRIMITIVE:
        switch (corto_primitive(type)->kind) {
        case CORTO_INTEGER:
            if (type->size == sizeof(char)) { mtype = T_BYTE; }
            else if (type->size == sizeof(short)) { mtype = T_SHORT; }
            else if (type->size == sizeof(int)) { mtype = T_INT; }
            else if (type->size == sizeof(long)) { mtype = T_LONG; }
            else if (type->size == sizeof(long long)) { mtype = T_LONGLONG; }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return mtype;
}
