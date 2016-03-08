#include "cortopy_serialize.h"


typedef struct cortopy_serData {
    cortopy_object* pyObj;
    void* dest;
} cortopy_serData;


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
    _data->dest = CORTO_OFFSET(_data->pyObj, sizeof(cortopy_object));
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
    // s.program[CORTO_COMPOSITE] = NULL;
    // s.program[CORTO_COLLECTION] = NULL;
    // s.metaprogram[CORTO_ELEMENT] = NULL;
    // s.metaprogram[CORTO_MEMBER] = NULL;
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
