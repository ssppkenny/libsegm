#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/npy_3kcompat.h>

#include <structmember.h>
#include <structseq.h>


#include "PageSegmenter.h"

static PyTypeObject GlyphResultType = {0, 0, 0, 0, 0, 0};

static PyStructSequence_Field glyph_result_fields[] = {
    {"x", "x coord"},
    {"y", "y coord"},
    {"width", "width"},
    {"height", "height"},
    {NULL}
};

static PyStructSequence_Desc glyph_result_desc = {
    "glyph_result",
    NULL,
    glyph_result_fields,
    4
};


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

    std::vector<glyph> glyphs;

    if (nd >= 2) {
        cv::Mat mat = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
        //cv::bitwise_not(mat, mat);
        //cv::threshold(c, c, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        PageSegmenter ps(mat);
        glyphs = ps.get_glyphs();
        //cv::imwrite("test.jpg", c);
        Py_DECREF(contig);
    }


/* Use mat here */


    Py_INCREF(input);
    Py_INCREF(dtype);



   //PyObject *nt = PyStructSequence_New(&GlyphResultType);

    int N = glyphs.size();
    PyObject* list = PyList_New(N);
    for (int i = 0; i < N; ++i) {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(glyphs.at(i).x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(glyphs.at(i).y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(glyphs.at(i).width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(glyphs.at(i).height));
        PyList_SetItem(list, i, (PyObject*)res);
    }




    //PyObject *rslt = PyTuple_New(1);
    //PyTuple_SetItem(rslt, 0, (PyObject*)res);
   // PyTuple_SetItem(rslt, 1, PyLong_FromLong(glyphs.at(0).y));
   // PyTuple_SetItem(rslt, 2, PyLong_FromLong(glyphs.at(0).width));
   // PyTuple_SetItem(rslt, 3, PyLong_FromLong(glyphs.at(0).height));
    return list;

    //return input;
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

    PyObject *mod = PyModule_Create(&segm_module);

    if (GlyphResultType.tp_name == 0)
        PyStructSequence_InitType(&GlyphResultType, &glyph_result_desc);
    Py_INCREF((PyObject *) &GlyphResultType);
    PyModule_AddObject(mod, "glyph_result", (PyObject *) &GlyphResultType);
    return mod;
}
