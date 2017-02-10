cmake_minimum_required (VERSION 2.8.10)
project (cmrcz)

# Use C99 (we need complex.h and getopt.h)
set(CMAKE_C_FLAGS "-std=gnu99")

set( CMAKE_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

add_executable(mrcz "${CMAKE_CURRENT_SOURCE_DIR}/mrcz.c")
add_library(mrcz_static STATIC "${CMAKE_CURRENT_SOURCE_DIR}/mrcz.c")
add_library(mrcz_shared SHARED "${CMAKE_CURRENT_SOURCE_DIR}/mrcz.c")

# parse the full version numbers from mrcz.h
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/mrcz.h _mrcz_h_contents)
string(REGEX REPLACE ".*#define[ \t]+MRCZ_VERSION_MAJOR[ \t]+([0-9]+).*"
     "\\1" CMRCZ_VERSION_MAJOR ${_mrcz_h_contents})
string(REGEX REPLACE ".*#define[ \t]+MRCZ_VERSION_MINOR[ \t]+([0-9]+).*"
    "\\1" CMRCZ_VERSION_MINOR ${_mrcz_h_contents})
string(REGEX REPLACE ".*#define[ \t]+MRCZ_VERSION_RELEASE[ \t]+([0-9]+).*"
    "\\1" CMRCZ_VERSION_PATCH ${_mrcz_h_contents})
string(REGEX REPLACE ".*#define[ \t]+MRCZ_VERSION_STRING[ \t]+\"([-0-9A-Za-z.]+)\".*"
    "\\1" CMRCZ_VERSION_STRING ${_mrcz_h_contents})


# User editable flags
option (USE_BLOSC "Use blosc meta-compressor" ON)


# Pass in Cmake configuration settings to source
configure_file ( "${CMAKE_CURRENT_SOURCE_DIR}/mrcz_config.h.in"  "${CMAKE_CURRENT_SOURCE_DIR}/mrcz_config.h" )


# Compile flags
set(GCC_COVERAGE_COMPILE_FLAGS "-O3 -mavx2")
#if (USE_BLOSC)
#    set(GCC_COVERAGE_LINK_FLAGS "-lpthread")
#endif (USE_BLOSC)

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

# Add c-blosc as external library
if (USE_BLOSC)
    set(GCC_COVERAGE_LINK_FLAGS "-lpthread")

    include(ExternalProject)
    ExternalProject_Add( blosc
        URL https://github.com/Blosc/c-blosc/archive/master.zip
        # GIT_REPOSITORY https://github.com/Blosc/c-blosc.git
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/c-blosc

        # Not compatible with CMake 2.8.12: DOWNLOAD_NO_PROGRESS 1
    
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DDEACTIVATE_SNAPPY=ON -DPREFER_EXTERNAL_ZLIB=OFF -DPREFER_EXTERNAL_ZSTD=OFF
        CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}"
        CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}"
        UPDATE_COMMAND ""      
    )
    

    # ExternalProject_Get_Property(c-blosc INSTALL_DIR)

    include_directories( "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/include" )
    message( "c-blosc installed in: ${CMAKE_CURRENT_BINARY_DIR}/c-blosc" )  # Hrm this is empty
    
   
    ##### THREADS #####
    find_package(Threads REQUIRED)
    if(THREADS_HAVE_PTHREAD_ARG)
      set_property(TARGET mrcz PROPERTY COMPILE_OPTIONS "-pthread")
      set_property(TARGET mrcz PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
    endif()
   

    link_directories( "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/lib" )
    
    # TODO: revert to static linking
    # TODO: how to handle for linux versus windows?  CMake docs are a little 
    # sparse.  Linux is libblosc.so, Win32 is blosc.dll.
    if(WIN32)
        set(BLOSC_STATIC_LIB "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/lib/libblosc.lib" )
        set(BLOSC_SHARED_LIB "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/lib/blosc.lib" )
    else()
        set(BLOSC_STATIC_LIB "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/lib/libblosc.a" )
        set(BLOSC_SHARED_LIB "${CMAKE_CURRENT_BINARY_DIR}/c-blosc/lib/libblosc.so" )
    endif()
    message( "BLOSC_STATIC_LIB = ${BLOSC_STATIC_LIB}" )
    
    # Force blosc to be built first by adding dependencies
    add_dependencies( mrcz blosc )
    add_dependencies( mrcz_static blosc )
    add_dependencies( mrcz_shared blosc )

    # Add links to the static blosc library
    target_link_libraries( mrcz ${BLOSC_STATIC_LIB} )
    target_link_libraries( mrcz_static ${BLOSC_STATIC_LIB} )
    target_link_libraries( mrcz_shared ${BLOSC_SHARED_LIB} )
    # set_property(TARGET mrcz PROPERTY COMPILE_OPTIONS "-blosc")

    if(CMAKE_THREAD_LIBS_INIT)
      target_link_libraries(mrcz "${CMAKE_THREAD_LIBS_INIT}")
    endif()
endif (USE_BLOSC)


# If the build type is not set, default to Release.
set(CMRCZ_DEFAULT_BUILD_TYPE Release)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to '${CMRCZ_DEFAULT_BUILD_TYPE}'.")
    set(CMAKE_BUILD_TYPE ${CMRCZ_DEFAULT_BUILD_TYPE} CACHE STRING
        "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


# TODO: UPLOAD TO BITBUCKET DOWNLOADS SECTION
# https://confluence.atlassian.com/x/nYVpM
# https://bitbucket.org/site/master/issues/3251/add-custom-file-uploads-to-rest-api-bb
# ---
#Sorry for not closing this issue earlier. Files can be uploaded to the 
#"Downloads" artifact tab directly from the command line via the API.
#To upload files, perform a multipart/form-data POST containing one or more 
#files fields:
#$ curl -s -u emmrcz -X POST https://api.bitbucket.org/2.0/repositories/emmrcz/c-mrcz/downloads -F files=@mrcz_binary.tar.gz
#When a file is uploaded with the same name as an existing artifact, then the 
#existing file will be replaced.
#Note that this very similar to how the snippets API works, which also supports 
#standard multipart/form-data file uploads
#P.S.
#This is a normal API endpoint, supporting all authentication schemes. My example 
#uses HTTP Basic Auth, but be aware that this is incompatible with the use of 
#2-step-verification, so consider OAuth 2 or an API Key if 2-sv is important to you.
# ---
# So it looks like custom scripts are the best choice because we need a password?  Or 
# not?  Appveyor is already authorized as me to work with the API.
