if (NOT CONE_COMPILER_BINARY)
    # Find the compiler
    find_program(
            CMAKE_CONE_COMPILER
            NAMES "conec"
            HINTS "${CMAKE_SOURCE_DIR}"
            DOC   "Cone source code compiler compiler")
else()
    set(CMAKE_CONE_COMPILER ${CONE_COMPILER_BINARY})
endif()

mark_as_advanced(CMAKE_CONE_COMPILER)

set(CMAKE_CONE_SOURCE_FILE_EXTENSIONS cone)

# Remember this as a potential error
set(CMAKE_CONE_OUTPUT_EXTENSION .o)
set(CMAKE_CONE_COMPILER_ENV_VAR "")


# Configure variables set in this file for fast reload later on
configure_file( ${CMAKE_CURRENT_LIST_DIR}/CMakeConeCompiler.cmake.in
        ${CMAKE_PLATFORM_INFO_DIR}/CMakeConeCompiler.cmake )