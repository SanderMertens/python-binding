#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"

#define CORTOPY_LASTERR_GOTO(errorLabel) do { PyErr_SetString(cortopy_CortoError, corto_lasterr()); goto errorLabel; } while(0)

static PyObject* cortopy_CortoError;

/*
 * Key: Full name of a Corto type
 * Value: None when type is being serialized, or cortopy_type object when ready
 */
static PyObject* cortopy_typesCache;
static PyObject* cortopy_typesCacheMappingProxy;

static PyTypeObject* MappingProxyType;
static PyTypeObject* import_MappingProxyType(void) {
    PyObject* typesModule = PyImport_ImportModule("types");
    if (typesModule == NULL) {
        goto error;
    }
    PyObject* MappingProxyType = PyObject_GetAttrString(typesModule, "MappingProxyType");
    Py_DECREF(typesModule);
    if (MappingProxyType == NULL) {
        goto error;
    }
    if (!PyType_Check(MappingProxyType)) {
        PyErr_SetString(PyExc_TypeError, "error importing MappingProxyType");
        Py_DECREF(MappingProxyType);
        goto error;
    }
    return (PyTypeObject*)MappingProxyType;
error:
    return NULL;
}


typedef struct {
    PyObject_HEAD
    const char* parent;
    const char* name;
    const char* type;
    corto_object this;
} cortopy_object;

static PyTypeObject cortopy_objectType;


static int
cortopy_objectInitExt(cortopy_object* self, corto_object parent, corto_string name, corto_type type, corto_object* this_p);

static int
cortopy_objectInit(cortopy_object* self, PyObject* args, PyObject* kwargs);

static PyObject*
cortopy_objectUpdate(cortopy_object* self, PyObject* args, PyObject* kwargs);

static PyObject *
cortopy_deserialize_primitive(corto_object cortoObj);

static PyObject *
cortopy_deserialize_void(corto_object cortoObj);

static PyObject *
cortopy_nameof(PyObject* self, PyObject* args);

static PyObject *
cortopy_declareChild(PyObject* self, PyObject* args, PyObject* kwargs);

static PyObject *
cortopy_define(PyObject* module, cortopy_object* o);

static PyObject *
cortopy_cortostr(PyObject* module, cortopy_object* o);

static PyObject *
cortopy_tryBuildType(corto_type type);

static PyTypeObject *
cortopy_buildType(corto_type type);

static PyObject *
cortopy_getType(PyObject* self, PyObject* args, PyObject* kwargs);

static PyObject *
cortopy_cortoToCortopyObject(corto_object o);

static PyObject *
cortopy_resolve(PyObject* self, PyObject* args);

static PyObject *
cortopy_setval(cortopy_object* self, PyObject* args, PyObject* kwargs);

typedef struct cortopy_pyMemberType {
    int type;
    size_t size;
} cortopy_pyMemberType;

static cortopy_pyMemberType
cortopy_objectTypeMemberType(corto_type type);

/*
 * Takes a cortopy.object, a type, or a str
 */
static int
cortopy_convertToObjectExt(PyObject* o, void* p, const char* msg)
{
    corto_object c = NULL;
    const char* name = NULL;
    if (PyObject_IsInstance(o, (PyObject*)&cortopy_objectType)) {
        c = ((cortopy_object*)o)->this;
        corto_claim(c);
    } else if (PyType_Check(o) && PyType_IsSubtype((PyTypeObject*)o, &cortopy_objectType)) {
        name = ((PyTypeObject*)o)->tp_name;
        c = corto_resolve(NULL, (corto_string)name);
    } else if (PyObject_IsInstance(o, (PyObject*)&PyUnicode_Type)) {
        name = PyUnicode_AsUTF8(o);
        c = corto_resolve(NULL, (corto_string)name);
    } else {
        PyErr_Format(PyExc_TypeError, msg, Py_TYPE(o)->tp_name);
        goto error;
    }
    if (c == NULL) {
        PyErr_Format(PyExc_ValueError, "cannot find %s", name);
        goto error;
    }
    *(corto_object*)p = c;
    return 1;
error:
    return 0;
}
/*
 * Gets a Corto object form a string or a Cortopy object.
 * Caller must release reference to the object.
 * p must corto_object*
 */
static int
cortopy_convertToObject(PyObject* o, void* p)
{
    return cortopy_convertToObjectExt(o, p, "expected cortopy.Object or str, not '%s'");
}


static int
cortopy_convertToObjectOrRoot(PyObject* o, void* p)
{
    if (o == Py_None) {
        corto_claim(root_o);
        *(corto_object*)p = root_o;
    } else {
        int success = cortopy_convertToObjectExt(o, p, "expected cortopy.Object, str, or None, not %s");
        if (!success) {
            goto error;
        }
    }
    return 1;
error:
    return 0;
}


/*
 * If `this_p` is NULL, shall not declare a new object; if it is not NULL, then
 *     a new object shall be declared and assigned to location given by the pointer.
 * Returns 0 on success, -1 on error.
 */
static int
cortopy_objectInitExt(cortopy_object* self, corto_object parent, corto_string name, corto_type type, corto_object* this_p)
{
    if (this_p) {
        *this_p = corto_declareChild(parent, (corto_string)name, type);
        if (*this_p == NULL) {
            CORTOPY_LASTERR_GOTO(errorCreateChild);
        }
        self->this = *this_p;
    } else {
        self->this = NULL;
    }

    if ((self->name = corto_strdup(name)) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupName);
    }

    if ((self->parent = corto_strdup(corto_fullpath(NULL, parent))) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupParentName);
    }
    corto_release(parent);
    parent = NULL;

    if ((self->type = corto_strdup(corto_fullpath(NULL, type))) == NULL) {
        CORTOPY_LASTERR_GOTO(errorDupType);
    }
    corto_release(type);
    type = NULL;

    return 0;

errorDupType:
    corto_dealloc((void*)self->parent);
errorDupParentName:
    corto_dealloc((void*)self->name);
errorDupName:
errorCreateChild:
    if (parent) {
        corto_release(parent);
    }
    return -1;
}


static int
cortopy_objectInit(cortopy_object* self, PyObject* args, PyObject* kwargs)
{
    static char* kwds[] = {"parent", "name", "type", NULL};
    corto_object parent = NULL;
    corto_string name = NULL;
    corto_object type = NULL;
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "O&sO&:CortoObject", kwds,
            cortopy_convertToObjectOrRoot, &parent,
            &name,
            cortopy_convertToObject, &type)
    ) {
        goto errorParseTupleAndKeywords;
    }
    if (name == NULL) {
        PyErr_SetString(PyExc_ValueError, "both parent and name should be either None or str");
        goto errorNeitherScopedNotAnonymous;
    }

    corto_object this;
    if (cortopy_objectInitExt(self, parent, name, type, &this)) {
        goto errorInitExt;
    }

    if (corto_define(this)) {
        CORTOPY_LASTERR_GOTO(errorDefine);
    }

    return 0;
errorDefine:
errorInitExt:
errorNeitherScopedNotAnonymous:
errorParseTupleAndKeywords:
    return -1;
}


static PyObject *
cortopy_objectBeginUpdate(cortopy_object* self)
{
    if (corto_updateBegin(self->this)) {
        CORTOPY_LASTERR_GOTO(error);
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static PyObject *
cortopy_objectCancelUpdate(cortopy_object* self)
{
    if (corto_updateCancel(self->this)) {
        CORTOPY_LASTERR_GOTO(error);
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static PyObject *
cortopy_objectEndUpdate(cortopy_object* self)
{
    corto_updateEnd(self->this);
    Py_RETURN_NONE;
}


static PyObject *
cortopy_objectUpdate(cortopy_object* self, PyObject* args, PyObject* kwargs)
{
    if (corto_typeof(self->this)->kind == CORTO_VOID) {
        char* kwds[] = {NULL};
        if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwds)) {
            goto error;
        }
        if (corto_update(self->this)) {
            goto error;
        }
    } else {
        if (corto_updateBegin(self->this)) {
            CORTOPY_LASTERR_GOTO(error);
        }
        if (cortopy_setval(self, args, kwargs) == NULL) {
            goto error;
        }
        corto_updateEnd(self->this);
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static PyMethodDef cortopy_objectMethods[] = {
    {"update", (PyCFunction)cortopy_objectUpdate, METH_VARARGS|METH_KEYWORDS, "updates the value"},
    {"begin_update", (PyCFunction)cortopy_objectBeginUpdate, METH_NOARGS, "begins an update"},
    {"cancel_update", (PyCFunction)cortopy_objectCancelUpdate, METH_NOARGS, "cancels an update"},
    {"end_update", (PyCFunction)cortopy_objectEndUpdate, METH_NOARGS, "ends an update"},
    {"setval", (PyCFunction)cortopy_setval, METH_VARARGS|METH_KEYWORDS, ""},
    {NULL}
};


static PyMemberDef cortopy_objectMembers[] = {
    {"parent", T_STRING, offsetof(cortopy_object, parent), READONLY, "full path of the object's parent"},
    {"name", T_STRING, offsetof(cortopy_object, name), READONLY, "name of the object"},
    {"type", T_STRING, offsetof(cortopy_object, type), READONLY, "memory location of the Corto object"},
    {NULL}
};


static PyTypeObject cortopy_objectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cortopy.object",             /* tp_name */
    sizeof(cortopy_object),       /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "Corto object with lifecycle",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    cortopy_objectMethods,     /* tp_methods */
    cortopy_objectMembers,     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)cortopy_objectInit,                         /* tp_init */
    0,                         /* tp_alloc */
};


static PyObject *
cortopy_deserialize_primitive(corto_object cortoObj)
{
    corto_assert(corto_typeof(cortoObj)->kind != CORTO_PRIMITIVE, "expected corto primitive");

    PyObject* pyObj = NULL;
    switch (corto_primitive(corto_typeof(cortoObj))->kind) {
    case CORTO_INTEGER:
        // pyObj = cortopy_deserialize_integer(cortoObj);
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "primitive type not supported");
        pyObj = NULL;
        break;
    }
    return pyObj;
}


static PyObject *
cortopy_deserialize_void(corto_object cortoObj)
{
    corto_assert(corto_typeof(cortoObj)->kind != CORTO_PRIMITIVE, "expected corto void");

    PyObject* newargs = Py_BuildValue("(s s)", corto_fullpath(NULL, corto_parentof(cortoObj)), corto_nameof(cortoObj));
    if (!newargs) {
        goto errorNewArgs;
    }
    PyObject* pyObj = cortopy_objectType.tp_new(&cortopy_objectType, newargs, NULL);
    if (pyObj == NULL) {
        goto errorNew;
    }
    if (cortopy_objectType.tp_init(pyObj, newargs, NULL)) {
        goto errorInit;
    }
    return pyObj;
errorInit:
    Py_DECREF(pyObj);
errorNew:
errorNewArgs:
    return NULL;
}


/*
 * This function returns a new reference.
 */
static PyObject *
cortopy_nameof(PyObject* self, PyObject* args)
{
    PyObject* o = NULL;
    if (!PyArg_ParseTuple(args, "O", &o)) {
        return NULL;
    }
    PyObject* name = Py_None;
    if (Py_TYPE(o) == &cortopy_objectType) {
        name = PyUnicode_FromString(((cortopy_object*)o)->name);
    } else {
        PyErr_SetString(PyExc_TypeError, "did not provide valid Corto object to cortopy.nameof");
        return NULL;
    }
    return name;
}


#define CORTOPY_DECLARE_CHILD_DOC \
"Declares a scoped Corto object given the names of parent, the new object, and type."

static PyObject *
cortopy_declareChild(PyObject* self, PyObject* args, PyObject* kwargs)
{
    static char* kwds[] = {"parent", "name", "type", "value", NULL};
    static char* argFmt = "O&sO&|O:declare_child";
    corto_object parent;
    char* name = NULL;
    corto_object type = NULL;
    PyObject* value = NULL;
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, argFmt, kwds,
            cortopy_convertToObjectOrRoot, &parent,
            &name,
            cortopy_convertToObject, &type,
            &value)
    ) {
        goto errorParseTupleAndKeywords;
    }

    corto_object o = corto_declareChild(parent, name, type);
    if (!o) {
        CORTOPY_LASTERR_GOTO(errorDeclareChild);
    }

    PyTypeObject* cpType = (PyTypeObject*)cortopy_tryBuildType(type);
    if (cpType == NULL) {
        goto errorTryBuildType;
    } else if ((PyObject*)cpType == Py_None) {
        PyErr_Format(PyExc_RuntimeError, "type %s was cached as None", corto_fullpath(NULL, type));
        goto errorTryBuildType;
    }
    PyObject* cpObj;
    PyObject* newargs = Py_BuildValue("(s s s)", corto_fullpath(NULL, parent), name, corto_fullpath(NULL, type));
    cpObj = cpType->tp_new(cpType, newargs, NULL);
    if (cpObj == NULL) {
        goto errorNew;
    }
    if (cpType->tp_init(cpObj, newargs, NULL)) {
        goto errorInit;
    }
    Py_DECREF(newargs);

    corto_release(type);
    corto_release(parent);
    return cpObj;
errorInit:
errorNew:
errorTryBuildType:
errorDeclareChild:
    corto_release(type);
    corto_release(parent);
errorParseTupleAndKeywords:
    return NULL;
}


static PyObject *
cortopy_define(PyObject* module, cortopy_object* o)
{
    if (corto_define(o->this)) {
        CORTOPY_LASTERR_GOTO(error);
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static corto_int16
cortopy_setvalInteger(void* dest, PyObject* pyValue, corto_type type)
{
    int overflow = 0;
    signed long long int cValue = PyLong_AsLongLongAndOverflow(pyValue, &overflow);
    signed long long int max = corto_int(type)->max;
    signed long long int min = corto_int(type)->min;
    if (overflow || cValue > max || cValue < min) {
        PyErr_Format(PyExc_ValueError, "value must be int between %lld and %lld, have %S", min, max, pyValue);
        goto error;
    }
    corto_binaryOperator(type, CORTO_ASSIGN, dest, &cValue, dest);
    return 0;
error:
    return -1;
}


typedef struct cortopySetvalSerializerData {
    void* dest;       /* pointer to insides of cortopy object */
    PyObject *args;   /* original args passed to set val */
    PyObject *kwargs; /* original kwargs passed to set val */
    PyObject* value;  /* value that should be written into dest, comes from args or kwargs */
} cortopySetvalSerializerData;


static corto_int16
cortopy_setvalSerializePrimitive(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    corto_int16 result = 0;
    corto_type type = corto_valueType(value);
    cortopySetvalSerializerData* _data = data;

    switch (corto_primitive(type)->kind) {
    case CORTO_INTEGER:
        {
            result = cortopy_setvalInteger(_data->dest, _data->value, type);
        }
        break;
    default:
        PyErr_Format(PyExc_TypeError, "setting value not supported for %s", corto_fullpath(NULL, type));
    }
    return result;
}

static corto_int16
cortopy_setvalSerializeComposite(corto_serializer serializer, corto_value* value, void* data)
{
    cortopySetvalSerializerData* _data = data;
    static char* kwds[] = {"value", NULL};
    PyObject* valueDict = NULL;
    // TODO this function may not have to parse arguments
    if (!PyArg_ParseTupleAndKeywords(_data->args, _data->kwargs, "O!", kwds, &PyDict_Type, &valueDict)) {
        goto error;
    }
    _data->value = valueDict;
    if (corto_serializeMembers(serializer, value, data)) {
        goto error;
    }
    return 0;
error:
    return -1;
}

static corto_int16
cortopy_setvalSerializeMember(corto_serializer serializer, corto_value* value, void* data)
{
    cortopySetvalSerializerData* _data = data;
    corto_member member = value->is.member.t;
    corto_string memberName = corto_nameof(member);
    PyObject* valueDict = _data->value;
    PyObject* memberValue = PyDict_GetItemString(valueDict, memberName);
    struct cortopy_pyMemberType memberType = cortopy_objectTypeMemberType(corto_valueType(value));
    if (memberValue) {
        Py_INCREF(memberValue);
        cortopySetvalSerializerData memberData = *_data;
        memberData.value = memberValue;
        if (corto_serializeValue(serializer, value, &memberData)) {
            goto error;
        }
    }
    _data->dest = CORTO_OFFSET(_data->dest, memberType.size);
    Py_DECREF(memberValue);
    return 0;
error:
    Py_DECREF(memberValue);
    return -1;
}

static corto_int16
cortopy_setvalSerializeObject(corto_serializer serializer, corto_value* value, void* data)
{
    /*
     * For any cortopy.object, its value starts after the PyObject headers,
     * which are the size of PyObject.
     */
    cortopySetvalSerializerData* _data = data;
    PyObject* pyValue = NULL;
    static char* kwds[] = {"value", NULL};
    switch (corto_valueType(value)->kind) {
    case CORTO_PRIMITIVE:
        if (!PyArg_ParseTupleAndKeywords(_data->args, _data->kwargs, "O:setval", kwds, &pyValue)) {
            goto error;
        }
        break;
    case CORTO_COMPOSITE:
        if (!PyArg_ParseTupleAndKeywords(_data->args, _data->kwargs, "O!:setval", kwds, &PyDict_Type, &pyValue)) {
            goto error;
        }
        break;
    default:
        PyErr_Format(PyExc_ValueError, "cannot setval on this type");
        goto error;
    }
    _data->value = pyValue;
    _data->dest = CORTO_OFFSET(_data->dest, sizeof(cortopy_object));
    if (corto_serializeValue(serializer, value, data)) {
        goto error;
    }
    return 0;
error:
    return -1;
}



static struct corto_serializer_s
cortopy_setvalSerializer(
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
    s.program[CORTO_PRIMITIVE] = cortopy_setvalSerializePrimitive;
    s.program[CORTO_COMPOSITE] = cortopy_setvalSerializeComposite;
    s.program[CORTO_COLLECTION] = NULL;
    s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = cortopy_setvalSerializeMember;
    s.metaprogram[CORTO_OBJECT] = cortopy_setvalSerializeObject;

    return s;
}


static PyObject *
cortopy_setval(cortopy_object* self, PyObject* args, PyObject* kwargs)
{
    /* Argument parsing is done when during object-level serialization */
    struct corto_serializer_s serializer = cortopy_setvalSerializer(
        CORTO_PRIVATE, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER
    );
    cortopySetvalSerializerData data = {self, args, kwargs, NULL};
    corto_type type = corto_resolve(NULL, (corto_string)self->type);
    if (type == NULL) {
        PyErr_Format(PyExc_ValueError, "cannot find %s", self->type);
        goto errorTypeNotFound;
    }
    if (corto_metaWalk(&serializer, type, &data)) {
        goto errorMetaWalk;
    }
    corto_release(type);
    Py_RETURN_NONE;
errorMetaWalk:
    corto_release(type);
errorTypeNotFound:
    return NULL;
}


/*
 * Returns the corto serialization of the object *as is in memory*.
 */
static PyObject *
cortopy_cortostr(PyObject* module, cortopy_object* o)
{
    if (o->this == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "object is not in the object store");
        goto error;
    }
    corto_string str = corto_str(o->this, 0);
    if (str == NULL) {
        CORTOPY_LASTERR_GOTO(error);
    }
    PyObject* pystr = PyUnicode_FromString(str);
    corto_dealloc(str);
    return pystr;
error:
    return NULL;
}


/* START TYPE DESERIALIZER */


static corto_int16
cortopy_serializeTypeDependency(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    CORTO_UNUSED(data);
    corto_type type = corto_valueType(value);
    PyObject* pyType = cortopy_tryBuildType(type);
    if (pyType == NULL) {
        goto error;
    }
    return 0;
error:
    return -1;
}


static struct corto_serializer_s
cortopy_typeSerializer(
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
    s.program[CORTO_PRIMITIVE] = NULL;
    s.program[CORTO_COLLECTION] = NULL;
    s.metaprogram[CORTO_ELEMENT] = cortopy_serializeTypeDependency;
    s.metaprogram[CORTO_MEMBER] = cortopy_serializeTypeDependency;
    s.metaprogram[CORTO_BASE] = NULL;
    s.metaprogram[CORTO_OBJECT] = NULL;

    return s;
}


static cortopy_pyMemberType
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


static corto_int16
cortopy_sizeForTypeSerializerMember(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    struct cortopy_pyMemberType* _data = data;
    corto_type type = corto_valueType(value);
    _data->size += cortopy_objectTypeMemberType(type).size;
    return 0;
}


static struct corto_serializer_s
cortopy_sizeForTypeSerializer(
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
    s.program[CORTO_PRIMITIVE] = cortopy_sizeForTypeSerializerMember;
    s.program[CORTO_COLLECTION] = NULL;
    s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = cortopy_sizeForTypeSerializerMember;
    s.metaprogram[CORTO_OBJECT] = NULL;

    return s;
}

static size_t
cortopy_sizeForType(corto_type type)
{
    struct corto_serializer_s s = cortopy_sizeForTypeSerializer(CORTO_PRIVATE, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER);
    struct cortopy_pyMemberType data = {0};
    corto_metaWalk(&s, type, &data);
    return sizeof(cortopy_object) + data.size;
}


static PyObject *
cortopy_typeRepr(PyObject* o)
{
    return Py_BuildValue("s", corto_fullpath(NULL, ((cortopy_object*)o)->this));
}


static int
cortopy_buildMemberDefPrimitive(PyTypeObject* cpType, corto_type cType)
{
    CORTO_UNUSED(cpType);
    struct cortopy_pyMemberType memberType = cortopy_objectTypeMemberType(cType);
    PyMemberDef* memberDefs = corto_alloc(2 * sizeof(PyMemberDef));
    if (memberDefs == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    memberDefs[0] = (PyMemberDef){"val", memberType.type, sizeof(cortopy_object), 0, NULL};
    memberDefs[1] = (PyMemberDef){0};
    cpType->tp_members = memberDefs;
    return 0;
error:
    return -1;
}


typedef struct cortopy_memberCountData {size_t memberDefCount; size_t getSetDefCount; } cortopy_memberCountData;


/*
 * Determines whether a type should use PyMemberDef or PyGetSetDef.
 */
static corto_bool
cortopy_usesGetSetDef(corto_type type)
{
    return type->kind != CORTO_PRIMITIVE;
}


static corto_int16
cortopy_memberCountSerializerCount(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    cortopy_memberCountData *_data = data;
    if (cortopy_usesGetSetDef(corto_valueType(value))) {
        _data->getSetDefCount++;
    } else {
        _data->memberDefCount++;
    }
    return 0;
}

static struct corto_serializer_s
cortopy_memberCountSerializer(
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
    s.program[CORTO_PRIMITIVE] = NULL;
    s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = cortopy_memberCountSerializerCount;
    s.metaprogram[CORTO_BASE] = NULL;
    s.metaprogram[CORTO_OBJECT] = NULL;

    return s;
}

typedef struct cortopy_memberDefSerializerData {
    PyMemberDef* memberDefs;
    PyGetSetDef* getSetDefs;
    size_t memberDefCount;
    size_t getSetDefCount;
    size_t totalOffset;
} cortopy_memberDefSerializerData;


typedef struct cortopy_getSetClosure {
    char* name;
    PyObject* type;
    size_t offset;
} cortopy_getSetClosure;


static PyObject *
cortopy_compositeGetter(PyObject* self, void* closure)
{
    cortopy_getSetClosure* _closure = closure;
    PyObject** value = CORTO_OFFSET(self, _closure->offset);
    if (*value == NULL) {
        PyErr_Format(PyExc_ValueError, "could not retrieve value from member %s", _closure->name);
        goto error;
    }
    return *value;
error:
    return NULL;
}


static int
cortopy_compositeSetter(PyObject* self, PyObject* value, void* closure)
{
    cortopy_getSetClosure* _closure = closure;
    if (!PyObject_IsInstance(value, _closure->type)) {
        PyErr_Format(PyExc_TypeError, "expected type %S but got type %S", _closure->type, Py_TYPE(value));
        goto error;
    }
    Py_INCREF(value);
    PyObject** target = CORTO_OFFSET(self, _closure->offset);
    Py_XDECREF(*target);
    *target = value;
    return 0;
error:
    return -1;
}


static int
cortopy_createGetSetDef(char* name, PyObject* type, cortopy_memberDefSerializerData* data)
{
    PyGetSetDef* getSetDef = &(data->getSetDefs[data->getSetDefCount]);
    cortopy_getSetClosure* closure = corto_alloc(sizeof(cortopy_getSetClosure));
    if (closure == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    *closure = (cortopy_getSetClosure){name, type, data->totalOffset};
    *getSetDef = (PyGetSetDef){name, cortopy_compositeGetter, cortopy_compositeSetter, NULL, closure};
    data->getSetDefCount++;
    return 0;
error:
    return -1;
}


static int
cortopy_createMemberDef(char* name, cortopy_pyMemberType* memberType, cortopy_memberDefSerializerData* data)
{
    PyMemberDef* memberDef = &(data->memberDefs[data->memberDefCount]);
    *memberDef = (PyMemberDef){name, memberType->type, data->totalOffset, 0, NULL};
    data->memberDefCount++;
    return 0;
}


static corto_int16
cortopy_memberDefMember(corto_serializer serializer, corto_value* value, void* data)
{
    CORTO_UNUSED(serializer);
    cortopy_memberDefSerializerData* _data = data;
    corto_member member = value->is.member.t;
    corto_string name = corto_nameof(member);
    corto_type type = corto_valueType(value);
    cortopy_pyMemberType memberType = cortopy_objectTypeMemberType(type);
    if (cortopy_usesGetSetDef(type)) {
        PyObject* cpType = cortopy_tryBuildType(type);
        if (cpType == NULL) {
            goto error;
        }
        if (cortopy_createGetSetDef(name, cpType, _data)) {
            goto error;
        }
    } else {
        if (cortopy_createMemberDef(name, &memberType, _data)) {
            goto error;
        }
    }
    _data->totalOffset += memberType.size;
    return 0;
error:
    return -1;
}

/*
 * Serializer that creates PyMemberDef and PyGetSetDef instances for a class
 */
static struct corto_serializer_s
cortopy_memberDefSerializer(
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
    s.program[CORTO_PRIMITIVE] = NULL;
    s.metaprogram[CORTO_ELEMENT] = NULL;
    s.metaprogram[CORTO_MEMBER] = cortopy_memberDefMember;
    s.metaprogram[CORTO_BASE] = NULL;
    s.metaprogram[CORTO_OBJECT] = NULL;

    return s;
}


static int
cortopy_buildMemberDefComposite(PyTypeObject* cpType, corto_type cType)
{
    /* Count PyMemberDef's and PyGetSetDef's required */
    struct corto_serializer_s memberCountSerializer = cortopy_memberCountSerializer(
        CORTO_PRIVATE | CORTO_LOCAL, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER
    );
    cortopy_memberCountData memberCountData = {0, 0};
    corto_metaWalk(&memberCountSerializer, cType, &memberCountData);
    size_t memberDefCount = memberCountData.memberDefCount;
    size_t getSetDefCount = memberCountData.getSetDefCount;
    if (memberDefCount == 0 && getSetDefCount == 0) {
        goto finish;
    }

    PyMemberDef* memberDefs = corto_alloc((memberDefCount + 1) * sizeof(PyMemberDef));
    if (memberDefs == NULL) {
        PyErr_NoMemory();
        goto errorAllocMemberDefs;
    }
    memberDefs[memberDefCount] = (PyMemberDef){0};

    PyGetSetDef* getSetDefs = corto_alloc((getSetDefCount + 1) * sizeof(PyGetSetDef));
    if (getSetDefs == NULL) {
        PyErr_NoMemory();
        goto errorAllocGetSetDefs;
    }
    getSetDefs[getSetDefCount] = (PyGetSetDef){0};

    /* Initialize PyMemberDef's and PyGetSetDef's */
    struct corto_serializer_s memberDefSerializer = cortopy_memberDefSerializer(
        CORTO_PRIVATE | CORTO_LOCAL, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER
    );
    PyTypeObject* base = cpType->tp_base;
    cortopy_memberDefSerializerData memberDefSerializerData = {memberDefs, getSetDefs, 0, 0, base->tp_basicsize};
    corto_metaWalk(&memberDefSerializer, cType, &memberDefSerializerData);

    /* Put the PyMemberDef's and PyGetSetDef into the tp_members and tp_getset slots */
    cpType->tp_members = memberDefs;
    cpType->tp_getset = getSetDefs;
finish:

    return 0;
errorAllocGetSetDefs:
    corto_dealloc(memberDefs);
errorAllocMemberDefs:
    return -1;
}


static int
/* tp_members and tp_getset */
cortopy_buildMemberDef(PyTypeObject* cpType, corto_type cType)
{
    switch (cType->kind) {
    case CORTO_PRIMITIVE:
        if (cortopy_buildMemberDefPrimitive(cpType, cType)) {
            goto error;
        }
        break;
    case CORTO_COMPOSITE:
        if (cortopy_buildMemberDefComposite(cpType, cType)) {
            goto error;
        }
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "type not supported");
        break;
    }
    return 0;
error:
    return -1;
}


static PyTypeObject *
cortopy_buildType(corto_type type)
{
    PyObject* baseType = (PyObject*)&cortopy_objectType;
    if (type->kind == CORTO_COMPOSITE) {
        corto_type base = corto_type(corto_interface(type)->base);
        if (base) {
            baseType = cortopy_tryBuildType(base);
            if (baseType == NULL) {
                goto error;
            } else if (!PyType_Check(baseType)) {
                PyErr_SetString(PyExc_RuntimeError, "did not build base type object correctly");
                goto error;
            }
        }
    }
    corto_string typeFullpath = corto_fullpath(NULL, type);
    PyObject* pyTypeFullpath = PyUnicode_FromString(typeFullpath);
    if (pyTypeFullpath == NULL) {
        goto error;
    }

    /* TODO when must tp_name, etc. be deallocated */
    PyTypeObject* cortopyType = corto_calloc(sizeof(PyTypeObject));
    /* Manually do PyVarObject_HEAD_INIT */
    cortopyType->ob_base = (PyVarObject){PyObject_HEAD_INIT(&PyType_Type) 0};
    // TODO what to do with types that are not typical valid identifiers
    // TODO revise proper value https://docs.python.org/2/c-api/typeobj.html?highlight=tp_name#c.PyTypeObject.tp_name
    corto_string tp_name = NULL;
    if (corto_asprintf(&tp_name, "%s", corto_nameof(type)) <= 0) {
        CORTOPY_LASTERR_GOTO(error);
    }
    cortopyType->tp_name = tp_name;
    cortopyType->tp_basicsize = cortopy_sizeForType(type);
    // TODO maybe need FINALIZE flag for deallocating name
    cortopyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    cortopyType->tp_repr = cortopy_typeRepr;
    cortopyType->tp_new = 0;
    cortopyType->tp_base = (PyTypeObject*)baseType;
    /* tp_members and tp_getset */
    if (cortopy_buildMemberDef(cortopyType, type)) {
        goto error;
    }

    // TODO remove the data object
    struct corto_serializer_s s = cortopy_typeSerializer(CORTO_PRIVATE | CORTO_LOCAL, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER);
    corto_metaWalk(&s, type, NULL);

    if (PyType_Ready(cortopyType) < 0) {
        goto error;
    }
    Py_INCREF(cortopyType);

    return cortopyType;
error:
    return NULL;
}


/*
 * Things are added to the typesCache just before they start being serialized
 * so that types with "dependencies" upon themselves don't loop infinitely.
 * This function returns a new reference. I think.
 * Returns None when the type hasn't finished serializing.
 */
static PyObject *
cortopy_tryBuildType(corto_type type)
{
    corto_string typeFullpath = corto_strdup(corto_fullpath(NULL, type));
    if (typeFullpath == NULL) {
        goto errorStrdup;
    }
    PyObject* pyType = PyDict_GetItemString(cortopy_typesCache, typeFullpath);
    if (pyType && pyType != Py_None) {
        Py_INCREF(pyType);
        goto finish;
    } else if (pyType == Py_None) {
        goto finish;
    }

    if (PyDict_SetItemString(cortopy_typesCache, typeFullpath, Py_None)) {
        goto errorSetCacheNone;
    }
    pyType = (PyObject*)cortopy_buildType(type);
    if (pyType == NULL) {
        goto errorSerializeType;
    }

    if (PyDict_SetItemString(cortopy_typesCache, typeFullpath, pyType)) {
        goto errorSetItemType;
    }

finish:
    corto_dealloc(typeFullpath);
    return pyType;
errorSetItemType:
errorSerializeType:
    PyDict_DelItemString(cortopy_typesCache, typeFullpath);
errorSetCacheNone:
    corto_dealloc(typeFullpath);
errorStrdup:
    return NULL;
}


/* END TYPE DESERIALIZER */

static PyObject *
cortopy_getType(PyObject* self, PyObject* args, PyObject* kwargs)
{
    char* kwds[] = {"type_", NULL};
    char* typeName = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:CortoObject", kwds, &typeName)) {
        goto error;
    }

    corto_object type = corto_resolve(NULL, typeName);
    if (type == NULL) {
        PyErr_Format(PyExc_ValueError, "type %s could not be found", typeName);
        goto error;
    }
    PyObject* pyType = (PyObject*)cortopy_tryBuildType(type);
    corto_release(type);
    if (pyType == NULL) {
        goto error;
    } else if ((PyObject*)pyType == Py_None) {
        PyErr_Format(PyExc_RuntimeError, "%s is None", typeName);
        goto error;
    }
    return (PyObject*)pyType;
error:
    return NULL;
}


static PyObject *
cortopy_cortoToCortopyObject(corto_object o)
{
    PyObject* pyo = NULL;
    switch (corto_typeof(o)->kind) {
    case CORTO_PRIMITIVE:
        pyo = cortopy_deserialize_primitive(o);
        break;
    case CORTO_VOID:
        pyo = cortopy_deserialize_void(o);
        break;
    default:
        PyErr_Format(PyExc_RuntimeError, "corto type %s not supported yet", corto_nameof(corto_typeof(o)));
        goto error_typeNotSupported;
    }
    return pyo;
error_typeNotSupported:
    return NULL;
}


static PyObject *
cortopy_resolve(PyObject* self, PyObject* args)
{
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }
    corto_object cortoObj = corto_resolve(NULL, (corto_string)name);
    if (!cortoObj) {
        PyErr_Format(PyExc_ValueError, "could not find %s", name);
        return NULL;
    }
    PyObject* cortopyType = cortopy_tryBuildType(corto_typeof(cortoObj));
    if (cortopyType == NULL) {
        goto error;
    }

    PyObject* pyObj = cortopy_cortoToCortopyObject(cortoObj);
    return pyObj;
error:
    return NULL;
}

static PyObject *
cortopy_eval(PyObject* self, PyObject* args)
{
    char* str = PyUnicode_AsUTF8(args);
    corto_value value;
    if (corto_expr(root_o, str, &value)) {
        PyErr_SetString(PyExc_RuntimeError, "error parsing code");
        goto error;
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static PyMethodDef cortopyMethods[] = {
    {"resolve", cortopy_resolve, METH_VARARGS, "Resolves and constructs and object and returns it"},
    {"nameof", cortopy_nameof, METH_VARARGS, "Returns a name given an address"},
    {"declare_child", (PyCFunction)cortopy_declareChild, METH_VARARGS|METH_KEYWORDS, CORTOPY_DECLARE_CHILD_DOC},
    {"define", (PyCFunction)cortopy_define, METH_O, "Sets the state to defined"},
    {"str", (PyCFunction)cortopy_cortostr, METH_O, "Returns the string serialization of the in-store object"},
    {"gettype", (PyCFunction)cortopy_getType, METH_VARARGS|METH_KEYWORDS, "Serializes a type into a Python object"},
    {"eval", (PyCFunction)cortopy_eval, METH_O},
    {NULL, NULL, 0, NULL}
};


static void
cortopy_free(PyObject* self)
{
    corto_stop();
    Py_XDECREF(MappingProxyType);
    Py_XDECREF(cortopy_typesCache);
    Py_XDECREF(cortopy_typesCacheMappingProxy);
}


static struct PyModuleDef cortopymodule = {
   PyModuleDef_HEAD_INIT,
   "cortopy",   /* name of module */
   NULL,        /* module documentation, may be NULL */
   -1,          /* size of per-interpreter state of the module,
                   or -1 if the module keeps state in global variables. */
   cortopyMethods,
   NULL,
   NULL,
   NULL,
   (freefunc)cortopy_free
};


static int
cortopy_initTypesCacheMappingProxy(void)
{
    PyObject* typesCacheMappingProxyArgs = Py_BuildValue("(O)", cortopy_typesCache);
    cortopy_typesCacheMappingProxy = MappingProxyType->tp_new(MappingProxyType, typesCacheMappingProxyArgs, NULL);
    if (cortopy_typesCacheMappingProxy == NULL) {
        goto error;
    }
    if (MappingProxyType->tp_init(cortopy_typesCacheMappingProxy, typesCacheMappingProxyArgs, NULL)) {
        goto error;
    }
    return 0;
error:
    return -1;
}


PyMODINIT_FUNC
PyInit_cortopy(void)
{
    PyObject *m = PyModule_Create(&cortopymodule);
    if (m == NULL) {
        return NULL;
    }

    cortopy_objectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&cortopy_objectType) < 0) {
        return NULL;
    }
    Py_INCREF(&cortopy_objectType);
    PyModule_AddObject(m, "object", (PyObject *)&cortopy_objectType);

    cortopy_CortoError = PyErr_NewException("cortopy.CortoError", NULL, NULL);
    Py_INCREF(cortopy_CortoError);
    PyModule_AddObject(m, "CortoError", cortopy_CortoError);

    cortopy_typesCache = PyDict_New();
    MappingProxyType = import_MappingProxyType();
    if (MappingProxyType == NULL) {
        goto error;
    }
    if (cortopy_initTypesCacheMappingProxy()) {
        goto error;
    }

    PyModule_AddObject(m, "types", cortopy_typesCacheMappingProxy);

    if (corto_start()) {
        PyErr_SetString(cortopy_CortoError, "could not start object store");
        return NULL;
    }

    return m;
error:
    return NULL;
}
