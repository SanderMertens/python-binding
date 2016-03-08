#include "cortopy_deserialize.h"


typedef struct cortopy_objectDeserData {
    void* dest;       /* pointer to corto object or insides of it */
    PyObject* value;  /* value that should be written into dest, comes from args or kwargs */
    void* src; /* */
} cortopy_objectDeserData;


static corto_int16
cortopy_objectDeserPrimitive(corto_serializer serializer, corto_value* value, void* data)
{
    cortopy_objectDeserData* _data = data;
    corto_type type = corto_valueType(value);
    switch (corto_primitive(type)->kind) {
    case CORTO_INTEGER:
        memcpy(_data->dest, _data->src, type->size);
        break;
    default:
        PyErr_Format(PyExc_TypeError, "type not supported for update: %s", corto_fullpath(NULL, corto_valueType(value)));
        goto error;
    }
    return 0;
error:
    return -1;
}


static corto_int16
cortopy_objectDeserObject(corto_serializer serializer, corto_value* value, void* data)
{
    /*
     * For any cortopy.object, its value starts after the PyObject headers,
     * which are the size of PyObject.
     */
    cortopy_objectDeserData* _data = data;
    _data->src = CORTO_OFFSET(_data->value, sizeof(cortopy_object));
    if (corto_serializeValue(serializer, value, data)) {
        goto error;
    }
    return 0;
error:
    return -1;
}


static struct corto_serializer_s
cortopy_objectDeserializer(
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
    s.program[CORTO_PRIMITIVE] = cortopy_objectDeserPrimitive;
    s.program[CORTO_COMPOSITE] = NULL;
    s.program[CORTO_COLLECTION] = NULL;
    s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = NULL;
    s.metaprogram[CORTO_OBJECT] = cortopy_objectDeserObject;
    return s;
}


corto_int16
cortopy_objectDeser(cortopy_object* self)
{
    struct corto_serializer_s serializer = cortopy_objectDeserializer(
        CORTO_PRIVATE, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER
    );
    cortopy_objectDeserData data = {self->this, (PyObject*)self, NULL};
    // TODO replace type string for type object
    corto_type type = corto_resolve(NULL, (corto_string)self->type);
    if (type == NULL) {
        PyErr_Format(PyExc_ValueError, "cannot find %s", self->type);
        goto errorTypeNotFound;
    }
    if (corto_metaWalk(&serializer, type, &data)) {
        goto errorMetaWalk;
    }
    corto_release(type);
    return 0;
errorMetaWalk:
    corto_release(type);
errorTypeNotFound:
    return -1;
}
