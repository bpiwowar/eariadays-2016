# find HTSLIB
#
# exports:
#
#   HTSLIB_FOUND
#   HTSLIB_INCLUDE_DIRS
#   HTSLIB_LIBRARIES
#

include(FindPkgConfig)
include(FindPackageHandleStandardArgs)

# Use pkg-config to get hints about paths
pkg_check_modules(HTSLIB_PKGCONF htslib)

# Include dir
find_path(HTSLIB_INCLUDE_DIR
        NAMES htslib/bgzf.h
        PATHS ${HTSLIB_PKGCONF_INCLUDE_DIRS}
        )

# Finally the library itself
find_library(HTSLIB_LIBRARY
        NAMES hts
        PATHS ${HTSLIB_PKGCONF_LIBRARY_DIRS}
        )

FIND_PACKAGE_HANDLE_STANDARD_ARGS(HTSLIB DEFAULT_MSG HTSLIB_LIBRARY HTSLIB_INCLUDE_DIR)


if(HTSLIB_PKGCONF_FOUND)
    set(HTSLIB_LIBRARIES ${HTSLIB_LIBRARY} ${HTSLIB_PKGCONF_LIBRARIES})
    set(HTSLIB_INCLUDE_DIRS ${HTSLIB_INCLUDE_DIR} ${HTSLIB_PKGCONF_INCLUDE_DIRS})
    set(HTSLIB_FOUND yes)
else()
    set(HTSLIB_LIBRARIES)
    set(HTSLIB_INCLUDE_DIRS)
    set(HTSLIB_FOUND no)
endif()
