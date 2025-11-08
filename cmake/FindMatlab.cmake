################################################################################
# FindMatlab.cmake
# ================
#
# Find MATLAB installation and provide useful variables for building MEX files
# and S-functions.
#
# This module finds MATLAB installations on Windows, Linux, and macOS, and
# provides CMake variables for the MATLAB root directory, MEX compiler, include
# directories, and libraries.
#
# Usage:
#   find_package(Matlab REQUIRED)
#
# Components (optional):
#   MEX_COMPILER - Require MEX compiler
#   SIMULINK     - Require Simulink installation
#
# Example:
#   find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER SIMULINK)
#
# User-Settable Variables:
#   MATLAB_ROOT          - MATLAB installation root directory (optional hint)
#   MATLAB_BIN_DIR       - MATLAB bin directory (optional hint)
#   MATLAB_VERSION       - Specific MATLAB version to find (e.g., "R2023b")
#
# Output Variables:
#   Matlab_FOUND         - True if MATLAB is found
#   MATLAB_ROOT          - MATLAB installation root directory
#   MATLAB_BIN_DIR       - MATLAB bin directory
#   MATLAB_INCLUDE_DIR   - MATLAB include directory for MEX files
#   MATLAB_MEX_COMPILER  - Full path to MEX compiler executable
#   MATLAB_LIBRARIES     - MATLAB libraries for linking
#   MATLAB_VERSION       - MATLAB version string (e.g., "R2023b")
#   MATLAB_VERSION_MAJOR - Major version number
#   MATLAB_VERSION_MINOR - Minor version number
#   MATLAB_HAS_SIMULINK  - True if Simulink is installed
#
# Cache Variables:
#   MATLAB_ROOT_DIR      - Cached MATLAB root directory
################################################################################

# Include required CMake modules
include(FindPackageHandleStandardArgs)

################################################################################
# Platform-specific MATLAB search paths
################################################################################

set(_MATLAB_SEARCH_PATHS "")

if(WIN32)
    # Windows: Check common installation directories
    list(APPEND _MATLAB_SEARCH_PATHS
        "C:/Program Files/MATLAB"
        "C:/Program Files (x86)/MATLAB"
    )

    # Add environment variable paths (handle special characters in variable names)
    if(DEFINED ENV{ProgramFiles})
        list(APPEND _MATLAB_SEARCH_PATHS "$ENV{ProgramFiles}/MATLAB")
    endif()

    # Handle ProgramFiles(x86) - CMake needs special syntax for parentheses
    file(TO_CMAKE_PATH "$ENV{ProgramW6432}" _PROGRAM_FILES_64)
    if(_PROGRAM_FILES_64)
        list(APPEND _MATLAB_SEARCH_PATHS "${_PROGRAM_FILES_64}/MATLAB")
    endif()

    # Alternative method for x86 program files using registry-style path
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit CMake, add both 64-bit and 32-bit paths
        file(TO_CMAKE_PATH "$ENV{ProgramW6432}" _PF64)
        if(_PF64)
            list(APPEND _MATLAB_SEARCH_PATHS "${_PF64}/MATLAB")
        endif()
        # On 64-bit Windows, ProgramFiles points to x86 location when run from 32-bit process
        set(_PFx86 "C:/Program Files (x86)")
        if(EXISTS "${_PFx86}")
            list(APPEND _MATLAB_SEARCH_PATHS "${_PFx86}/MATLAB")
        endif()
    endif()

    # Check for MATLAB installations in registry (if available)
    set(_MATLAB_VERSIONS R2024b R2024a R2023b R2023a R2022b R2022a R2021b R2021a R2020b R2020a)
    foreach(_VERSION ${_MATLAB_VERSIONS})
        list(APPEND _MATLAB_SEARCH_PATHS
            "C:/Program Files/MATLAB/${_VERSION}"
            "C:/Program Files (x86)/MATLAB/${_VERSION}"
        )
    endforeach()

elseif(APPLE)
    # macOS: Check Applications directory
    list(APPEND _MATLAB_SEARCH_PATHS
        "/Applications"
    )

    set(_MATLAB_VERSIONS R2024b R2024a R2023b R2023a R2022b R2022a R2021b R2021a R2020b R2020a)
    foreach(_VERSION ${_MATLAB_VERSIONS})
        list(APPEND _MATLAB_SEARCH_PATHS
            "/Applications/MATLAB_${_VERSION}.app"
        )
    endforeach()

else()
    # Linux: Check common installation directories
    list(APPEND _MATLAB_SEARCH_PATHS
        "/usr/local/MATLAB"
        "/opt/MATLAB"
        "$ENV{HOME}/MATLAB"
        "/usr/local"
    )

    set(_MATLAB_VERSIONS R2024b R2024a R2023b R2023a R2022b R2022a R2021b R2021a R2020b R2020a)
    foreach(_VERSION ${_MATLAB_VERSIONS})
        list(APPEND _MATLAB_SEARCH_PATHS
            "/usr/local/MATLAB/${_VERSION}"
            "/opt/MATLAB/${_VERSION}"
        )
    endforeach()
endif()

# Add user-provided hints
if(DEFINED MATLAB_ROOT)
    list(INSERT _MATLAB_SEARCH_PATHS 0 "${MATLAB_ROOT}")
endif()

if(DEFINED MATLAB_BIN_DIR)
    get_filename_component(_MATLAB_ROOT_FROM_BIN "${MATLAB_BIN_DIR}" DIRECTORY)
    list(INSERT _MATLAB_SEARCH_PATHS 0 "${_MATLAB_ROOT_FROM_BIN}")
endif()

################################################################################
# Find MATLAB root directory
################################################################################

find_path(MATLAB_ROOT_DIR
    NAMES bin/matlab bin/matlab.exe
    PATHS ${_MATLAB_SEARCH_PATHS}
    DOC "MATLAB installation root directory"
)

# If found, set MATLAB_ROOT
if(MATLAB_ROOT_DIR)
    set(MATLAB_ROOT "${MATLAB_ROOT_DIR}")
else()
    # Try to find matlab executable directly
    find_program(_MATLAB_EXE
        NAMES matlab
        PATHS ${_MATLAB_SEARCH_PATHS}
        PATH_SUFFIXES bin
    )

    if(_MATLAB_EXE)
        get_filename_component(_MATLAB_BIN_DIR "${_MATLAB_EXE}" DIRECTORY)
        get_filename_component(MATLAB_ROOT "${_MATLAB_BIN_DIR}" DIRECTORY)
        set(MATLAB_ROOT_DIR "${MATLAB_ROOT}" CACHE PATH "MATLAB root directory" FORCE)
    endif()
endif()

################################################################################
# Find MATLAB components
################################################################################

if(MATLAB_ROOT)
    # Set MATLAB bin directory
    set(MATLAB_BIN_DIR "${MATLAB_ROOT}/bin")

    # Platform-specific paths
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_MATLAB_ARCH "win64")
        else()
            set(_MATLAB_ARCH "win32")
        endif()
    elseif(APPLE)
        set(_MATLAB_ARCH "maci64")
    else()
        set(_MATLAB_ARCH "glnxa64")
    endif()

    # Find MEX compiler
    find_program(MATLAB_MEX_COMPILER
        NAMES mex mex.bat
        PATHS "${MATLAB_BIN_DIR}"
        NO_DEFAULT_PATH
        DOC "MATLAB MEX compiler"
    )

    # Find MATLAB include directory
    find_path(MATLAB_INCLUDE_DIR
        NAMES mex.h
        PATHS
            "${MATLAB_ROOT}/extern/include"
            "${MATLAB_ROOT}/simulink/include"
        NO_DEFAULT_PATH
        DOC "MATLAB include directory"
    )

    # Find MATLAB libraries
    set(_MATLAB_LIB_PATHS
        "${MATLAB_ROOT}/bin/${_MATLAB_ARCH}"
        "${MATLAB_ROOT}/extern/lib/${_MATLAB_ARCH}"
        "${MATLAB_ROOT}/extern/lib/${_MATLAB_ARCH}/microsoft"
    )

    find_library(MATLAB_MEX_LIBRARY
        NAMES libmex mex
        PATHS ${_MATLAB_LIB_PATHS}
        NO_DEFAULT_PATH
    )

    find_library(MATLAB_MX_LIBRARY
        NAMES libmx mx
        PATHS ${_MATLAB_LIB_PATHS}
        NO_DEFAULT_PATH
    )

    find_library(MATLAB_MAT_LIBRARY
        NAMES libmat mat
        PATHS ${_MATLAB_LIB_PATHS}
        NO_DEFAULT_PATH
    )

    # Collect all libraries
    set(MATLAB_LIBRARIES "")
    if(MATLAB_MEX_LIBRARY)
        list(APPEND MATLAB_LIBRARIES ${MATLAB_MEX_LIBRARY})
    endif()
    if(MATLAB_MX_LIBRARY)
        list(APPEND MATLAB_LIBRARIES ${MATLAB_MX_LIBRARY})
    endif()
    if(MATLAB_MAT_LIBRARY)
        list(APPEND MATLAB_LIBRARIES ${MATLAB_MAT_LIBRARY})
    endif()

    # Detect MATLAB version
    if(EXISTS "${MATLAB_ROOT}/VersionInfo.xml")
        file(READ "${MATLAB_ROOT}/VersionInfo.xml" _VERSION_INFO)

        # Extract release string (e.g., R2023b)
        string(REGEX MATCH "<release>([^<]+)</release>" _RELEASE_MATCH "${_VERSION_INFO}")
        if(_RELEASE_MATCH)
            set(MATLAB_VERSION "${CMAKE_MATCH_1}")
        endif()

        # Extract version numbers
        string(REGEX MATCH "<version>([0-9]+)\\.([0-9]+)" _VERSION_MATCH "${_VERSION_INFO}")
        if(_VERSION_MATCH)
            set(MATLAB_VERSION_MAJOR "${CMAKE_MATCH_1}")
            set(MATLAB_VERSION_MINOR "${CMAKE_MATCH_2}")
        endif()
    endif()

    # Check for Simulink
    if(EXISTS "${MATLAB_ROOT}/simulink")
        set(MATLAB_HAS_SIMULINK TRUE)
    else()
        set(MATLAB_HAS_SIMULINK FALSE)
    endif()

    # Additional Simulink include directories
    if(MATLAB_HAS_SIMULINK)
        set(MATLAB_SIMULINK_INCLUDE_DIR "${MATLAB_ROOT}/simulink/include")
    endif()
endif()

################################################################################
# Handle component requirements
################################################################################

set(_MATLAB_REQUIRED_VARS MATLAB_ROOT MATLAB_BIN_DIR)

if(Matlab_FIND_COMPONENTS)
    foreach(_COMPONENT ${Matlab_FIND_COMPONENTS})
        if(_COMPONENT STREQUAL "MEX_COMPILER")
            list(APPEND _MATLAB_REQUIRED_VARS MATLAB_MEX_COMPILER)

        elseif(_COMPONENT STREQUAL "SIMULINK")
            if(NOT MATLAB_HAS_SIMULINK)
                if(Matlab_FIND_REQUIRED_SIMULINK)
                    message(FATAL_ERROR "Simulink not found in MATLAB installation: ${MATLAB_ROOT}")
                endif()
            endif()

        else()
            message(WARNING "Unknown Matlab component: ${_COMPONENT}")
        endif()
    endforeach()
endif()

################################################################################
# Standard package handling
################################################################################

find_package_handle_standard_args(Matlab
    REQUIRED_VARS ${_MATLAB_REQUIRED_VARS}
    VERSION_VAR MATLAB_VERSION
    FAIL_MESSAGE "Could not find MATLAB. Please set MATLAB_ROOT or MATLAB_BIN_DIR."
)

################################################################################
# Create imported target
################################################################################

if(Matlab_FOUND AND NOT TARGET Matlab::Matlab)
    add_library(Matlab::Matlab INTERFACE IMPORTED)

    if(MATLAB_INCLUDE_DIR)
        set_target_properties(Matlab::Matlab PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${MATLAB_INCLUDE_DIR}"
        )
    endif()

    if(MATLAB_LIBRARIES)
        set_target_properties(Matlab::Matlab PROPERTIES
            INTERFACE_LINK_LIBRARIES "${MATLAB_LIBRARIES}"
        )
    endif()
endif()

################################################################################
# Helper functions
################################################################################

# Function to compile MEX files
function(matlab_add_mex)
    set(options )
    set(oneValueArgs NAME OUTPUT_NAME)
    set(multiValueArgs SOURCES LINK_LIBRARIES INCLUDE_DIRECTORIES)
    cmake_parse_arguments(MEX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MATLAB_MEX_COMPILER)
        message(FATAL_ERROR "MEX compiler not found. Cannot compile MEX file.")
    endif()

    if(NOT MEX_NAME)
        message(FATAL_ERROR "matlab_add_mex: NAME is required")
    endif()

    if(NOT MEX_SOURCES)
        message(FATAL_ERROR "matlab_add_mex: SOURCES is required")
    endif()

    # Build MEX compile command
    set(_MEX_COMMAND "${MATLAB_MEX_COMPILER}" "-v")

    # Add sources
    foreach(_SRC ${MEX_SOURCES})
        list(APPEND _MEX_COMMAND "${_SRC}")
    endforeach()

    # Add include directories
    foreach(_INC ${MEX_INCLUDE_DIRECTORIES})
        list(APPEND _MEX_COMMAND "-I${_INC}")
    endforeach()

    # Add link libraries
    foreach(_LIB ${MEX_LINK_LIBRARIES})
        list(APPEND _MEX_COMMAND "-l${_LIB}")
    endforeach()

    # Set output name
    if(MEX_OUTPUT_NAME)
        list(APPEND _MEX_COMMAND "-output" "${MEX_OUTPUT_NAME}")
    else()
        list(APPEND _MEX_COMMAND "-output" "${MEX_NAME}")
    endif()

    # Add custom target
    add_custom_target(${MEX_NAME} ALL
        COMMAND ${_MEX_COMMAND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Compiling MEX file: ${MEX_NAME}"
        SOURCES ${MEX_SOURCES}
    )
endfunction()

################################################################################
# Print configuration (if verbose)
################################################################################

if(Matlab_FOUND AND NOT Matlab_FIND_QUIETLY)
    message(STATUS "MATLAB Configuration:")
    message(STATUS "  Version:        ${MATLAB_VERSION}")
    message(STATUS "  Root:           ${MATLAB_ROOT}")
    message(STATUS "  Bin directory:  ${MATLAB_BIN_DIR}")
    message(STATUS "  MEX compiler:   ${MATLAB_MEX_COMPILER}")
    message(STATUS "  Include dir:    ${MATLAB_INCLUDE_DIR}")
    message(STATUS "  Has Simulink:   ${MATLAB_HAS_SIMULINK}")
endif()

# Cleanup
mark_as_advanced(
    MATLAB_ROOT_DIR
    MATLAB_MEX_COMPILER
    MATLAB_INCLUDE_DIR
    MATLAB_MEX_LIBRARY
    MATLAB_MX_LIBRARY
    MATLAB_MAT_LIBRARY
)
