#include "Python.h"
#include "structmember.h"


#include "corto.h"


typedef struct {
    struct _longobject self;
    const char* name;
} cortopy_int;

// TODO change unsinged lnog long int for ssize_t
// n (int) [Py_ssize_t]
// Convert a Python integer to a C Py_ssize_t.

/*
 * __new__(parent_id, name, x=0, base=10)?
 */
static PyObject*
cortopy_intNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
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
    // type->tp_base->tp_new
    PyObject* o = PyLong_Type.tp_new(type, newArgs, newKwargs);

    ((cortopy_int *)o)->name = name;
    return o;
}

static PyMemberDef cortopy_intMembers[] = {
    {"name", T_STRING, offsetof(cortopy_int, name), READONLY, "name of the object"},
    {NULL}
};


static PyTypeObject cortopy_intType = {
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_LONG_SUBCLASS | Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "int type for corto objects",           /* tp_doc */
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



typedef struct {
    PyObject_HEAD
    const char* name;
} cortopy_object;

static PyObject*
cortopy_objectNew(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"parent_id", "name"};
    const char* parent_id = NULL;
    const char* name = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "zz:CortoObject", kwlist, &parent_id, &name)) return NULL;
    PyObject* o = PyType_GenericNew(type, args, kwds);

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
cortopy_resolve_primitive() {

}

static PyObject *
cortopy_resolve(PyObject* self, PyObject* args)
{
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }
    corto_object o = corto_resolve(NULL, (corto_string)name);
    if (!o) {
        corto_string s;
        corto_asprintf(&s, "could not find %s", name);
        PyErr_SetString(PyExc_RuntimeError, s);
        corto_dealloc(s);
        return NULL;
    }

    PyObject* pyo = Py_None;
    PyObject* newargs;
    PyObject* newkwargs;
    switch (corto_typeof(o)->kind) {
        case CORTO_PRIMITIVE: {
            // TODO make value for parent_id
            newargs = Py_BuildValue("(s s)", "some_parent_id", corto_nameof(o));
            newkwargs = Py_BuildValue("{}");
            if (!newargs) return NULL;
            pyo = cortopy_intNew(&cortopy_intType, newargs, newkwargs);
            if (!pyo) return NULL;
        }
        default:
            PyErr_SetString(PyExc_RuntimeError, "corto type not supported yet");
            return NULL;
    }
    return pyo;
    // return Py_BuildValue("K", (unsigned long long int)o);
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
        PyErr_SetString(PyExc_RuntimeError, "did not provide cortopy.object to cortopy.nameof");
        return NULL;
    }
    return name;
}


static PyMethodDef cortopyMethods[] = {
    {"start", cortopy_start, METH_NOARGS, "Start a corto shell"},
    {"stop", cortopy_stop, METH_NOARGS, "Start a corto shell"},
    {"root", cortopy_root, METH_NOARGS, "Get the address of root"},
    {"resolve", cortopy_resolve, METH_VARARGS, "Resolves and constructs and object and returns it"},
    {"nameof", cortopy_nameof, METH_VARARGS, "Returns a name given an address"},
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef cortopymodule = {
   PyModuleDef_HEAD_INIT,
   "cortopy",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
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

    return m;
}
