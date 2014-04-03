macro(grab_source DIR SOURCE_VAR) 
    file(GLOB TEMP_SOURCE_FILES "${DIR}/*.h" "${DIR}/*.cpp" "${DIR}/*.c")
    #list(APPEND ${SOURCE_VAR} ${TEMP_SOURCE_FILES} PARENT_SCOPE)
    list(APPEND ${SOURCE_VAR} ${TEMP_SOURCE_FILES})
endmacro()


grab_source(cel3ds SOURCE_FILES)
grab_source(celengine SOURCE_FILES)
grab_source(celephem SOURCE_FILES)
grab_source(celmath SOURCE_FILES)
grab_source(celmodel SOURCE_FILES)
grab_source(celtxf SOURCE_FILES)
grab_source(celutil SOURCE_FILES)

grab_source(celestia SOURCE_FILES)

LIST(SORT SOURCE_FILES)
#message(STATUS ${SOURCE_FILES})
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/celutil/windirector.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/celutil/wintimer.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/celutil/winutil.cpp)
#message(FATAL_ERROR ${CMAKE_CURRENT_SOURCE_DIR}/celutil/winutil.cpp)
#message(FATAL_ERROR ${SOURCE_FILES})
#message(STATUS ${SOURCE_FILES})

#message(FATAL_ERROR ${SOURCE_FILES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(celestia ${SOURCE_FILES})
target_link_libraries(celestia ${SHARED_LIBS})

