################################################################################
# CMake Build Configuration for MATLAB/Simulink S-Function
#
# This CMake script compiles a custom S-Function MEX file that integrates
# optical flow tracking capabilities into Simulink models.
#
# CMake variables (optional - FindMatlab will auto-detect):
#   MATLAB_ROOT    - MATLAB installation root directory
#   MATLAB_BIN_DIR - Path to MATLAB bin directory (alternative to MATLAB_ROOT)
#
# Output files:
#   - matlab_simulink_s_function.mex* (platform-specific MEX file)
#   - include_directories.txt (for Simulink code generation)
#   - link_directories.txt (for Simulink code generation)
#   - link_libraries.txt (for Simulink code generation)
#   - source_files.txt (for Simulink code generation)
################################################################################

cmake_minimum_required(VERSION 3.10)
project(matlab_simulink_s_function)


################################################################################
# Find MATLAB Installation
################################################################################

# Add cmake modules directory to module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find MATLAB (with MEX compiler required)
# FindMatlab will automatically search common installation paths
# You can override by setting: cmake -DMATLAB_ROOT=/path/to/matlab ..
find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER)

if(NOT Matlab_FOUND)
    message(FATAL_ERROR
            "MATLAB not found. Please specify MATLAB location:\n"
            "  cmake -DMATLAB_ROOT=/path/to/matlab ..\n"
            "  OR\n"
            "  cmake -DMATLAB_BIN_DIR=/path/to/matlab/bin ..")
endif()

# Use the found MEX compiler
set(MEX_COMPILER "${MATLAB_MEX_COMPILER}")

# Platform-specific compiler flags
# Note: MEX-specific flags are handled in the build section below
set(CUSTOM_COMPILE_FLAGS "")
if(WIN32)
    # Windows: /MT flag is handled via COMPFLAGS in build section
    # Don't add it here to avoid path parsing issues
elseif(UNIX AND NOT APPLE)
    # Linux-specific flags can be added here if needed
elseif(APPLE)
    # macOS-specific flags can be added here if needed
else()
    message(WARNING "Unsupported platform detected. Build may fail.")
endif()

################################################################################
# External Package Dependencies
################################################################################
set(CUSTOM_PACKAGES
    OpenCV
)

foreach(PACKAGE ${CUSTOM_PACKAGES})
    find_package(${PACKAGE} REQUIRED)
    message(STATUS "Found ${PACKAGE}")
endforeach()

################################################################################
# Source Files and Include Directories
################################################################################
set(SRCS
    ${CMAKE_SOURCE_DIR}/src/s_function.cpp
    ${CMAKE_SOURCE_DIR}/src/optical_flow_velocity.cpp
)
set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")

################################################################################
# Build Compiler and Linker Flags
################################################################################
set(INCLUDE_FLAGS "")
set(LIB_FLAGS "")
set(INCLUDE_FLAGS_RAW "")
set(LIB_FLAGS_RAW "")
set(LINK_DIRS_RAW "")
list(APPEND INCLUDE_FLAGS_RAW "${INCLUDE_DIRS}")

# Collect include directories from all packages
foreach(PACKAGE ${CUSTOM_PACKAGES})
    foreach(DIR ${${PACKAGE}_INCLUDE_DIRS})
        string(APPEND INCLUDE_FLAGS "-I${DIR} ")
        list(APPEND INCLUDE_FLAGS_RAW "${DIR} ")
    endforeach()
endforeach()

# Add custom include directory
string(APPEND INCLUDE_FLAGS "-I${INCLUDE_DIRS} ")

# Collect library directories from all packages
foreach(PACKAGE ${CUSTOM_PACKAGES})
    # Method 1: Check for explicitly defined library directory variables
    if(DEFINED ${PACKAGE}_LIBRARY_DIRS)
        foreach(DIR ${${PACKAGE}_LIBRARY_DIRS})
            list(APPEND LINK_DIRS_RAW "${DIR}")
        endforeach()
    elseif(DEFINED ${PACKAGE}_LIBRARY_DIR)
        list(APPEND LINK_DIRS_RAW "${${PACKAGE}_LIBRARY_DIR}")
    elseif(DEFINED ${PACKAGE}_LIB_DIR)
        list(APPEND LINK_DIRS_RAW "${${PACKAGE}_LIB_DIR}")
    endif()

    # Method 2: Extract directories from absolute library paths
    foreach(LIB ${${PACKAGE}_LIBRARIES})
        if(IS_ABSOLUTE "${LIB}")
            get_filename_component(LIB_DIR "${LIB}" DIRECTORY)
            list(APPEND LINK_DIRS_RAW "${LIB_DIR}")
        endif()
    endforeach()

    # Method 3: Try to find libraries and extract their directories
    # This handles cases where ${PACKAGE}_LIBRARIES contains only library names
    foreach(LIB ${${PACKAGE}_LIBRARIES})
        if(NOT IS_ABSOLUTE "${LIB}" AND NOT LIB MATCHES "\\.(so|a|lib|dll|dylib)$")
            # It's just a library name, try to find it
            set(LIB_SEARCH_NAMES "")

            # Platform-specific library naming
            if(WIN32)
                list(APPEND LIB_SEARCH_NAMES "${LIB}.lib" "lib${LIB}.lib" "${LIB}.dll")
            elseif(APPLE)
                list(APPEND LIB_SEARCH_NAMES "lib${LIB}.dylib" "lib${LIB}.a" "${LIB}.dylib")
            else()
                list(APPEND LIB_SEARCH_NAMES "lib${LIB}.so" "lib${LIB}.a" "${LIB}.so")
            endif()

            # Try to locate the library
            find_library(${LIB}_LOCATION
                NAMES ${LIB_SEARCH_NAMES}
                HINTS ${${PACKAGE}_LIBRARY_DIRS} ${${PACKAGE}_LIBRARY_DIR}
                NO_DEFAULT_PATH
            )

            if(NOT ${LIB}_LOCATION)
                # Try system paths if not found
                find_library(${LIB}_LOCATION NAMES ${LIB_SEARCH_NAMES})
            endif()

            if(${LIB}_LOCATION)
                get_filename_component(LIB_DIR "${${LIB}_LOCATION}" DIRECTORY)
                list(APPEND LINK_DIRS_RAW "${LIB_DIR}")
                unset(${LIB}_LOCATION CACHE)
            endif()
        endif()
    endforeach()
endforeach()

# Remove duplicate directories
if(LINK_DIRS_RAW)
    list(REMOVE_DUPLICATES LINK_DIRS_RAW)
endif()

# Collect libraries from all packages
foreach(PACKAGE ${CUSTOM_PACKAGES})
    foreach(LIB ${${PACKAGE}_LIBRARIES})
        # For MEX compilation flags, use -l syntax (library name only)
        # Extract just the library name without path or prefix/suffix
        get_filename_component(LIB_NAME "${LIB}" NAME_WE)
        string(REGEX REPLACE "^lib" "" LIB_NAME_CLEAN "${LIB_NAME}")
        string(APPEND LIB_FLAGS "-l${LIB_NAME_CLEAN} ")

        # For LIB_FLAGS_RAW, include the full library path with extension
        set(LIB_FULL_PATH "")

        if(IS_ABSOLUTE "${LIB}")
            # LIB is already an absolute path, use as-is
            set(LIB_FULL_PATH "${LIB}")
        elseif(LIB MATCHES "\\.(so|a|lib|dll|dylib)$")
            # LIB has an extension but is relative - use as-is
            set(LIB_FULL_PATH "${LIB}")
        elseif(EXISTS "${LIB}")
            # LIB exists as a file path, use as-is
            set(LIB_FULL_PATH "${LIB}")
        else()
            # LIB is just a library name, try to find the actual file
            set(LIB_SEARCH_NAMES "")

            # Platform-specific library naming
            if(WIN32)
                list(APPEND LIB_SEARCH_NAMES "${LIB}.lib" "lib${LIB}.lib" "${LIB}.dll")
            elseif(APPLE)
                list(APPEND LIB_SEARCH_NAMES "lib${LIB}.dylib" "lib${LIB}.a" "${LIB}.dylib")
            else()
                list(APPEND LIB_SEARCH_NAMES "lib${LIB}.so" "lib${LIB}.a" "${LIB}.so")
            endif()

            # Try to locate the library file
            find_library(${LIB}_FULL_LOCATION
                NAMES ${LIB_SEARCH_NAMES}
                HINTS ${${PACKAGE}_LIBRARY_DIRS} ${${PACKAGE}_LIBRARY_DIR}
                NO_DEFAULT_PATH
            )

            if(NOT ${LIB}_FULL_LOCATION)
                # Try system paths if not found in package directories
                find_library(${LIB}_FULL_LOCATION NAMES ${LIB_SEARCH_NAMES})
            endif()

            if(${LIB}_FULL_LOCATION)
                set(LIB_FULL_PATH "${${LIB}_FULL_LOCATION}")
                unset(${LIB}_FULL_LOCATION CACHE)
            else()
                # If not found, use the library name as-is
                set(LIB_FULL_PATH "${LIB}")
            endif()
        endif()

        list(APPEND LIB_FLAGS_RAW "${LIB_FULL_PATH}")
    endforeach()
endforeach()

# Collect source files
set(SRC_FLAGS "")
foreach(SRC ${SRCS})
    string(APPEND SRC_FLAGS "${SRC} ")
endforeach()

################################################################################
# Compile MEX S-Function
################################################################################

# Platform-specific optimization flags
if(WIN32)
    # Windows: Use COMPFLAGS for MSVC with /O2 optimization and /MT static runtime
    set(OPTIMIZATION_FLAGS "COMPFLAGS=\"$COMPFLAGS /O2 /MT\"")
else()
    # Unix/Linux/macOS: Use CFLAGS with -O3
    set(OPTIMIZATION_FLAGS "CFLAGS='\$CFLAGS -O3'")
endif()

set(BUILD_FLAGS "-v ${OPTIMIZATION_FLAGS} ${SRC_FLAGS} ${CUSTOM_COMPILE_FLAGS} ${INCLUDE_FLAGS} ${LIB_FLAGS}")

message(STATUS "")
message(STATUS "========================================================================")
message(STATUS "Compiling S-Function MEX file...")
message(STATUS "========================================================================")
message(STATUS "MEX Compiler: ${MEX_COMPILER}")
message(STATUS "Build flags: ${BUILD_FLAGS}")
message(STATUS "")

if(WIN32)
    # On Windows, execute MEX directly without shell wrapping
    # Separate arguments into a list for proper quoting
    separate_arguments(BUILD_FLAGS_LIST WINDOWS_COMMAND ${BUILD_FLAGS})
    execute_process(
        COMMAND "${MEX_COMPILER}" ${BUILD_FLAGS_LIST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE MEX_RESULT
    )
else()
    # On Unix, use shell to handle complex quoting
    execute_process(
        COMMAND bash -c "${MEX_COMPILER} ${BUILD_FLAGS}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE MEX_RESULT
    )
endif()

if(NOT MEX_RESULT EQUAL "0")
    message(FATAL_ERROR "S-Function compilation failed with exit code: ${MEX_RESULT}")
endif()

################################################################################
# Generate Configuration Files for Simulink Code Generation
################################################################################

# Debug: Show what will be written to files
message(STATUS "")
message(STATUS "Link directories for code generation:")
foreach(LINK_DIR ${LINK_DIRS_RAW})
    message(STATUS "  ${LINK_DIR}")
endforeach()

message(STATUS "")
message(STATUS "Library files for code generation:")
foreach(LIB_FILE ${LIB_FLAGS_RAW})
    message(STATUS "  ${LIB_FILE}")
endforeach()

# Write configuration files
write_file("${CMAKE_SOURCE_DIR}/include_directories.txt" "${INCLUDE_FLAGS_RAW}")
write_file("${CMAKE_SOURCE_DIR}/link_directories.txt" "${LINK_DIRS_RAW}")
write_file("${CMAKE_SOURCE_DIR}/link_libraries.txt" "${LIB_FLAGS_RAW}")
write_file("${CMAKE_SOURCE_DIR}/source_files.txt" "${SRC_FLAGS}")

message(STATUS "")
message(STATUS "========================================================================")
message(STATUS "S-Function compilation completed successfully!")
message(STATUS "========================================================================")
message(STATUS "")
message(STATUS "Generated MEX file: ${CMAKE_CURRENT_BINARY_DIR}/matlab_simulink_s_function.mex*")
message(STATUS "")
message(STATUS "Configuration files for Simulink code generation:")
message(STATUS "  - include_directories.txt  (Include directories)")
message(STATUS "  - link_directories.txt     (Library search paths)")
message(STATUS "  - link_libraries.txt       (Libraries with full paths)")
message(STATUS "  - source_files.txt         (Source files)")
message(STATUS "")
message(STATUS "To use these in Simulink code generation:")
message(STATUS "  1. Open Model Configuration Parameters")
message(STATUS "  2. Navigate to Code Generation > Custom Code")
message(STATUS "  3. Add the contents of these files to:")
message(STATUS "     - Include directories   -> 'Include directories'")
message(STATUS "     - Link directories      -> 'Library search path'")
message(STATUS "     - Link libraries        -> 'Libraries'")
message(STATUS "     - Source files          -> 'Source files'")
message(STATUS "========================================================================")
message(STATUS "")
