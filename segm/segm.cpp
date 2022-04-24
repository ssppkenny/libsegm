#include <Python.h>
#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/npy_3kcompat.h>

#include <structmember.h>
#include <structseq.h>


#include "PageSegmenter.h"
#include "Enclosure.h"
#include "RectJoin.h"
#include "IntervalJoin.h"
#include "segmentation.h"

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

static PyObject* get_neighbors(PyObject* self, PyObject *args) {
    
    PyObject *lst;
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OOO&", &lst, &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }
    int size = PyList_Size(lst);


    std::vector<cv::Rect> rects;
    for (int i = 0; i<size; i++) {
        PyObject* item_object = PyList_GetItem(lst, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }
    

    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    std::vector<std::vector<int>> nn;
    for (int r = 0; r < dims[0]; r++) {
        std::vector<int> v;
        int* row = (int*)PyArray_GETPTR1(contig, r);
        for (int c = 0; c < dims[1]; c++) {
            //int val = mat.at<unsigned int>(r,c);
            int val = row[c];
            v.push_back(val);
        }
        nn.push_back(v);
    }


    std::map<int,std::tuple<cv::Rect,int,double>> nmap = find_neighbors(rects, nn);

    PyObject *ret_val = PyDict_New();

    for (auto it=nmap.begin(); it != nmap.end(); it++) {
        auto k = it->first;
        auto v = it->second;

        cv::Rect r = std::get<0>(v);
        int s = std::get<1>(v);
        double d = std::get<2>(v);

        PyObject* t = PyTuple_New(3);
        
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(r.x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(r.y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(r.width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(r.height));
        
        PyTuple_SetItem(t, 0, (PyObject*)res);

        PyTuple_SetItem(t, 1, PyLong_FromLong(s));
        PyTuple_SetItem(t, 2, PyFloat_FromDouble(d));

        if (r.x == -1) {
            PyDict_SetItem(ret_val, PyLong_FromLong(k), Py_None);
        } else {
            PyDict_SetItem(ret_val, PyLong_FromLong(k), t);
        }
    }

    return ret_val;

}

static PyObject* get_joined_intervals(PyObject* self, PyObject *args) {
    PyObject *list;

    PyArg_ParseTuple(args, "O!", &PyList_Type, &list);

    int size = PyList_Size(list);

    std::vector<std::tuple<int,int,int>> rngs;

    for (int i = 0; i<size; i++) {
        PyObject* obj = PyList_GetItem(list, i);
        long a = PyInt_AsLong(PyTuple_GET_ITEM(obj, 0));
        long b = PyInt_AsLong(PyTuple_GET_ITEM(obj, 1));
        long c = PyInt_AsLong(PyTuple_GET_ITEM(obj, 2));
        rngs.push_back(std::make_tuple(a,b,c));
    }

    std::vector<interval<int>> intervals = join_intervals(rngs);

    PyObject* ret_val = PyList_New(intervals.size());

    for (int i=0; i<intervals.size(); i++) {
        PyObject* t = PyTuple_New(2);
        PyTuple_SetItem(t, 0, PyLong_FromLong(intervals[i].low()));
        PyTuple_SetItem(t, 1, PyLong_FromLong(intervals[i].high()));
        PyList_SetItem(ret_val, i, t);
    }



    return ret_val;
    
}
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

        std::vector<int> areas;
        double sum_of_areas = 0.0;

        for (int i = 1; i < rectComponents.rows; i++)
        {
            int x = rectComponents.at<int>(Point(0, i));
            int y = rectComponents.at<int>(Point(1, i));
            int w = rectComponents.at<int>(Point(2, i));
            int h = rectComponents.at<int>(Point(3, i));
            int area = rectComponents.at<int>(Point(4, i));

            Rect rectangle(x, y, w, h);
            rects.push_back(rectangle);
            areas.push_back(area);
            sum_of_areas += area;
        }

        double avg_area = sum_of_areas / count;

        std::vector<cv::Rect> filtered_rects;
        for (int i = 0; i<rects.size(); i++) 
        {
            cv::Rect r = rects[i];
            int area = areas[i];
            if (area < avg_area / 10) {
                continue;
            } else {
                filtered_rects.push_back(r);
            }
        }

        joined_rects = join_rects(filtered_rects);
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
        cv::threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
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
    {"get_joined_intervals", (PyCFunction) get_joined_intervals, METH_VARARGS, "get_joined_intervals finds all interecting intervals and joines them, returns joined intervals"},
    {"get_neighbors", (PyCFunction) get_neighbors, METH_VARARGS, "get_neighbors finds all neigboring rectangles"},
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
