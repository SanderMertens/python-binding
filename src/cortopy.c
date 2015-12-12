#include "Python.h"

#include "corto.h"


static PyObject *
cortopy_root(PyObject *self, PyObject *args)
{
    PyObject* root = Py_BuildValue("i", root_o);
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
    return Py_BuildValue("i", o);
}


static PyObject *
cortopy_nameof(PyObject* self, PyObject* args)
{
    int p;
    if (!PyArg_ParseTuple(args, "i", &p)) {
        return NULL;
    }
    printf("lang is in %d\n", p);
    printf("size of int: %d, size of pointer: %d\n", sizeof(int), sizeof(void*));
    printf("as a pointer: %d\n", (int)(void*)p);
    corto_string name = corto_nameof((corto_object)p);
    // if (!name) {
    //     PyErr_SetString(PyExc_RuntimeError, "could not find object");
    //     return NULL;
    // }
    return Py_BuildValue("s", "ye");
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
    PyObject *m;

    m = PyModule_Create(&cortopymodule);
    if (m == NULL) {
        return NULL;
    }

    return m;
}
