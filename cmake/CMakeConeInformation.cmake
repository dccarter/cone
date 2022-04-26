# This file sets the basic flags for the GAMBIT compiler

# Generate the C files
set(CMAKE_CONE_COMPILE_OBJECT
        "<CMAKE_CONE_COMPILER> -o CMakeFiles/<TARGET>.dir/<TARGET> <SOURCE>")

# Build a executable
set(CMAKE_CONE_LINK_EXECUTABLE
        "<CMAKE_C_COMPILER> -o <TARGET> <OBJECTS>")

set(CMAKE_CONE_INFORMATION_LOADED 1)

# Configure variables set in this file for fast reload later on
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeConeCompiler.cmake.in
        ${CMAKE_PLATFORM_INFO_DIR}/CMakeConeCompiler.cmake)
