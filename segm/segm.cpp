#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/npy_3kcompat.h>

#include <structmember.h>
#include <structseq.h>


#include "PageSegmenter.h"
#include "Enclosure.h"
#include "RectJoin.h"

static PyTypeObject GlyphResultType = {0, 0, 0, 0, 0, 0};

static PyStructSequence_Field glyph_result_fields[] =
{
    {"x", "x coord"},
    {"y", "y coord"},
    {"width", "width"},
    {"height", "height"},
    {NULL}
};

static PyStructSequence_Desc glyph_result_desc =
{
    "glyph_result",
    NULL,
    glyph_result_fields,
    4
};

static PyObject* get_joined_rects(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }


    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    std::vector<cv::Rect> rects;
    std::vector<cv::Rect> joined_rects;
    std::set<std::array<int,4>> enclosing_rects;
    if (nd >= 2)
    {
        cv::Mat mat = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
        threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        Mat labeled(mat.size(), mat.type());
        Mat rectComponents = Mat::zeros(Size(0, 0), 0);
        Mat centComponents = Mat::zeros(Size(0, 0), 0);
        connectedComponentsWithStats(mat, labeled, rectComponents, centComponents);
        int count = rectComponents.rows - 1;

        for (int i = 1; i < rectComponents.rows; i++)
        {
            int x = rectComponents.at<int>(Point(0, i));
            int y = rectComponents.at<int>(Point(1, i));
            int w = rectComponents.at<int>(Point(2, i));
            int h = rectComponents.at<int>(Point(3, i));

            Rect rectangle(x, y, w, h);
            rects.push_back(rectangle);
        }

        joined_rects = join_rects(rects);
        Enclosure enc(joined_rects);
        enclosing_rects = enc.solve();

        Py_DECREF(contig);
    }


    Py_INCREF(input);
    Py_INCREF(dtype);

    std::vector<cv::Rect> new_rects;
    for (auto it = enclosing_rects.begin(); it != enclosing_rects.end(); ++it)
    {
        array<int, 4> a = *it;
        int x = -get<0>(a);
        int y = -get<1>(a);
        int width = get<2>(a) - x;
        int height = get<3>(a) - y;
        new_rects.push_back(cv::Rect(x,y,width,height));

    }

    int N = new_rects.size();
    PyObject* list = PyList_New(N);
    for (int i = 0; i < N; ++i)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(new_rects[i].x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(new_rects[i].y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(new_rects[i].width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(new_rects[i].height));
        PyList_SetItem(list, i, (PyObject*)res);
    }

    return list;
}

// This is the definition of a method
static PyObject* get_glyphs(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }


    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    std::vector<glyph> glyphs;

    if (nd >= 2)
    {
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
    for (int i = 0; i < N; ++i)
    {
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
static PyMethodDef method_table[] =
{
    {"get_glyphs", (PyCFunction) get_glyphs, METH_VARARGS, "get_glyphs finds all glyphs (letters) in an image"},
    {"get_joined_rects", (PyCFunction) get_joined_rects, METH_VARARGS, "get_joined_rects finds all interecting rectangles and joines them, returns joined rectangles"},
    {NULL, NULL, 0, NULL} // Sentinel value ending the table
};

// A struct contains the definition of a module
static struct PyModuleDef segm_module_def =
{
    PyModuleDef_HEAD_INIT,
    "_segm", // Module name
    "image segmentation module",
    -1,   // Optional size of the module state memory
    method_table
};

// The module init function
PyMODINIT_FUNC PyInit__segm(void)
{
    import_array();

    PyObject *mod = PyModule_Create(&segm_module_def);

    if (GlyphResultType.tp_name == 0)
        PyStructSequence_InitType(&GlyphResultType, &glyph_result_desc);
    Py_INCREF((PyObject *) &GlyphResultType);
    PyModule_AddObject(mod, "glyph_result", (PyObject *) &GlyphResultType);
    return mod;
}
