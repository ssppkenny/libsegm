#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/npy_3kcompat.h>
#include "PageSegmenter.h"

// This is the definition of a method
static PyObject* reflow(PyObject* self, PyObject *args) {
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype)) {
        return NULL;
    }


    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
        dtype,
        1, 3, NPY_ARRAY_CARRAY, NULL);

    if (nd >= 2) {

        cv::Mat mat = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
        cv::bitwise_not(mat, mat);
        cv::threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        PageSegmenter ps(mat);
        std::vector<glyph> glyphs = ps.get_glyphs();
        cv::imwrite("test.jpg", mat);
        Py_DECREF(contig); 
    }


/* Use mat here */


    Py_INCREF(input);
    Py_INCREF(dtype);
    return input;
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
    import_array();
    return PyModule_Create(&segm_module);
}
