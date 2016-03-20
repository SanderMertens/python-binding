#include "cortopy_members.h"
#include "cortopy_serialize.h"
#include "cortopy_objectInitExt.h"


typedef struct cortopy_serData {
    cortopy_object* topLevelObject;
    // corto_bool isTopLevel;
    void* dest; /* location within the PyObject */
    // corto_bool isBase;
    // cortopy_object* lastObjectAllocated;
} cortopy_serData;


static corto_int16
cortopy_serBase(corto_serializer serializer, corto_value* value, void* data)
{
    cortopy_serData* _data = data;
    cortopy_serData newData = *_data;
    if (corto_serializeValue(serializer, value, &newData)) {
        goto error;
    }
    PyTypeObject* pyType = (PyTypeObject*)cortopy_tryBuildType(corto_valueType(value));
    CORTOPY_IF_NULL_GOTO(pyType, error);
    _data->dest = CORTO_OFFSET(_data->dest, pyType->tp_basicsize - sizeof(cortopy_object));
    return 0;
error:
    return -1;
}


static corto_int16
cortopy_serComposite(corto_serializer serializer, corto_value* value, void* data)
{
    /*
     * For structs, create an instance bound to a Corto object.
     * For classes, create an instance not bound to a Corto object.
     * The top level object doesn't need to create an instance.
     */
    cortopy_serData* _data = data;
    cortopy_serData newData = *_data; 
    cortopy_object* object;
    if (value->kind == CORTO_OBJECT) {
        object = _data->topLevelObject;
        // newData.isTopLevel = _data->isTopLevel = FALSE;
    } else if (value->kind != CORTO_BASE) {
        /* TODO it is awkward to make a top-level check here */
        corto_type type = corto_valueType(value);
        if (type->reference) {
            PyErr_Format(PyExc_TypeError, "serializing Corto reference type to "
                "Python type not yet supported (%s)", corto_fullpath(NULL, type));
            goto error;
        }
        PyTypeObject* pyType = (PyTypeObject*)cortopy_tryBuildType(type);
        CORTOPY_IF_NULL_GOTO(pyType, error);
        object = (cortopy_object*)pyType->tp_new(pyType, NULL, NULL);
        CORTOPY_IF_NULL_GOTO(object, error);
        // memcpy(_data->dest, object, pyType->tp_basicsize);
        *(void**)_data->dest = object;
        if (cortopy_objectInitUnbound(object, type)) {
            goto error;
        }
        newData.dest = CORTO_OFFSET(object, sizeof(cortopy_object));
    }
    if (corto_serializeMembers(serializer, value, &newData)) {
        goto error;
    }
    // if (value->kind == CORTO_BASE) {
    //     _data->dest = newData.dest;
    // }
    return 0;
error:
    return -1;
}


static corto_int16
cortopy_serMember(corto_serializer serializer, corto_value* value, void* data)
{
    cortopy_serData* _data = data;
    corto_type type = corto_valueType(value);
    cortopy_serData newData = *_data;
    if (corto_serializeValue(serializer, value, &newData)) {
        goto error;
    }
    size_t size = cortopy_objectMemberSize(type);
    _data->dest = CORTO_OFFSET(_data->dest, size);
    return 0;
error:
    return -1;
}


static corto_int16
cortopy_serPrimitive(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    corto_type type = corto_valueType(value);
    cortopy_serData* _data = data;
    void* valueValue = corto_valueValue(value);
    switch (corto_primitive(type)->kind) {
    case CORTO_INTEGER:
        memcpy(_data->dest, valueValue, type->size);
        break;
    default:
        PyErr_Format(PyExc_TypeError, "setting value not supported for %s", corto_fullpath(NULL, type));
        goto error;
    }
    return 0;
error:
    return -1;
}


static corto_int16
cortopy_serObject(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    cortopy_serData* _data = data;
    _data->dest = CORTO_OFFSET(_data->topLevelObject, sizeof(cortopy_object));
    if (corto_serializeValue(serializer, value, data)) {
        goto error;
    }
    return 0;
error:
    return -1;
}


static struct corto_serializer_s
cortopy_serializer(
    corto_modifier access,
    corto_operatorKind accessKind,
    corto_serializerTraceKind trace)
{
    struct corto_serializer_s s;
    corto_serializerInit(&s);
    s.access = access;
    s.accessKind = accessKind;
    s.traceKind = trace;
    s.reference = NULL;
    s.program[CORTO_PRIMITIVE] = cortopy_serPrimitive;
    s.program[CORTO_COMPOSITE] = cortopy_serComposite;
    // s.program[CORTO_COLLECTION] = NULL;
    // s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = cortopy_serMember;
    s.metaprogram[CORTO_BASE] = cortopy_serBase;
    s.metaprogram[CORTO_OBJECT] = cortopy_serObject;
    return s;
}


corto_int16
cortopy_serialize(corto_object src, cortopy_object* dest)
{
    struct corto_serializer_s serializer = cortopy_serializer(
        CORTO_PRIVATE, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER
    );
    cortopy_serData data = {dest, NULL};
    if (corto_serialize(&serializer, src, &data)) {
        CORTOPY_LASTERR_GOTO(error);
    }
    return 0;
error:
    return -1;
}
