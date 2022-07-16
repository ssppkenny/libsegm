cmake_minimum_required(VERSION 3.10)
project(segm)
## common.cpp Enclosure.cpp ImageNode.cpp LineSpacing.cpp PageSegmenter.cpp Reflow.cpp Xycut.cpp segm.cpp )
set(SRCS segm/common.cpp segm/Enclosure.cpp segm/fastcluster_dm.cpp segm/fastcluster.cpp segm/IntervalJoin.cpp segm/ImageNode.cpp segm/LineSpacing.cpp segm/PageSegmenter.cpp segm/RectJoin.cpp segm/Reflow.cpp segm/Xycut.cpp segm/segmentation.cpp segm/segm.cpp) 

set(ignoreMe "${SKBUILD}")

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)

set(MINIMUM_LEPTONICA_VERSION 1.79)
set(MINIMUM_FLANN_VERSION 1.9.1)

find_package(Boost 1.74.0 COMPONENTS system filesystem graph) 


find_package( OpenCV REQUIRED core imgproc imgcodecs core)


find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT Flann_DIR)
    pkg_check_modules(Flann REQUIRED flann>=${MINIMUM_FLANN_VERSION})
else()
    find_package(Flann ${MINIMUM_FLANN_VERSION} REQUIRED flann)
endif()

if (NOT Flann_FOUND)
    message(FATAL_ERROR "Cannot find required library Flann. Quitting!")
endif(NOT Flann_FOUND)

find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT LZ4_DIR)
    pkg_check_modules(Lz4 REQUIRED liblz4)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT Leptonica_DIR)
    pkg_check_modules(Leptonica REQUIRED lept>=${MINIMUM_LEPTONICA_VERSION})
else()
    find_package(Leptonica ${MINIMUM_LEPTONICA_VERSION} REQUIRED CONFIG)
endif()
if (NOT Leptonica_FOUND)
    message(FATAL_ERROR "Cannot find required library Leptonica. Quitting!")
endif(NOT Leptonica_FOUND)

set(SEGM_PYTHON_CPP_MODULE_NAME "_segm" CACHE STRING "Name of the C extension module file")

find_package(PythonInterp 3.9 REQUIRED)

find_package(PythonLibs 3.9 REQUIRED)

find_package(PythonExtensions REQUIRED)

add_library(_segm MODULE ${SRCS})
python_extension_module(_segm)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(Python3 3.9 REQUIRED COMPONENTS NumPy)
    message(STATUS ${Python3_NumPy_INCLUDE_DIRS})
endif()

exec_program(${PYTHON_EXECUTABLE}
             ARGS "-c \"import numpy; print(numpy.get_include())\""
             OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
             RETURN_VALUE NUMPY_NOT_FOUND
            )
if(NUMPY_NOT_FOUND)
    message(FATAL_ERROR "NumPy headers not found")
endif()


##include_directories( ${PYTHON_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS} ${Leptonica_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${Flann_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

target_link_directories(_segm PUBLIC ${Python_LIBRARY_DIRS} ${Flann_LIBRARY_DIRS} ${Leptonica_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

target_include_directories(_segm PUBLIC ${PYTHON_INCLUDE_DIRS} ${Python3_NumPy_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} ${Leptonica_INCLUDE_DIRS})
target_link_libraries(_segm ${Boost_LIBRARIES} ${Flann_LIBRARIES} ${OpenCV_LIBRARIES} ${Leptonica_LIBRARIES} ${Lz4_LIBRARIES} ${Gettext_LINK_LIBRARIES} )
install(TARGETS _segm LIBRARY DESTINATION segm)

