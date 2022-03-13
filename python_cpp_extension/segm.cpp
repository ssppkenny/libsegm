#include <Python.h>
#include "PageSegmenter.h"

// This is the definition of a method
static PyObject* reflow(PyObject *self, PyObject *args) {
    return PyLong_FromLong(0);
}

// Exported methods are collected in a table
PyMethodDef method_table[] = {
    {"reflow", (PyCFunction) reflow, METH_VARARGS, "reflow function reads an image ndarray and returns a reflowed image ndarray"},
    {NULL, NULL, 0, NULL} // Sentinel value ending the table
};

// A struct contains the definition of a module
PyModuleDef segm_module = {
    PyModuleDef_HEAD_INIT,
    "segm", // Module name
    "image segmentation module",
    -1,   // Optional size of the module state memory
    method_table,
    NULL, // Optional slot definitions
    NULL, // Optional traversal function
    NULL, // Optional clear function
    NULL  // Optional module deallocation function
};

// The module init function
PyMODINIT_FUNC PyInit_segm(void) {
    return PyModule_Create(&segm_module);
}
