#ifndef FLOW_READER_PDF_LIB_H
#define FLOW_READER_PDF_LIB_H

#include <jni.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativeGrayscaleBytes
        (JNIEnv *, jclass, jlong, jint);

JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativeBytes
        (JNIEnv *, jclass, jlong, jint);

    JNIEXPORT jstring JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBook_getNativeTitle
        (JNIEnv *, jclass, jlong);

    JNIEXPORT jstring JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBook_getNativeAuthor
        (JNIEnv *, jclass, jlong);

    JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBook_close
        (JNIEnv *, jclass, jlong);

    JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativePageGlyphs
        (JNIEnv *, jclass, jlong, jint, jobject);

    JNIEXPORT jobject JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativeReflowedBytes
        (JNIEnv *, jclass, jlong, jint, jfloat, jint, jobject, jobject, jboolean, jfloat);

    JNIEXPORT jlong JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBook_openBook
        (JNIEnv *, jobject, jstring);

    JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBook_getNumberOfPages
        (JNIEnv *, jobject, jlong);

    /*
     * Class:     com_veve_flowreader_model_impl_djvu_DjvuBookPage
     * Method:    getNativeWidth
     * Signature: (JI)I
     */
    JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativeWidth
        (JNIEnv *, jclass, jlong, jint);

    /*
     * Class:     com_veve_flowreader_model_impl_djvu_DjvuBookPage
     * Method:    getNativeHeight
     * Signature: (JI)I
     */
    JNIEXPORT jint JNICALL Java_com_veve_flowreader_model_impl_pdf_PdfBookPage_getNativeHeight
        (JNIEnv *, jclass, jlong, jint);

    image_format get_pdf_pixels(JNIEnv*, jlong, jint, char**);


#ifdef __cplusplus
}


#endif
#endif //FLOW_READER_PDF_LIB_H
