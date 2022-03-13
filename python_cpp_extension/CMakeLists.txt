set(PYTHON_CPP_EXTENSION_SRCS
    "segm.cpp"
)

add_library(python_cpp_extension SHARED ${PYTHON_CPP_EXTENSION_SRCS})

set_target_properties(
    python_cpp_extension
    PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${SEGM_PYTHON_CPP_MODULE_NAME}
        LINKER_LANGUAGE C
)

find_package(PythonInterp 3.9 REQUIRED)

find_package(PythonLibs 3.9 REQUIRED)

exec_program(${PYTHON_EXECUTABLE}
             ARGS "-c \"import numpy; print(numpy.get_include())\""
             OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
             RETURN_VALUE NUMPY_NOT_FOUND
            )
if(NUMPY_NOT_FOUND)
    message(FATAL_ERROR "NumPy headers not found")
endif()


include_directories( ${PYTHON_INCLUDE_DIR} )

target_include_directories(python_cpp_extension PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(python_cpp_extension ${PYTHON_LIBRARIES} segm)



