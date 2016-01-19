#include "Python.h"
#include "structmember.h"

#include "corto/corto.h"


static PyObject* cortopy_CortoError;


typedef struct {
    struct _longobject self;
    const char* name;
} cortopy_int;


static struct PyMemberDef cortopy_intMembers[] = {
    {"name", T_STRING, offsetof(cortopy_int, name), READONLY, "name of the object"},
    {NULL}
};


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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_LONG_SUBCLASS | Py_TPFLAGS_BASETYPE, /* tp_flags */
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
    &PyLong_Type,              /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
};


// TODO change unsinged lnog long int for ssize_t
// n (int) [Py_ssize_t]
// Convert a Python integer to a C Py_ssize_t.

/*
 * __new__(parent_id, name, x=0, base=10)?
 */
PyObject*
cortopy_intNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"parent_id", "name", "x", "base", 0};
    const char* parent_id = NULL;
    const char* name = NULL;
    PyObject* x = NULL;
    PyObject* obase = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "zz|OO:int", kwlist, &parent_id, &name, &x, &obase)) return NULL;

    Py_ssize_t newArgsSize = PyTuple_Size(args) - 2;
    PyObject* newArgs = PyTuple_New(newArgsSize);
    PyObject* newKwargs = PyDict_New();
    if (newArgsSize > 0) {
        if (PyTuple_SetItem(newArgs, 0, x)) return NULL;
        if (newArgsSize > 1) {
            if (PyTuple_SetItem(newArgs, 1, obase)) return NULL;
        }
    } else {
        if (PyDict_SetItemString(newKwargs, "x", x)) return NULL;
        if (PyDict_SetItemString(newKwargs, "obase", obase)) return NULL;
    }

    PyObject* o = type->tp_base->tp_new(type, newArgs, newKwargs);

    ((cortopy_int *)o)->name = name;
    return o;
}


typedef struct {
    PyObject_HEAD
    const char* name;
} cortopy_object;


static PyObject*
cortopy_objectNew(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    static char *kwds[] = {"parent_id", "name"};
    const char* parent_id = NULL;
    const char* name = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "zz:CortoObject", kwds, &parent_id, &name)) return NULL;
    PyObject* o = PyType_GenericNew(type, args, kwargs);

    ((cortopy_object *)o)->name = name;
    return o;
}


static PyMemberDef cortopy_objectMembers[] = {
    {"name", T_STRING, offsetof(cortopy_object, name), READONLY, "name of the object"},
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
    0,                         /* tp_methods */
    cortopy_objectMembers,                         /* tp_members */
    0,                         /* tp_getset */
    0,              /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
};


static PyObject *
cortopy_root(PyObject *self, PyObject *args)
{
    PyObject* root = Py_BuildValue("K", (unsigned long long int)root_o);
    return root;
}


static PyObject *
cortopy_start(PyObject* self, PyObject* args)
{
    if (corto_start()) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
cortopy_stop(PyObject* self, PyObject* args)
{
    corto_stop();
    Py_RETURN_NONE;
}


static PyObject *
cortopy_resolve_integer(corto_object cortoObj)
{
    corto_type type = corto_typeof(cortoObj);
    if (type->kind != CORTO_PRIMITIVE && corto_primitive(type)->kind != CORTO_INTEGER) {
        PyErr_SetString(PyExc_RuntimeError, "expected corto integer");
        return NULL;
    }

    // TODO how do we allocate / deallocate the parent id?
    PyObject* intNewArgs = Py_BuildValue("(s s L)", "parent_id", corto_nameof(cortoObj), *(corto_int64 *)cortoObj);
    if (!intNewArgs) return NULL;

    PyObject* intNewKwargs = PyDict_New();
    if (!intNewKwargs) return NULL;

    PyObject* pyObj = cortopy_intNew(&cortopy_intType, intNewArgs, intNewKwargs);
    if (!pyObj) return NULL;
    if (cortopy_intType.tp_init(pyObj, intNewArgs, intNewKwargs)) return NULL;

    return pyObj;
}


static PyObject *
cortopy_resolve_primitive(corto_object cortoObj)
{
    // TODO replace for assertion
    corto_type type = corto_typeof(cortoObj);
    if (type->kind != CORTO_PRIMITIVE) {
        PyErr_SetString(PyExc_RuntimeError, "expected corto primitive");
        return NULL;
    }
    PyObject* pyObj;
    switch (corto_primitive(type)->kind) {
    case CORTO_INTEGER:
        pyObj = cortopy_resolve_integer(cortoObj);
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "primitive type not supported");
        pyObj = NULL;
        break;
    }
    return pyObj;
}

static PyObject *
cortopy_resolve_void(corto_object cortoObj)
{
    // TODO replace for assertion
    corto_type type = corto_typeof(cortoObj);
    if (type->kind != CORTO_PRIMITIVE) {
        PyErr_SetString(PyExc_RuntimeError, "expected corto primitive");
        return NULL;
    }
    PyObject* newargs = Py_BuildValue("(s s)", "parent_id", corto_nameof(cortoObj));
    if (!newargs) return NULL;
    PyObject* newkwargs = Py_BuildValue("{}");
    if (!newkwargs) return NULL;
    PyObject* pyObj = cortopy_objectNew(&cortopy_objectType, newargs, newkwargs);
    if (!pyObj) {
        Py_DECREF(newargs);
        Py_DECREF(newkwargs);
        return NULL;
    }
    if (cortopy_objectType.tp_init(pyObj, newargs, newkwargs)) {
        Py_DECREF(pyObj);
        return NULL;
    }
    return pyObj;
}


static PyObject *
cortopy_deserialize(corto_object o)
{
    PyObject* pyo = NULL;
    switch (corto_typeof(o)->kind) {
    case CORTO_PRIMITIVE:
        pyo = cortopy_resolve_primitive(o);
        break;
    case CORTO_VOID:
        pyo = cortopy_resolve_void(o);
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

    PyObject* pyObj = cortopy_deserialize(cortoObj);
    return pyObj;
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
        name = PyUnicode_FromString(((cortopy_int*)o)->name);
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
    static char* kwds[] = {"parent", "name", "type"};
    static char* argFmt = "zss|";
    char* parentName = NULL;
    char* name = NULL;
    char* typeName = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, argFmt, kwds, &parentName, &name, &typeName)) {
        goto error_parseTupleAndKeywords;
    }

    corto_object parent = NULL;
    if (parentName) {
        parent = corto_resolve(NULL, parentName);
    } else {
        if (corto_claim(root_o)) {
            PyErr_SetString(cortopy_CortoError, corto_lasterr());
            goto error_claimRoot;
        }
        parent = root_o;
    }
    if (!parent) {
        PyErr_Format(PyExc_ValueError, "could not find parent \"%s\"", parentName);
        goto error_parentNotFound;
    }

    corto_type type = corto_resolve(NULL, typeName);
    if (!type) {
        PyErr_Format(PyExc_ValueError, "could not find type \"%s\"", parentName);
        goto error_typeNotFound;
    }

    corto_object o = corto_declareChild(parent, name, type);
    if (!o) {
        PyErr_SetString(cortopy_CortoError, corto_lasterr());
        goto error_declareChild;
    }
    PyObject* pyObj = cortopy_deserialize(o);

    corto_release(type);
    corto_release(parent);
    return pyObj;

error_declareChild:
    corto_release(type);
error_typeNotFound:
    corto_release(parent);
error_parentNotFound:
error_claimRoot:
error_parseTupleAndKeywords:
    return NULL;
}


static PyMethodDef cortopyMethods[] = {
    {"start", cortopy_start, METH_NOARGS, "Start a corto shell"},
    {"stop", cortopy_stop, METH_NOARGS, "Start a corto shell"},
    {"root", cortopy_root, METH_NOARGS, "Get the address of root"},
    {"resolve", cortopy_resolve, METH_VARARGS, "Resolves and constructs and object and returns it"},
    {"nameof", cortopy_nameof, METH_VARARGS, "Returns a name given an address"},
    {"declare_child", (PyCFunction)cortopy_declareChild, METH_VARARGS|METH_KEYWORDS, CORTOPY_DECLARE_CHILD_DOC},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef cortopymodule = {
   PyModuleDef_HEAD_INIT,
   "cortopy",   /* name of module */
   NULL,        /* module documentation, may be NULL */
   -1,          /* size of per-interpreter state of the module,
                   or -1 if the module keeps state in global variables. */
   cortopyMethods
};


PyMODINIT_FUNC
PyInit_cortopy(void)
{
    PyObject *m = PyModule_Create(&cortopymodule);
    if (m == NULL) return NULL;

    cortopy_intType.tp_new = cortopy_intNew;
    if (PyType_Ready(&cortopy_intType) < 0) return NULL;
    Py_INCREF(&cortopy_intType);
    PyModule_AddObject(m, "int", (PyObject *)&cortopy_intType);

    cortopy_objectType.tp_new = cortopy_objectNew;
    if (PyType_Ready(&cortopy_objectType) < 0) return NULL;
    Py_INCREF(&cortopy_objectType);
    PyModule_AddObject(m, "object", (PyObject *)&cortopy_objectType);

    cortopy_CortoError = PyErr_NewException("cortopy.CortoError", NULL, NULL);
    Py_INCREF(cortopy_CortoError);
    PyModule_AddObject(m, "CortoError", cortopy_CortoError);

    return m;
}
