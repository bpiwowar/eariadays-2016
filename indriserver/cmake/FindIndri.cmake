# find Indri
#
# exports:
#
#   INDRI_FOUND
#   INDRI_INCLUDE_DIRS
#   INDRI_LIBRARIES
#

#include(FindPkgConfig)
include(FindPackageHandleStandardArgs)

# Use pkg-config to get hints about paths
# pkg_check_modules(INDRI_PKGCONF REQUIRED indri)

# Include dir
find_path(INDRI_INCLUDE_DIR
        NAMES indri/Index.hpp
        PATHS ${INDRI_PKGCONF_INCLUDE_DIRS}
        )

# Finally the library itself
find_library(INDRI_LIBRARY
        NAMES indri
        PATHS ${INDRI_PKGCONF_LIBRARY_DIRS}
        )

FIND_PACKAGE_HANDLE_STANDARD_ARGS(INDRI DEFAULT_MSG INDRI_LIBRARY INDRI_INCLUDE_DIR)


if(INDRI_PKGCONF_FOUND)
    set(INDRI_LIBRARIES ${INDRI_LIBRARY} ${INDRI_PKGCONF_LIBRARIES})
    set(INDRI_INCLUDE_DIRS ${INDRI_INCLUDE_DIR} ${INDRI_PKGCONF_INCLUDE_DIRS})
    set(INDRI_FOUND yes)
else()
    set(INDRI_LIBRARIES)
    set(INDRI_INCLUDE_DIRS)
    set(INDRI_FOUND no)
endif()
