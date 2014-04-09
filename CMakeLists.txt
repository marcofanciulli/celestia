# Required for statically linking the MSVC runtime
set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

project(Celestia)
cmake_minimum_required(VERSION 2.8)
include (GenerateExportHeader)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE DEBUG)
set(BUILD_SHARED_LIBS OFF)

set(TARGET_ARCHITECTURE "x64")
if(CMAKE_COMPILER_IS_GNUCXX)
    # Ensure we use C++ 11 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    # Ensure we generate position independent code 
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )    
    endif()
endif()

if(APPLE)
    # Ensure we use C++ 11 
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
endif()

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(TARGET_ARCHITECTURE "x86")
endif()

#list(APPEND SHARED_LIBS )

add_subdirectory(libraries/cspice)
set_target_properties(cspice PROPERTIES FOLDER "3rdparty")
list(APPEND SHARED_LIBS cspice)

# Cross platform access to shader functionality
add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
set_target_properties(glew PROPERTIES FOLDER "3rdparty")
list(APPEND SHARED_LIBS glew)

find_package(OpenGL REQUIRED) 
list(APPEND SHARED_LIBS ${OPENGL_LIBRARIES})

if((WIN32 OR APPLE))
    set(SDL_SHARED OFF CACHE BOOL "don't build shared library")
    add_subdirectory(libraries/sdl2)
    list(APPEND SHARED_LIBS SDL2-static)
    list(APPEND SHARED_LIBS SDL2main)
    set_target_properties(SDL2-static PROPERTIES FOLDER "3rdparty/SDL")
    set_target_properties(SDL2main PROPERTIES FOLDER "3rdparty/SDL")
    
    add_subdirectory(libraries/lua)
    set_target_properties(liblua PROPERTIES FOLDER "3rdparty")
    list(APPEND SHARED_LIBS liblua)
    add_definitions(-DLUA_VER=0x050100)

    if (WIN32)
        set(SIZEOF_VOIDP ${CMAKE_SIZEOF_VOID_P})
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
          set(ARCH_64 TRUE)
          set(PROCESSOR_ARCH "x64")
        else()
          set(ARCH_64 FALSE)
          set(PROCESSOR_ARCH "x86")
        endif()
        link_directories($ENV{DXSDK_DIR}\\lib\\${PROCESSOR_ARCH})
    endif()

    # zlib
    add_subdirectory(libraries/zlib)
    set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libraries/zlib)
    include_directories(${CMAKE_BINARY_DIR}/libraries/zlib)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/zlib)
    list(APPEND SHARED_LIBS zlib)
    set_target_properties(zlib PROPERTIES FOLDER "3rdparty")

    # png
    set(PNG_STATIC ON CACHE BOOL "Build PNG static library")
    set(PNG_TESTS OFF CACHE BOOL "Build PNG tests")
    set(PNG_SHARED OFF CACHE BOOL "Build PNG shared library")
    add_subdirectory(libraries/libpng)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/libpng)
    include_directories(${CMAKE_BINARY_DIR}/libraries/libpng)
    list(APPEND SHARED_LIBS png)
    set_target_properties(png PROPERTIES FOLDER "3rdparty")

    add_subdirectory(libraries/libjpeg)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/libjpeg)
    include_directories(${CMAKE_BINARY_DIR}/libraries/libjpeg)
    list(APPEND SHARED_LIBS jpeg)
    set_target_properties(jpeg PROPERTIES FOLDER "3rdparty")

    include_directories(${CMAKE_SOURCE_DIR}/windows)


else()
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
    list(APPEND SHARED_LIBS ${JPEG_LIBRARIES})
    find_package(PNG REQUIRED)
    include_directories(${PNG_INCLUDE_DIR})
    list(APPEND SHARED_LIBS ${PNG_LIBRARIES})
endif()

include_directories(${CMAKE_SOURCE_DIR}/libraries/Eigen)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)

add_subdirectory(libraries/curveplot)
set_target_properties(curveplot PROPERTIES FOLDER "3rdparty")
list(APPEND SHARED_LIBS curveplot)

# Add debug output from the Oculus SDK
add_definitions(-DOVR_BUILD_DEBUG)

# The primary SDK artifact, a static library for Oculus access 
add_subdirectory(libraries/OculusSDK/LibOVR)
set_target_properties(OculusVR PROPERTIES FOLDER "3rdparty")
list(APPEND SHARED_LIBS OculusVR)
 
# Make sure to set up the OVR include paths
include_directories(libraries/OculusSDK/LibOVR/Include)
include_directories(libraries/OculusSDK/LibOVR/Src)
include_directories(libraries/OculusSDK/LibOVR/Src/Kernel)
include_directories(libraries/OculusSDK/LibOVR/Src/Util)

include_directories(libraries/glm)
include_directories(libraries/gl)

include_directories(${CMAKE_BINARY_DIR}/libraries/lua)
include_directories(${CMAKE_SOURCE_DIR}/libraries/lua/src)
include_directories(${CMAKE_SOURCE_DIR}/libraries/lua/etc)
include_directories(${CMAKE_SOURCE_DIR}/libraries/sdl2/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/cspice/include)
include_directories(${CMAKE_SOURCE_DIR}/libraries/curveplot/include)

add_subdirectory(src)

