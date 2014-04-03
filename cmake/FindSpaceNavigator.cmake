set(SPNAV_POSSIBLE_PATHS
    /usr
)

find_path(SPNAV_INCLUDE_DIR 
    NAMES spnav.h
    PATH_SUFFIXES "include"
    PATHS ${SPNAV_POSSIBLE_PATHS}
)

find_library(SPNAV_LIBRARY 
    NAMES spnav
    PATH_SUFFIXES "lib"
    PATHS ${SPNAV_POSSIBLE_PATHS}
)

set(SPNAV_LIBRARIES ${SPNAV_LIBRARY})

find_package_handle_standard_args(SPNAV DEFAULT_MSG SPNAV_LIBRARY SPNAV_INCLUDE_DIR)
