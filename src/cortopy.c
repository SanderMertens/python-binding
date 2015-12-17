#include "Python.h"
#include "structmember.h"


#include "corto.h"


typedef struct {
    struct _longobject self;
    char* name;
} cortopy_int;

// TODO change unsinged lnog long int for ssize_t
// n (int) [Py_ssize_t]
// Convert a Python integer to a C Py_ssize_t.

/*
 * __new__(parent_id, name, x=0, base=10)?
 */
static PyObject*
cortopy_intNew(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
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
    PyObject* o = PyLong_Type.tp_new(subtype, newArgs, newKwargs);

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
    cortopy_intNew,                         /* tp_new */
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
    return Py_BuildValue("K", (unsigned long long int)o);
}


static PyObject *
cortopy_nameof(PyObject* self, PyObject* args)
{
    unsigned long long int p;
    if (!PyArg_ParseTuple(args, "K", &p)) {
        return NULL;
    }
    corto_string name = corto_nameof((corto_object)p);
    if (!name) {
        PyErr_SetString(PyExc_RuntimeError, "could not find object");
        return NULL;
    }
    return Py_BuildValue("s", name);
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

    return m;
}
