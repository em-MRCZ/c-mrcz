cmake_minimum_required (VERSION 2.8.10)
project (cmrcz)



set( CMAKE_CURRENT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src" )

add_executable(mrcz "${CMAKE_CURRENT_SOURCE_DIR}/mrcz.c" )

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
# Tricky: CMake can't find the include unless we build c-blosc first:
# http://stackoverflow.com/questions/17446981/cmake-externalproject-add-and-findpackage/23570741


if (USE_BLOSC)

    include(ExternalProject)
    ExternalProject_Add( c-blosc
        # URL https://github.com/Blosc/c-blosc/archive/v1.11.1.tar.gz
        GIT_REPOSITORY https://github.com/Blosc/c-blosc.git
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/c-blosc
    
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DDEACTIVATE_SNAPPY=ON -DPREFER_EXTERNAL_ZLIB=OFF -DPREFER_EXTERNAL_ZSTD=OFF
        CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}"
        CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}"
        UPDATE_COMMAND ""      
    )
    

    # ExternalProject_Get_Property(c-blosc INSTALL_DIR)

    include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/c-blosc/include" )
    message( "c-blosc installed in: ${CMAKE_CURRENT_SOURCE_DIR}/c-blosc" )  # Hrm this is empty
    
   
    ##### THREADS #####
    find_package(Threads REQUIRED)
    if(THREADS_HAVE_PTHREAD_ARG)
      set_property(TARGET mrcz PROPERTY COMPILE_OPTIONS "-pthread")
      set_property(TARGET mrcz PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
    endif()
   
    #add_dependencies( mrcz c-blosc )
    #add_library( c-blosc STATIC INCLUDE )
    #link_directories( "${C_BLOSC_INSTALL_DIR}/lib" )
    link_directories( "${CMAKE_CURRENT_SOURCE_DIR}/c-blosc/lib" )
    # target_link_libraries( mrcz c-blosc )
    # Ok so we're not getting zlib, lz4lib, etc...
    target_link_libraries( mrcz ${CMAKE_CURRENT_SOURCE_DIR}/c-blosc/lib/libblosc.so )
    # set_property(TARGET mrcz PROPERTY COMPILE_OPTIONS "-blosc")

    if(CMAKE_THREAD_LIBS_INIT)
      target_link_libraries(mrcz "${CMAKE_THREAD_LIBS_INIT}")
    endif()
endif (USE_BLOSC)


# If the build type is not set, default to Release.
set(CMRCZ_DEFAULT_BUILD_TYPE Debug)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified. Defaulting to '${CMRCZ_DEFAULT_BUILD_TYPE}'.")
    set(CMAKE_BUILD_TYPE ${CMRCZ_DEFAULT_BUILD_TYPE} CACHE STRING
        "Choose the type of build." FORCE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()


