cmake_minimum_required(VERSION 3.21)
project(segm)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(MINIMUM_LEPTONICA_VERSION 1.82)
set(MINIMUM_FLANN_VERSION 1.9.1)

set(SEGM_PYTHON_CPP_MODULE_NAME "_segm" CACHE STRING "Name of the C extension module file")


find_package(Boost 1.78.0 COMPONENTS system filesystem graph) 

include_directories(${Boost_INCLUDE_DIRS}) 

find_package( OpenCV REQUIRED core imgproc imgcodecs core)

include_directories( ${OpenCV_INCLUDE_DIRS} )

find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT Flann_DIR)
    pkg_check_modules(Flann REQUIRED flann>=${MINIMUM_FLANN_VERSION})
    link_directories(${Flann_LIBRARY_DIRS})
else()
    find_package(Flann ${MINIMUM_FLANN_VERSION} REQUIRED flann)
endif()

if (NOT Flann_FOUND)
    message(FATAL_ERROR "Cannot find required library Flann. Quitting!")
endif(NOT Flann_FOUND)

find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT LZ4_DIR)
    pkg_check_modules(Lz4 REQUIRED liblz4)
    link_directories(${Lz4_LIBRARY_DIRS})
endif()


find_package(PkgConfig)
if(PKG_CONFIG_EXECUTABLE AND NOT Leptonica_DIR)
    pkg_check_modules(Leptonica REQUIRED lept>=${MINIMUM_LEPTONICA_VERSION})
    link_directories(${Leptonica_LIBRARY_DIRS})
else()
    find_package(Leptonica ${MINIMUM_LEPTONICA_VERSION} REQUIRED CONFIG)
endif()
if (NOT Leptonica_FOUND)
    message(FATAL_ERROR "Cannot find required library Leptonica. Quitting!")
endif(NOT Leptonica_FOUND)


include_directories( ${Leptonica_INCLUDE_DIRS} )

add_subdirectory(src)

add_subdirectory(python_cpp_extension)



