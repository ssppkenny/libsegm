#include <Python.h>

#include <numpy/ndarraytypes.h>
#include <numpy/ufuncobject.h>
#include <numpy/npy_3kcompat.h>

#include <structmember.h>
#include <structseq.h>
#include <boost/python.hpp>

#include "PageSegmenter.h"
#include "Enclosure.h"
#include "RectJoin.h"
#include "IntervalJoin.h"
#include "segmentation.h"


namespace bp = boost::python;

static PyTypeObject GlyphResultType = {0, 0, 0, 0, 0, 0};

static PyStructSequence_Field glyph_result_fields[] =
{
    {"x", "x coord"},
    {"y", "y coord"},
    {"width", "width"},
    {"height", "height"},
    {"shift", "shift"},
    {NULL}
};

static PyStructSequence_Desc glyph_result_desc =
{
    "glyph_result",
    NULL,
    glyph_result_fields,
    5
};

static PyObject* get_bounding_rects_for_words(PyObject* self, PyObject *args)
{

    PyObject *words;
    PyArg_ParseTuple(args, "O!", &PyList_Type, &words);

    std::vector<std::vector<cv::Rect>> ws;

    int size = PyList_Size(words);
    for (int i=0; i<size; i++)
    {
        PyObject* list_item = PyList_GetItem(words, i);
        int ssize = PyList_Size(list_item);
        std::vector<cv::Rect> word;
        for (int j=0; j<ssize; j++)
        {
            PyObject* item_object = PyList_GetItem(list_item, j);
            int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
            int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
            int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
            int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

            cv::Rect r(x,y,w,h);
            word.push_back(r);
        }
        ws.push_back(word);
    }

    std::vector<cv::Rect> new_rects = bounding_rects_for_words(ws);

    PyObject* list = PyList_New(new_rects.size());
    for (int i = 0; i < new_rects.size(); i++)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(new_rects[i].x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(new_rects[i].y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(new_rects[i].width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(new_rects[i].height));
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
        PyList_SetItem(list, i, (PyObject*)res);
    }

    return list;
}

static PyObject* get_grouped_glyphs(PyObject* self, PyObject *args)
{

    PyObject *glyphs;
    PyArg_ParseTuple(args, "O!", &PyList_Type, &glyphs);

    std::vector<cv::Rect> rects;

    int size = PyList_Size(glyphs);
    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(glyphs, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }

    std::vector<cv::Rect> new_rects = group_glyphs(rects);
    PyObject* list = PyList_New(new_rects.size());
    for (int i = 0; i < new_rects.size(); i++)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(new_rects[i].x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(new_rects[i].y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(new_rects[i].width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(new_rects[i].height));
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
        PyList_SetItem(list, i, (PyObject*)res);
    }

    return list;

}

static PyObject* get_baseline(PyObject* self, PyObject *args)
{

    PyObject *glyphs;
    PyArg_ParseTuple(args, "O!", &PyList_Type, &glyphs);

    std::vector<cv::Rect> rects;

    int size = PyList_Size(glyphs);
    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(glyphs, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }

    double bl = baseline(rects);
    return PyFloat_FromDouble(bl);

}

static PyObject* get_word_limits(PyObject* self, PyObject *args)
{
    PyObject *glyphs;
    PyObject *interword_gaps;
    PyArg_ParseTuple(args, "O!O!", &PyDict_Type, &interword_gaps, &PyList_Type, &glyphs);

    std::map<int,int> m;
    std::vector<cv::Rect> rects;

    int size = PyList_Size(glyphs);
    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(glyphs, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }

    PyObject* dict_keys = PyDict_Keys(interword_gaps);
    size = PyList_Size(dict_keys);
    for (int i=0; i<size; i++)
    {

        PyObject* item = PyList_GetItem(dict_keys, i);
        int k = PyInt_AsLong(item);
        int v = PyInt_AsLong(PyDict_GetItem(interword_gaps, item));
        m[k] = v;
    }

    std::vector<std::vector<cv::Rect>> limits = word_limits(m, rects);

    PyObject* list = PyList_New(limits.size());
    for (int i = 0; i < limits.size(); i++)
    {
        std::vector<cv::Rect> word = limits[i];
        PyObject* sublist = PyList_New(word.size());
        for (int j=0; j<word.size(); j++)
        {

            PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
            PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(word[j].x));
            PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(word[j].y));
            PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(word[j].width));
            PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(word[j].height));
            PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
            PyList_SetItem(sublist, j, (PyObject*)res);
        }
        PyList_SetItem(list, i, sublist);

    }

    return list;
}

static PyObject* get_bounding_rect(PyObject* self, PyObject *args)
{

    PyObject *glyphs;
    PyObject *all_inds;
    PyArg_ParseTuple(args, "O!O!", &PyList_Type, &all_inds, &PyList_Type, &glyphs);

    std::vector<cv::Rect> rects;

    int size = PyList_Size(glyphs);
    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(glyphs, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }

    size = PyList_Size(all_inds);
    std::set<int> inds;
    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(all_inds, i);
        int ind = PyInt_AsLong(item_object);
        inds.insert(ind);
    }

    cv::Rect r = bounding_rect(inds, rects);

    PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);

    PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(r.x));
    PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(r.y));
    PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(r.width));
    PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(r.height));
    PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));

    return (PyObject*)res;

}

static PyObject* get_interword_gaps(PyObject* self, PyObject *args)
{
    PyObject *list;

    PyArg_ParseTuple(args, "O!", &PyList_Type, &list);

    int size = PyList_Size(list);
    std::vector<cv::Rect> rects;

    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(list, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
    }

    std::map<int,int> interword_gaps = group_words(rects);

    PyObject *ret_val = PyDict_New();

    for (auto it=interword_gaps.begin(); it != interword_gaps.end(); it++)
    {
        int k = it->first;
        int v = it->second;

        PyDict_SetItem(ret_val, PyLong_FromLong(k), PyLong_FromLong(v));
    }

    return ret_val;
}
static PyObject* get_neighbors(PyObject* self, PyObject *args)
{

    PyObject *lst;
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OOO&", &lst, &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }
    int size = PyList_Size(lst);


    std::vector<cv::Rect> rects;

    double sum_of_heights = 0.0;

    for (int i = 0; i<size; i++)
    {
        PyObject* item_object = PyList_GetItem(lst, i);
        int x = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 0));
        int y = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 1));
        int w = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 2));
        int h = PyInt_AsLong(PyStructSequence_GET_ITEM(item_object, 3));

        cv::Rect r(x,y,w,h);
        rects.push_back(r);
        sum_of_heights += h;
    }

    double average_height = sum_of_heights / size;

    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    std::vector<std::vector<int>> nn;
    for (int r = 0; r < dims[0]; r++)
    {
        std::vector<int> v;
        int* row = (int*)PyArray_GETPTR1(contig, r);
        for (int c = 0; c < dims[1]; c++)
        {
            //int val = mat.at<unsigned int>(r,c);
            int val = row[c];
            v.push_back(val);
        }
        nn.push_back(v);
    }


    std::map<int,std::tuple<cv::Rect,int,double>> nmap = find_neighbors(rects, nn, average_height);

    PyObject *ret_val = PyDict_New();

    for (auto it=nmap.begin(); it != nmap.end(); it++)
    {
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
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));

        PyTuple_SetItem(t, 0, (PyObject*)res);

        PyTuple_SetItem(t, 1, PyLong_FromLong(s));
        PyTuple_SetItem(t, 2, PyFloat_FromDouble(d));

        if (r.x == -1)
        {
            PyDict_SetItem(ret_val, PyLong_FromLong(k), Py_None);
        }
        else
        {
            PyDict_SetItem(ret_val, PyLong_FromLong(k), t);
        }
    }

    return ret_val;

}

static PyObject* get_joined_intervals(PyObject* self, PyObject *args)
{
    PyObject *list;

    PyArg_ParseTuple(args, "O!", &PyList_Type, &list);

    int size = PyList_Size(list);

    std::vector<std::tuple<int,int,int>> rngs;

    for (int i = 0; i<size; i++)
    {
        PyObject* obj = PyList_GetItem(list, i);
        long a = PyInt_AsLong(PyTuple_GET_ITEM(obj, 0));
        long b = PyInt_AsLong(PyTuple_GET_ITEM(obj, 1));
        long c = PyInt_AsLong(PyTuple_GET_ITEM(obj, 2));
        rngs.push_back(std::make_tuple(a,b,c));
    }

    std::vector<interval<int>> intervals = join_intervals(rngs);

    PyObject* ret_val = PyList_New(intervals.size());

    for (int i=0; i<intervals.size(); i++)
    {
        PyObject* t = PyTuple_New(2);
        PyTuple_SetItem(t, 0, PyLong_FromLong(intervals[i].low()));
        PyTuple_SetItem(t, 1, PyLong_FromLong(intervals[i].high()));
        PyList_SetItem(ret_val, i, t);
    }



    return ret_val;

}

static std::map<int,int> clusters(PyObject* input, PyArray_Descr* dtype)
{

    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    std::vector<cv::Rect> rects;
    std::vector<cv::Rect> joined_rects;
    std::set<std::array<int,4>> enclosing_rects;
    cv::Mat mat;
    if (nd >= 2)
    {
        cv::Mat m = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
        mat= m.clone();
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
            if (w > 25 * h || h > 25 * w)
            {
                continue;
            }
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
            if (area < avg_area / 10)
            {
                continue;
            }
            else
            {
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
    double s = 0;
    for (auto it = enclosing_rects.begin(); it != enclosing_rects.end(); ++it)
    {
        std::array<int, 4> a = *it;
        int x = -std::get<0>(a);
        int y = -std::get<1>(a);
        int width = std::get<2>(a) - x;
        int height = std::get<3>(a) - y;
        new_rects.push_back(cv::Rect(x,y,width,height));
        s += height;
    }

    double most_frequent_height = s / new_rects.size();
    int n = new_rects.size();
    double* distmat = new double[(n*(n-1))/2];
    int i, k, j;
    for (i=k=0; i<n; i++)
    {
        for (j=i+1; j<n; j++)
        {
            // compute distance between observables i and j
            double x1 = (new_rects[i].x + new_rects[i].width)/2;
            double y1 = (new_rects[i].y + new_rects[i].height)/2;
            double x2 = (new_rects[j].x + new_rects[j].width)/2;
            double y2 = (new_rects[j].y + new_rects[j].height)/2;
            distmat[k] = sqrt( (x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2) );
            k++;
        }
    }
    int* merge = new int[2*(n-1)];
    double* height = new double[n-1];
    hclust_fast(n, distmat, HCLUST_METHOD_SINGLE, merge, height);


    int* labels = new int[n];

    int last_cluster_count = std::numeric_limits<int>::max();
    int count = 0;
    double x = 0.5;
    std::vector<int> cluster_sizes;
    while (x<=4)
    {
        double coef = x;
        x+=0.1;
        cutree_cdist(n, merge, height, most_frequent_height * coef, labels);

        std::map<int,std::vector<int>> res;
        std::set<int> keys;

        for (int t=0; t<n; t++)
        {
            int key = labels[t];
            keys.insert(key);
            if (res.find(key) != res.end())
            {
                std::vector<int> v = res[key];
                v.push_back(t);
                res[key] = v;
            }
            else
            {
                std::vector<int> v;
                v.push_back(t);
                res[key] = v;
            }
        }

        std::vector<std::vector<int>> small_components;
        int n = cluster_sizes.size();
        cluster_sizes.push_back(keys.size());

        if (n > 1 && (cluster_sizes[n-1] == cluster_sizes[n-2]))  
        {
                break;
        }


        last_cluster_count = keys.size();

    }
    std::map<int,int> ret_val;
    for (int i=0; i<n; i++)
    {
        ret_val[i] = labels[i];
    }

    delete[] distmat;
    delete[] merge;
    delete[] height;
    delete[] labels;
    return ret_val;

}
static std::tuple<std::vector<cv::Rect>, std::vector<int>> join_rects(PyObject* input, PyArray_Descr* dtype, bool do_captions)
{

    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    cv::Mat mat;
    std::vector<cv::Rect> new_rects;
    if (nd >= 2)
    {
        cv::Mat m = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
        mat= m.clone();
        new_rects = find_enclosing_rects(mat);
        Py_DECREF(contig);
    }


    Py_INCREF(input);
    Py_INCREF(dtype);

    if (!do_captions || new_rects.size() < 10)
    {
        return std::make_tuple(new_rects, std::vector<int>());
    }

    auto belongs = detect_captions(mat, new_rects);

    std::vector<cv::Rect> rects_with_joined_captions;
    std::vector<int> pic_indexes = join_with_captions(belongs, new_rects, rects_with_joined_captions);

    return std::make_tuple(rects_with_joined_captions, pic_indexes);

}

static PyObject* get_reflowed_image(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    float factor = 1.0f;
    float zoom_factor = 1.0f;
    if (!PyArg_ParseTuple(args, "ffOO&", &factor, &zoom_factor, &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }

    std::tuple<std::vector<cv::Rect>,std::vector<int>> new_rects_with_pictures = join_rects(input, dtype, true);
    std::vector<cv::Rect> new_rects = std::get<0>(new_rects_with_pictures);
    std::vector<int> pic_indexes = std::get<1>(new_rects_with_pictures);

    std::vector<cv::Rect> filtered_rects;
    std::vector<cv::Rect> pictures;
    for (int i=0;i<new_rects.size(); i++) {
        if (std::find(pic_indexes.begin(), pic_indexes.end(), i) == pic_indexes.end()) {
            filtered_rects.push_back(new_rects[i]);
        } else {
            pictures.push_back(new_rects[i]);
        }
    }

    int nd = PyArray_NDIM(input);
    npy_intp* dims = PyArray_DIMS(input);

    PyArrayObject* contig = (PyArrayObject*)PyArray_FromAny(input,
                            dtype,
                            1, 3, NPY_ARRAY_CARRAY, NULL);

    cv::Mat mat = cv::Mat(cv::Size(dims[1], dims[0]), CV_8UC1, PyArray_DATA(contig));
    cv::Mat new_image = filtered_rects.size() < 10 ? mat : find_reflowed_image(filtered_rects, pictures, factor, zoom_factor, mat);

    cv::Size s = new_image.size();
    const unsigned int nElem = s.height * s.width;
    uchar* m = new uchar[nElem];
    std::memcpy(m, new_image.data, nElem * sizeof(uchar));

    npy_intp mdim[] = { s.height, s.width };
    PyObject* img = PyArray_SimpleNewFromData(2, mdim, NPY_UINT8, (void*) m);

    PyObject* ret_val = Py_BuildValue("O", img, s.height, s.width);
    PyArray_ENABLEFLAGS((PyArrayObject*)ret_val, NPY_ARRAY_OWNDATA);
    
    return ret_val;

}


static PyObject* get_ordered_glyphs(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }

    auto new_rects_with_pictures = join_rects(input, dtype, true);
    std::vector<cv::Rect> new_rects = std::get<0>(new_rects_with_pictures);
    std::vector<int> pic_indexes = std::get<1>(new_rects_with_pictures);

    std::vector<cv::Rect> filtered_rects;
    for (int i=0;i<new_rects.size(); i++) {
        if (std::find(pic_indexes.begin(), pic_indexes.end(), i) == pic_indexes.end()) {
            filtered_rects.push_back(new_rects[i]);
        }
    }

    std::vector<words_struct> lines_of_words = find_ordered_glyphs(filtered_rects);


    PyObject *ret_val = PyList_New(lines_of_words.size());

    for (int i=0; i<lines_of_words.size(); i++)
    {
        words_struct words_s = lines_of_words[i];
        double lower = words_s.lower;
        std::vector<std::vector<cv::Rect>> words = words_s.words;
        PyObject *w = PyList_New(words.size());
        for (int j=0; j<words.size(); j++)
        {
            std::vector<cv::Rect> word = words[j];
            PyObject *w1 = PyList_New(word.size());
            for (int n=0; n<word.size(); n++)
            {
                cv::Rect r = word[n];
                PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
                PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(r.x));
                PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(r.y));
                PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(r.width));
                PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(r.height));
                PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong((int)lower - (r.y + r.height)));

                PyList_SetItem(w1, n, (PyObject*)res);

            }
            PyList_SetItem(w, j, w1);
        }
        PyList_SetItem(ret_val, i, w);
    }

    return ret_val;

}

static PyObject* get_clusters(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }

    std::map<int,int> map = clusters(input, dtype);

    std::tuple<std::vector<cv::Rect>,std::vector<int>> new_rects_with_pictures = join_rects(input, dtype, false);
    std::vector<cv::Rect> new_rects = std::get<0>(new_rects_with_pictures);
    //std::vector<int> pic_indexes = std::get<1>(new_rects_with_pictures);

    int N = new_rects.size();
    PyObject* list = PyList_New(N);
    for (int i = 0; i < N; ++i)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(new_rects[i].x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(new_rects[i].y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(new_rects[i].width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(new_rects[i].height));
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
        PyList_SetItem(list, i, (PyObject*)res);
    }

    PyObject *dict = PyDict_New();

    for (auto it=map.begin(); it != map.end(); it++)
    {
        int k = it->first;
        int v = it->second;

        PyDict_SetItem(dict, PyLong_FromLong(k), PyLong_FromLong(v));
    }

    PyObject *tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, list);
    PyTuple_SetItem(tuple, 1, dict);
    return tuple;
}

static PyObject* get_joined_rects(PyObject* self, PyObject *args)
{
    PyObject *input;
    PyArray_Descr *dtype = NULL;
    if (!PyArg_ParseTuple(args, "OO&", &input, PyArray_DescrConverter, &dtype))
    {
        return NULL;
    }

    auto new_rects_with_pictures = join_rects(input, dtype, true);
    std::vector<cv::Rect> new_rects = std::get<0>(new_rects_with_pictures); 
    std::vector<int> pic_indexes = std::get<1>(new_rects_with_pictures); 

    int N = new_rects.size();
    PyObject* list = PyList_New(N);
    for (int i = 0; i < N; ++i)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(new_rects[i].x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(new_rects[i].y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(new_rects[i].width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(new_rects[i].height));
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
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
        cv::threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        PageSegmenter ps(mat);
        glyphs = ps.get_glyphs();
        Py_DECREF(contig);
    }


    Py_INCREF(input);
    Py_INCREF(dtype);


    int N = glyphs.size();
    PyObject* list = PyList_New(N);
    for (int i = 0; i < N; ++i)
    {
        PyStructSequence* res = (PyStructSequence*) PyStructSequence_New(&GlyphResultType);
        PyStructSequence_SET_ITEM(res, 0, PyLong_FromLong(glyphs.at(i).x));
        PyStructSequence_SET_ITEM(res, 1, PyLong_FromLong(glyphs.at(i).y));
        PyStructSequence_SET_ITEM(res, 2, PyLong_FromLong(glyphs.at(i).width));
        PyStructSequence_SET_ITEM(res, 3, PyLong_FromLong(glyphs.at(i).height));
        PyStructSequence_SET_ITEM(res, 4, PyLong_FromLong(0));
        PyList_SetItem(list, i, (PyObject*)res);
    }

    return list;
}

// Exported methods are collected in a table
static PyMethodDef method_table[] =
{
    {"get_glyphs", (PyCFunction) get_glyphs, METH_VARARGS, "get_glyphs finds all glyphs (letters) in an image"},
    {"get_joined_rects", (PyCFunction) get_joined_rects, METH_VARARGS, "get_joined_rects finds all interecting rectangles and joines them, returns joined rectangles"},
    {"get_joined_intervals", (PyCFunction) get_joined_intervals, METH_VARARGS, "get_joined_intervals finds all interecting intervals and joines them, returns joined intervals"},
    {"get_neighbors", (PyCFunction) get_neighbors, METH_VARARGS, "get_neighbors finds all neigboring rectangles"},
    {"get_interword_gaps", (PyCFunction) get_interword_gaps, METH_VARARGS, "get_interword_gaps finds interword gaps"},
    {"get_bounding_rect", (PyCFunction) get_bounding_rect, METH_VARARGS, "get_bounding_rect for a list of rects"},
    {"get_word_limits", (PyCFunction) get_word_limits, METH_VARARGS, "get_word_limits get list of words"},
    {"get_baseline", (PyCFunction) get_baseline, METH_VARARGS, "gets baseline for a list of glyphs"},
    {"get_grouped_glyphs", (PyCFunction) get_grouped_glyphs, METH_VARARGS, "gets grouped glyphs"},
    {"get_bounding_rects_for_words", (PyCFunction) get_bounding_rects_for_words, METH_VARARGS, "gets bounding rects for words"},
    {"get_ordered_glyphs", (PyCFunction) get_ordered_glyphs, METH_VARARGS, "gets ordered glyphs"},
    {"get_reflowed_image", (PyCFunction) get_reflowed_image, METH_VARARGS, "gets reflowed image"},
    {"get_clusters", (PyCFunction) get_clusters, METH_VARARGS, "gets clusters"},
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
    //Py_Initialize();
    import_array();

    PyObject *mod = PyModule_Create(&segm_module_def);

    if (GlyphResultType.tp_name == 0)
        PyStructSequence_InitType(&GlyphResultType, &glyph_result_desc);
    Py_INCREF((PyObject *) &GlyphResultType);
    PyModule_AddObject(mod, "glyph_result", (PyObject *) &GlyphResultType);
    return mod;
}
