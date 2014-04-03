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
include_directories(${CMAKE_SOURCE_DIR}/libraries/cspice/include)
list(APPEND SHARED_LIBS cspice)

# Cross platform access to shader functionality
add_definitions(-DGLEW_STATIC)
add_subdirectory(libraries/glew)
set_target_properties(glew PROPERTIES FOLDER "3rdparty")
include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)
list(APPEND SHARED_LIBS glew)

include(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
list(APPEND SHARED_LIBS ${SDL2_LIBRARIES})

# GLFW - Cross platform OpenGL window creation and input handling
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
add_subdirectory(libraries/glfw)
set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)
list(APPEND EXAMPLE_LIBS glfw ${GLFW_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR}/libraries/Eigen)

add_subdirectory(libraries/curveplot)
set_target_properties(curveplot PROPERTIES FOLDER "3rdparty")
include_directories(${CMAKE_SOURCE_DIR}/libraries/curveplot/include)
list(APPEND SHARED_LIBS curveplot)

find_package(Lua51 REQUIRED) 
include_directories(${LUA_INCLUDE_DIR})
list(APPEND SHARED_LIBS ${LUA_LIBRARIES})
add_definitions(-DLUA_VER=0x050100)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

find_package(OpenGL REQUIRED) 
#include_directories(${LUA_INCLUDE_DIR})
list(APPEND SHARED_LIBS ${OPENGL_LIBRARIES})

#message(STATUS ${SHARED_LIBS})
# GLFW - Cross platform OpenGL window creation and input handling
#set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
#set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
#set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
#set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
#add_subdirectory(libraries/glfw)
#set_target_properties(glfw PROPERTIES FOLDER "3rdparty")
#include_directories(${CMAKE_SOURCE_DIR}/libraries/glfw/include)
#list(APPEND SHARED_LIBS glfw ${GLFW_LIBRARIES})

# Without OpenCV, we have to fall back on libpng
# which requires zlib.  For windows and apple we
# build the library.  For Unix systems we locate 
# the native package
if((WIN32 OR APPLE))
    # zlib
    add_subdirectory(libraries/zlib)
    set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libraries/zlib)
    include_directories(${CMAKE_BINARY_DIR}/libraries/zlib)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/zlib)
    list(APPEND SHARED_LIBS zlib)

    # png
    set(PNG_STATIC ON CACHE BOOL "Build PNG static library")
    set(PNG_TESTS OFF CACHE BOOL "Build PNG tests")
    set(PNG_SHARED OFF CACHE BOOL "Build PNG shared library")
    add_subdirectory(libraries/libpng)
    include_directories(${CMAKE_SOURCE_DIR}/libraries/libpng)
    include_directories(${CMAKE_BINARY_DIR}/libraries/libpng)
    list(APPEND SHARED_LIBS png)
    set_target_properties(zlib PROPERTIES FOLDER "3rdparty")
    set_target_properties(png PROPERTIES FOLDER "3rdparty")
else()
    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
    list(APPEND SHARED_LIBS ${JPEG_LIBRARIES})
    find_package(PNG REQUIRED)
    include_directories(${PNG_INCLUDE_DIR})
    list(APPEND SHARED_LIBS ${PNG_LIBRARIES})
endif()

add_subdirectory(src)

