#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


static PyObject* cortopy_CortoError;

/*
 * Key: Full name of a Corto type
 * Value: None when type is being serialized, or cortopy_type object when ready
 */
static PyObject* cortopy_typesCache;


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


static PyObject*
cortopy_deserialize_boolean(corto_object cortoObj);

static PyObject *
cortopy_deserialize_integer(corto_object cortoObj);

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

static PyTypeObject *
cortopy_tryBuildType(corto_type type);

static corto_int16
cortopy_serializeTypePart(corto_serializer s, corto_value* v, void* data);

static PyTypeObject *
cortopy_buildType(corto_type type);

static PyObject *
cortopy_getType(PyObject* self, PyObject* args, PyObject* kwargs);

static PyObject *
cortopy_cortoToCortopyObject(corto_object o);

static PyObject *
cortopy_resolve(PyObject* self, PyObject* args);

static PyObject *
cortopy_setvalInteger(cortopy_object* self, PyObject* val, corto_type type);

static PyObject *
cortopy_setvalPrimitive(cortopy_object* self, PyObject* val, corto_type type);

static PyObject *
cortopy_setval(cortopy_object* self, PyObject* val);


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
        // TODO better way to get the underlying Corto type
        // TODO maybe we need a metaclass for cortopy types
        name = ((PyTypeObject*)o)->tp_name;
        printf("got type name %s\n", name); // TODO remove
        c = corto_resolve(NULL, (corto_string)name);
    } else if (PyObject_IsInstance(o, (PyObject*)&PyUnicode_Type)) {
        name = PyUnicode_AsUTF8(o);
        c = corto_resolve(NULL, (corto_string)name);
        printf("got str name %s\n", name); // TODO remove
    } else {
        PyErr_Format(PyExc_TypeError, msg, o->ob_type->tp_name);
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
        puts("got root");
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
            PyErr_SetString(cortopy_CortoError, corto_lasterr());
            goto errorCreateChild;
        }
        self->this = *this_p;
    } else {
        self->this = NULL;
    }

    if ((self->name = corto_strdup(name)) == NULL) {
        PyErr_SetString(PyExc_RuntimeError, corto_lasterr());
        goto errorDupName;
    }

    if ((self->parent = corto_strdup(corto_fullpath(NULL, parent))) == NULL) {
        PyErr_SetString(PyExc_RuntimeError, corto_lasterr());
        goto errorDupParentName;
    }
    corto_release(parent);
    parent = NULL;

    if ((self->type = corto_strdup(corto_fullpath(NULL, type))) == NULL) {
        PyErr_SetString(PyExc_RuntimeError, corto_lasterr());
        goto errorDupType;
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

// TODO make receive type...
static int
cortopy_objectInit(cortopy_object* self, PyObject* args, PyObject* kwargs)
{
    static char* kwds[] = {"parent", "name", "type", NULL};
    // corto_string parentName = NULL;
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
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto errorDefine;
    }

    return 0;
errorDefine:
errorInitExt:
errorNeitherScopedNotAnonymous:
errorParseTupleAndKeywords:
    return -1;
}


static PyObject *
cortopy_objectUpdate(cortopy_object* self, PyObject* args, PyObject* kwargs)
{
    // TODO get rid of args and kwargs
    corto_update(self->this);
    Py_RETURN_NONE;
}


static PyMethodDef cortopy_objectMethods[] = {
    {"update", (PyCFunction)cortopy_objectUpdate, METH_VARARGS|METH_KEYWORDS, "updates the value"},
    {"setval", (PyCFunction)cortopy_setval, METH_O, ""},
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




typedef struct {
    cortopy_object self;
    PyObject* value;
} cortopy_int;


static struct PyMemberDef cortopy_intMembers[] = {
    {NULL}
};


/*
 * __new__(parent_id, name, x=0, base=10)?
 * TODO replace this for an __init__ method since we do not want it to be treated as immutable.
 */
static int
cortopy_intInit(cortopy_int* self, PyObject* args, PyObject* kwargs)
{
    static char *kwds[] = {"parent", "name", "x", NULL};
    corto_string parentName = NULL;
    corto_string name = NULL;
    long long int x = FALSE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "zz|L:bool", kwds, &parentName, &name, &x)) {
        goto errorParseTupleAndKeywords;
    }

    corto_object this = NULL;
    if (cortopy_objectInitExt((cortopy_object*)self, parentName, name, corto_type(corto_int64_o), &this) || this == NULL) {
        goto errorInitExt;
    }

    self->value = Py_BuildValue("L", x);

    if (x) {
        Py_INCREF(Py_True);
        self->value = Py_True;
    } else {
        Py_INCREF(Py_False);
        self->value = Py_False;
    }

    *(corto_int64*)this = x ? TRUE : FALSE;
    if (corto_define(this)) {
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto errorCortoDefine;
    }

    return 0;
errorCortoDefine:
errorInitExt:
errorParseTupleAndKeywords:
    return -1;
}


PyTypeObject cortopy_intType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cortopy.int",             /* tp_name */
    sizeof(cortopy_int),       /* tp_basicsize */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "int type for corto objects", /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    cortopy_intMembers,                         /* tp_members */
    0,                         /* tp_getset */
    &cortopy_objectType,              /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)cortopy_intInit,                         /* tp_init */
    0,                         /* tp_alloc */
};


typedef struct {
    cortopy_object self;
    PyObject* value; /* Py_True or Py_False */
} cortopy_bool;

PyTypeObject cortopy_boolType;


/*
 * For now, declares and defines a bool object.
 * cortopy.bool("/", "myBool", True)
 * We may solely declare and define objects in different steps in the future.
 */
static int
cortopy_boolInit(cortopy_bool* self, PyObject* args, PyObject* kwargs)
{
    static char *kwds[] = {"parent", "name", "x", 0};
    corto_string parentName = NULL;
    corto_string name = NULL;
    int x = FALSE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "zz|p:bool", kwds, &parentName, &name, &x)) {
        goto errorParseTupleAndKeywords;
    }

    corto_object this = NULL;
    if (cortopy_objectInitExt((cortopy_object*)self, parentName, name, corto_type(corto_bool_o), &this) || this == NULL) {
        goto errorInitExt;
    }

    self->value = 0;

    if (x) {
        Py_INCREF(Py_True);
        self->value = Py_True;
    } else {
        Py_INCREF(Py_False);
        self->value = Py_False;
    }

    *(corto_bool*)this = x ? TRUE : FALSE;
    if (corto_define(this)) {
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto errorCortoDefine;
    }

    return 0;
errorCortoDefine:
errorInitExt:
errorParseTupleAndKeywords:
    return -1;
}


static PyObject*
cortopy_deserialize_boolean(corto_object cortoObj)
{
    corto_type type = corto_typeof(cortoObj);
    corto_assert(type->kind == CORTO_PRIMITIVE && corto_primitive(type)->kind == CORTO_BOOLEAN, "not an instance of a boolean type");

    corto_string parentFullpath = corto_fullpath(NULL, corto_parentof(cortoObj));
    corto_string name = corto_nameof(cortoObj);
    PyObject* newArgs = NULL;
    if (corto_checkState(cortoObj, CORTO_DEFINED)) {
        newArgs = Py_BuildValue("(s s p)", parentFullpath, name, *(corto_bool*)cortoObj);
    } else {
        newArgs = Py_BuildValue("(s s)", parentFullpath, name);
    }
    if (newArgs == NULL) {
        goto errorNewArgs;
    }
    PyObject* newKwargs = PyDict_New();
    if (newKwargs == NULL) {
        goto errorNewDict;
    }
    PyObject* pyObj = cortopy_boolType.tp_new(&cortopy_boolType, newArgs, newKwargs);
    if (pyObj == NULL) {
        goto errorNew;
    }
    if (cortopy_boolType.tp_init(pyObj, newArgs, newKwargs)) {
        goto errorInit;
    }
    Py_XDECREF(newKwargs);
    Py_XDECREF(newArgs);
    return pyObj;
errorInit:
errorNew:
    Py_XDECREF(newKwargs);
errorNewDict:
    Py_XDECREF(newArgs);
errorNewArgs:
    return NULL;
}


PyMethodDef cortopy_boolMethods[] = {
    {NULL}
};

// TODO maybe change the value to T_BOOL
PyMemberDef cortopy_boolMembers[] = {
    {"value", T_OBJECT, offsetof(cortopy_bool, value), READONLY, "True or False"},
    {NULL}
};


PyTypeObject cortopy_boolType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cortopy.bool",             /* tp_name */
    sizeof(cortopy_bool),       /* tp_basicsize */
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
    0,                         /*  tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_FINALIZE,/* tp_flags */
    "bool type for corto objects", /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    cortopy_boolMethods,                         /* tp_methods */
    cortopy_boolMembers,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)&cortopy_boolInit,         /* tp_init */
    0,                         /* tp_alloc */
};


static PyObject *
cortopy_deserialize_integer(corto_object cortoObj)
{
    corto_type type = corto_typeof(cortoObj);
    if (type->kind != CORTO_PRIMITIVE && corto_primitive(type)->kind != CORTO_INTEGER) {
        PyErr_SetString(PyExc_ValueError, "expected corto integer");
        return NULL;
    }

    PyObject* intNewArgs = Py_BuildValue("(s s L)", corto_fullpath(NULL, cortoObj), corto_nameof(cortoObj), *(corto_int64 *)cortoObj);
    if (!intNewArgs) return NULL;

    PyObject* intNewKwargs = PyDict_New();
    if (!intNewKwargs) return NULL;

    // PyObject* pyObj = cortopy_intInit(&cortopy_intType, intNewArgs, intNewKwargs);
    // if (!pyObj) return NULL;
    // if (cortopy_intType.tp_init(pyObj, intNewArgs, intNewKwargs)) return NULL;

    return NULL;
}

static PyObject *
cortopy_deserialize_primitive(corto_object cortoObj)
{
    corto_assert(corto_typeof(cortoObj)->kind != CORTO_PRIMITIVE, "expected corto primitive");

    PyObject* pyObj;
    switch (corto_primitive(corto_typeof(cortoObj))->kind) {
    case CORTO_INTEGER:
        pyObj = cortopy_deserialize_integer(cortoObj);
        break;
    case CORTO_BOOLEAN:
        pyObj = cortopy_deserialize_boolean(cortoObj);
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
    PyObject* newkwargs = Py_BuildValue("{}");
    if (!newkwargs) {
        goto errorNewKwargs;
    }
    PyObject* pyObj = cortopy_objectType.tp_new(&cortopy_objectType, newargs, newkwargs);
    if (pyObj == NULL) {
        goto errorNew;
    }
    if (cortopy_objectType.tp_init(pyObj, newargs, newkwargs)) {
        goto errorInit;
    }
    return pyObj;
errorInit:
    Py_DECREF(pyObj);
errorNew:
    Py_DECREF(newkwargs);
errorNewKwargs:
    Py_DECREF(newargs);
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
    if (Py_TYPE(o) == &cortopy_intType) {
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
    puts("1");

    corto_object o = corto_declareChild(parent, name, type);
    if (!o) {
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto errorDeclareChild;
    }
    puts("2");

    PyTypeObject* cpType = cortopy_tryBuildType(type);
    if (cpType == NULL) {
        goto errorTryBuildType;
    }
    puts("3");
    PyObject* cpObj;
    PyObject* newargs = Py_BuildValue("(s s s)", corto_fullpath(NULL, parent), name, corto_fullpath(NULL, type));
    PyObject* newkwargs = Py_BuildValue("{}");
    cpObj = cpType->tp_new(cpType, newargs, newkwargs);
    if (cpObj == NULL) {
        goto errorNew;
    }
    if (cpType->tp_init(cpObj, newargs, newkwargs)) {
        goto errorInit;
    }
    puts("4");

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
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto error;
    }
    Py_RETURN_NONE;
error:
    return NULL;
}




static PyObject *
cortopy_setvalInteger(cortopy_object* self, PyObject* val, corto_type type)
{
    int isInteger = PyObject_IsInstance(val, (PyObject *)&PyLong_Type);
    if (isInteger == 1) {
        int overflow = 0;
        signed long long int cVal = PyLong_AsLongLongAndOverflow(val, &overflow);
        signed long long int max = corto_int(type)->max;
        signed long long int min = corto_int(type)->min;
        if (overflow || cVal > max || cVal < min) {
            PyErr_Format(PyExc_ValueError, "value must be between %lld and %lld, have %S", min, max, val);
            goto error;
        }
        *(signed long long int*)(self->this) = cVal;
    } else if (isInteger == 0) {
        PyErr_Format(PyExc_TypeError, "setval argument 1 must be int not %s", val->ob_type->tp_name);
        goto error;
    } else {
        goto error;
    }
    Py_RETURN_NONE;
error:
    return NULL;
}


static PyObject *
cortopy_setvalPrimitive(cortopy_object* self, PyObject* val, corto_type type)
{
    PyObject* result = NULL;
    switch(corto_primitive(type)->kind) {
    case CORTO_INTEGER:
        result = cortopy_setvalInteger(self, val, type);
        break;
    default:
        PyErr_Format(PyExc_TypeError, "setting value not supported for %s", corto_fullpath(NULL, type));
    }
    return result;
}


static PyObject *
cortopy_setval(cortopy_object* self, PyObject* val)
{
    corto_type type = corto_typeof(self->this);
    PyObject* result = NULL;
    switch (type->kind) {
    case CORTO_PRIMITIVE:
        result = cortopy_setvalPrimitive(self, val, type);
        break;
    default:
        PyErr_Format(PyExc_TypeError, "setting value not supported for %s", corto_fullpath(NULL, type));
    }
    return result;
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
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto error;
    }
    PyObject* pystr = PyUnicode_FromString(str);
    corto_dealloc(str);
    return pystr;
error:
    return NULL;
}


/* START TYPE DESERIALIZER */

static PyTypeObject *
cortopy_tryBuildType(corto_type type);


typedef struct cortopy_typeSerializerData
{

} cortopy_typeSerializerData;


static corto_int16
cortopy_serializeTypePart(corto_serializer s, corto_value* v, void* data)
{
    cortopy_typeSerializerData* _data = data;
    CORTO_UNUSED(_data);
    corto_type type = corto_valueType(v);
    PyTypeObject* pyType = cortopy_tryBuildType(type);
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
    // s.program[CORTO_COLLECTION] = NULL;
    s.metaprogram[CORTO_ELEMENT] = cortopy_serializeTypePart;
    s.metaprogram[CORTO_MEMBER] = cortopy_serializeTypePart;
    s.metaprogram[CORTO_BASE] = NULL;
    s.metaprogram[CORTO_OBJECT] = NULL;

    return s;
}

static size_t
cortopy_sizeForType(corto_type type)
{
    return sizeof(cortopy_object) + type->size; // TODO get the size of the Corto type
}


static PyObject *
cortopy_typeRepr(PyObject* o)
{
    return Py_BuildValue("s", corto_fullpath(NULL, ((cortopy_object*)o)->this));
}


static PyTypeObject *
cortopy_buildType(corto_type type)
{
    PyTypeObject* baseType = &cortopy_objectType;
    // if (type->kind == CORTO_COMPOSITE) {

    //     PyTypeObject* baseType = cortopy_tryBuildType((corto_type)(corto_interface(type)->base));
    //     if (baseType == NULL) {
    //         goto error;
    //     }
    // }

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
    corto_string tp_name;
    if (corto_asprintf(&tp_name, "%s", typeFullpath) <= 0) {
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto error;
    }
    cortopyType->tp_name = tp_name;
    cortopyType->tp_basicsize = cortopy_sizeForType(type);
    // TODO maybe need FINALIZE flag for deallocating name
    cortopyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    cortopyType->tp_repr = cortopy_typeRepr;
    cortopyType->tp_new = PyType_GenericNew;
    cortopyType->tp_base = baseType;

    struct corto_serializer_s s = cortopy_typeSerializer(CORTO_PRIVATE | CORTO_LOCAL, CORTO_NOT, CORTO_SERIALIZER_TRACE_NEVER);
    cortopy_typeSerializerData data = {};
    corto_metaWalk(&s, type, &data);

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
 */
static PyTypeObject *
cortopy_tryBuildType(corto_type type)
{
    corto_string typeFullpath = corto_fullpath(NULL, type);

    PyObject* pyType = PyDict_GetItemString(cortopy_typesCache, typeFullpath);
    if (pyType) {
        Py_INCREF(pyType);
        goto finish;
    }

    if (PyDict_SetItemString(cortopy_typesCache, typeFullpath, Py_None)) {
        puts(" ERROR TRY BUID TYPE SET ITEM STRING");
        goto errorSetCacheNone;
    }
    pyType = (PyObject*)cortopy_buildType(type); // TODO check if serializeType increased ref
    if (pyType == NULL) {
        goto errorSerializeType;
    }
    // TODO can the value for the key be replaced faster?
    if (PyDict_DelItemString(cortopy_typesCache, typeFullpath)) {
        goto errorDelItemNone;
    }
    if (PyDict_SetItemString(cortopy_typesCache, typeFullpath, pyType)) {
        goto errorSetItemType;
    }

finish:
    return (PyTypeObject*)pyType;
errorSetItemType:
errorDelItemNone:
errorSerializeType:
    PyDict_DelItemString(cortopy_typesCache, typeFullpath);
errorSetCacheNone:
    Py_DECREF(Py_None);
    return NULL;
}


/* END TYPE DESERIALIZER */

static PyObject *
cortopy_getType(PyObject* self, PyObject* args, PyObject* kwargs)
{
    char* kwds[] = {"type_", NULL};
    char* typeName;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:CortoObject", kwds, &typeName)) {
        goto error;
    }

    corto_object type = corto_resolve(NULL, typeName);
    if (type == NULL) {
        PyErr_Format(PyExc_ValueError, "type %s could not be found", typeName);
        goto error;
    }
    PyTypeObject* pyType = cortopy_tryBuildType(type);
    corto_release(type);
    if (pyType == NULL) {
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

    PyTypeObject* cortopyType = cortopy_tryBuildType(corto_typeof(cortoObj));
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

static PyObject *
cortopy_getTypesCache(PyObject* self, PyObject* args)
{
    return cortopy_typesCache;
}


static PyMethodDef cortopyMethods[] = {
    {"resolve", cortopy_resolve, METH_VARARGS, "Resolves and constructs and object and returns it"},
    {"nameof", cortopy_nameof, METH_VARARGS, "Returns a name given an address"},
    {"declare_child", (PyCFunction)cortopy_declareChild, METH_VARARGS|METH_KEYWORDS, CORTOPY_DECLARE_CHILD_DOC},
    {"define", (PyCFunction)cortopy_define, METH_O, "Sets the state to defined"},
    {"str", (PyCFunction)cortopy_cortostr, METH_O, "Returns the string serialization of the in-store object"},
    {"gettype", (PyCFunction)cortopy_getType, METH_VARARGS|METH_KEYWORDS, "Serializes a type into a Python object"},
    {"eval", (PyCFunction)cortopy_eval, METH_O},
    {"types", (PyCFunction)cortopy_getTypesCache, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}
};


static void
cortopy_free(PyObject* self)
{
    corto_stop();
}


// TODO free the cortopy_typesCache in m_free
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



int cortopy_testFixture() {

    /* Create a new struct type */
    corto_struct Point = corto_structDeclareChild(NULL, "Point");
    corto_memberCreateChild(Point, "x", corto_int32_o, 0);
    corto_memberCreateChild(Point, "y", corto_int32_o, 0);
    corto_structDefine(Point, NULL, 0);

    /* Create an object with the struct type */
    corto_object p = corto_createChild(NULL, "p", Point);

    /* Dynamically assign a value to it */
    corto_fromStr(&p, "{10, 20}");

    /* Print the value to the console */
    corto_string value = corto_str(p, 0);
    corto_dealloc(value);

    return 0;
}

PyMODINIT_FUNC
PyInit_cortopy(void)
{
    PyObject *m = PyModule_Create(&cortopymodule);
    if (m == NULL) {
        return NULL;
    }

    cortopy_intType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&cortopy_intType) < 0) {
        return NULL;
    }
    Py_INCREF(&cortopy_intType);
    PyModule_AddObject(m, "int", (PyObject *)&cortopy_intType);

    cortopy_objectType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&cortopy_objectType) < 0) {
        return NULL;
    }
    Py_INCREF(&cortopy_objectType);
    PyModule_AddObject(m, "object", (PyObject *)&cortopy_objectType);

    cortopy_boolType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&cortopy_boolType) < 0) {
        return NULL;
    }
    Py_INCREF(&cortopy_boolType);
    PyModule_AddObject(m, "bool", (PyObject *)&cortopy_boolType);

    cortopy_CortoError = PyErr_NewException("cortopy.CortoError", NULL, NULL);
    Py_INCREF(cortopy_CortoError);
    PyModule_AddObject(m, "CortoError", cortopy_CortoError);

    cortopy_typesCache = PyDict_New();

    if (corto_start()) {
        PyErr_SetString(cortopy_CortoError, "could not start object store");
        return NULL;
    }

    cortopy_testFixture();

    return m;
}
