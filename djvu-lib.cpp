
#include "djvu-lib.h"
#include "common.h"

#define PIXELS 4


struct Document {
    ddjvu_context_t *ctx;
    ddjvu_document_t *doc;
};

jstring get_annotation(JNIEnv *env, jlong bookId, const char* key) {
    Document *document = (Document *) bookId;
    miniexp_t annotations = ddjvu_document_get_anno(document->doc, 1);
    miniexp_t *keys = ddjvu_anno_get_metadata_keys(annotations);

    if (!keys) return env->NewStringUTF("");

    int i;
    for (i = 0; keys[i] != miniexp_nil; i++) {
        const char *value = ddjvu_anno_get_metadata(annotations, keys[i]);
        if (value) {
            const char* k = miniexp_to_name(keys[i]);
            if (std::strcmp(k, key)==0) {
                __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "value =  %s\n", value);
                return env->NewStringUTF(value);
            }
        }
    }

    if (keys) free(keys);
    return NULL;
}


JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_getNumberOfPages
(JNIEnv *env, jobject obj, jlong bookId) {

    Document *document = (Document *) bookId;
    ddjvu_document_t *doc = document->doc;
    ddjvu_context_t *ctx = document->ctx;

    ddjvu_message_wait(ctx);
    while (ddjvu_message_peek(ctx)) {
        if (ddjvu_document_decoding_done(doc)) {
            break;
        }
        ddjvu_message_pop(ctx);
    }
   return ddjvu_document_get_pagenum(doc);
}

JNIEXPORT jstring JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_getNativeTitle
(JNIEnv *env, jclass cls, jlong bookId) {
    return get_annotation(env, bookId, "Title");
}

JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_close
(JNIEnv *env, jclass cls, jlong bookId) {
     Document *document = (Document *) bookId;
     ddjvu_document_t *doc = document->doc;
     ddjvu_context_t *ctx = document->ctx;
     ddjvu_document_release(doc);
     ddjvu_context_release(ctx);
     return 0;
}


JNIEXPORT jstring JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_getNativeAuthor
(JNIEnv *env, jclass cls, jlong bookId) {
    return get_annotation(env, bookId, "Author");
}


JNIEXPORT jlong JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_openBook
(JNIEnv *env, jobject obj, jstring path) {

    Document *d = (Document *) malloc(sizeof(struct Document));
    const char *nativePath = env->GetStringUTFChars(path, 0);
    ddjvu_context_t *ctx = ddjvu_context_create("djvu");
    ddjvu_document_t *doc = ddjvu_document_create_by_filename(ctx, nativePath, TRUE);
    d->ctx = ctx;
    d->doc = doc;
    return (jlong) d;

}

JNIEXPORT jstring JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBook_openStringBook
(JNIEnv *env, jobject obj, jstring str) {

    return env->NewStringUTF("test");
}

JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativeWidth
(JNIEnv *env, jclass cls, jlong bookId, jint pageNumber) {

    Document *document = (Document *) bookId;
    ddjvu_document_t *doc = document->doc;

    int pageno = (int) pageNumber;
    ddjvu_status_t r;
    ddjvu_pageinfo_t info;
    while ((r = ddjvu_document_get_pageinfo(doc, pageno, &info)) < DDJVU_JOB_OK) {

    }
    return (jint) info.width;

}

JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativeHeight
(JNIEnv *env, jclass cls, jlong bookId, jint pageNumber) {

    Document *document = (Document *) bookId;
    ddjvu_document_t *doc = document->doc;

    int pageno = (int) pageNumber;

    ddjvu_status_t r;
    ddjvu_pageinfo_t info;
    while ((r = ddjvu_document_get_pageinfo(doc, pageno, &info)) < DDJVU_JOB_OK) {

    }
    return (jint) info.height;

}

void ThrowError(JNIEnv *env, const char *msg) {
    jclass exceptionClass = env->FindClass("java/lang/RuntimeException");
    if (!exceptionClass)
        return;
    if (!msg)
        env->ThrowNew(exceptionClass, "Djvu decoding error!");
    else env->ThrowNew(exceptionClass, msg);
}

void ThrowDjvuError(JNIEnv *env, const ddjvu_message_t *msg) {
    if (!msg || !msg->m_error.message)
        ThrowError(env, "Djvu decoding error!");
    else ThrowError(env, msg->m_error.message);
}

void handleMessages(JNIEnv *env, ddjvu_context_t *ctx) {
    const ddjvu_message_t *msg;
    while ((msg = ddjvu_message_peek(ctx))) {
        switch (msg->m_any.tag) {
            case DDJVU_ERROR:
                ThrowDjvuError(env, msg);
                break;
            case DDJVU_INFO:
                break;
            case DDJVU_DOCINFO:
                break;
            default:
                break;
        }
        ddjvu_message_pop(ctx);
    }
}


void waitAndHandleMessages(JNIEnv *env, ddjvu_context_t *contextHandle) {
    ddjvu_context_t *ctx = contextHandle;
    // Wait for first message
    ddjvu_message_wait(ctx);
    // Process available messages
    handleMessages(env, ctx);
}

image_format get_djvu_pixels(JNIEnv *env, jlong bookId, jint page_number, jboolean colored, char** pixels) {

    Document *document = (Document *) bookId;
    ddjvu_context_t *ctx = document->ctx;
    ddjvu_document_t *doc = document->doc;

    int pageno = (int) page_number;

    ddjvu_page_t *page = ddjvu_page_create_by_pageno(doc, pageno);

    ddjvu_status_t r;
    ddjvu_pageinfo_t info;
    while ((r = ddjvu_document_get_pageinfo(doc, pageno, &info)) < DDJVU_JOB_OK) {
    }

    int w = info.width;
    int h = info.height;

    ddjvu_rect_t rrect;
    ddjvu_rect_t prect;

    prect.x = 0;
    prect.y = 0;
    prect.w = w;
    prect.h = h;
    rrect = prect;

    unsigned int masks[] = {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000};
    ddjvu_format_t *pixelFormat = colored ? ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks) : ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, NULL);

    ddjvu_format_set_row_order(pixelFormat, 1);
    ddjvu_format_set_y_direction(pixelFormat, 1);

    int size = colored ? w * h * PIXELS : w * h;
    *pixels = (char *) malloc(size);

    while (!ddjvu_page_decoding_done(page)) {
        waitAndHandleMessages(env, ctx);
    }

    int strade = colored ? w * PIXELS : w;
    ddjvu_page_render(page, DDJVU_RENDER_COLOR,
                                     &prect,
                                     &rrect,
                                     pixelFormat,
                                     strade,
                                      *pixels);

    ddjvu_format_release(pixelFormat);
    return image_format(w,h,size, info.dpi);

}

JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativeReflowedBytes
        (JNIEnv *env, jclass cls, jlong bookId, jint pageNumber, jfloat scale, jint pageWidth, jobject pageSize, jobject list, jboolean preprocessing, jfloat margin) {

    std::vector<glyph> glyphs = convert_java_glyphs(env, list);

    char* pixels;
    image_format format = get_djvu_pixels(env, bookId, pageNumber, false, &pixels);
    int w = format.w;
    int h= format.h;

    Mat mat(h, w, CV_8UC1, &((char *) pixels)[0]);
    cv::Mat new_image;

    bool do_preprocessing = (bool)preprocessing;
    if (do_preprocessing) {
        std::vector<uchar> buff;//buffer for coding
        cv::imencode(".png", mat, buff);
        PIX* pix = pixReadMemPng((l_uint8*)&buff[0], buff.size()) ;
        PIX* result;
        dewarpSinglePage(pix, 127, 1, 1, 1, &result, NULL, 1);
        PIX* r = pixDeskew(result, 0);
        pixDestroy(&result);
        pixDestroy(&pix);

        cv::Mat m = pix8ToMat(r);
        threshold(m, m, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        cv::Mat rotated_with_pictures;
        std::vector<glyph> pic_glyphs = preprocess(m, rotated_with_pictures);
        reflow(m, new_image, scale, pageWidth, env, glyphs, list, pic_glyphs, rotated_with_pictures, true, margin);
        pixDestroy(&r);
    } else {
        threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
        reflow(mat, new_image, scale, pageWidth, env, glyphs, list, std::vector<glyph>(), mat, false, margin);
    }

    jclass clz = env->GetObjectClass(pageSize);

    jmethodID setPageWidthMid = env->GetMethodID(clz, "setPageWidth", "(I)V");
    jmethodID setPageHeightMid = env->GetMethodID(clz, "setPageHeight", "(I)V");
    env->CallVoidMethod(pageSize,setPageWidthMid, new_image.cols);
    env->CallVoidMethod(pageSize,setPageHeightMid, new_image.rows);

    cv::bitwise_not(new_image, new_image);
    jobject arrayList = splitMat(new_image, env);
    free(pixels);
    return arrayList;

}

JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativePageGlyphs
(JNIEnv *env, jclass cls, jlong bookId, jint pageNumber, jobject list) {

    char* pixels;
    image_format format = get_djvu_pixels(env, bookId, pageNumber, false, &pixels);
    int w = format.w;
    int h= format.h;

    Mat mat(h, w, CV_8UC1, &((char *) pixels)[0]);
    threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::Mat rotated_with_pictures;
    std::vector<glyph> pic_glyphs = preprocess(mat, rotated_with_pictures);
    //remove_skew(mat);


    size_t sizeInBytes = mat.total() * mat.elemSize();
    vector<glyph> new_glyphs = get_glyphs(mat);
    put_glyphs(env, new_glyphs, list);

    jbyteArray array = env->NewByteArray(sizeInBytes);
    env->SetByteArrayRegion(array, 0, sizeInBytes, (jbyte *) mat.data);
    free(pixels);
    return array;

}

JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativeBytes
(JNIEnv *env, jclass cls, jlong bookId, jint pageNumber) {

    char* pixels;
    image_format format = get_djvu_pixels(env, bookId, pageNumber,  true, &pixels);
    int w = format.w;
    int h= format.h;
    int size = format.size;

    jbyteArray array = env->NewByteArray(size);
    env->SetByteArrayRegion(array, 0, size, (jbyte *) pixels);
    free(pixels);

    return array;
}


JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_djvu_DjvuBookPage_getNativeGrayscaleBytes
        (JNIEnv *env, jclass cls, jlong bookId, jint pageNumber) {

    char* pixels;
    image_format format = get_djvu_pixels(env, bookId, pageNumber, false, &pixels);
    int w = format.w;
    int h= format.h;

    Mat mat(h, w, CV_8UC1, &((char *) pixels)[0]);
    threshold(mat, mat, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);


    size_t sizeInBytes = mat.total() * mat.elemSize();

    jbyteArray array = env->NewByteArray(sizeInBytes);
    env->SetByteArrayRegion(array, 0, sizeInBytes, (jbyte *) mat.data);
    free(pixels);
    return array;
}
