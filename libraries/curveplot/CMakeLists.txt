project (curveplot C)

include_directories(${CMAKE_SOURCE_DIR}/libraries/Eigen)
include_directories(${CMAKE_SOURCE_DIR}/libraries/glew/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(curveplot STATIC src/curveplot.cpp)
