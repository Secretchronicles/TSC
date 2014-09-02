find_path(PhysFS_INCLUDE_DIR physfs.h)
find_library(PhysFS_LIBRARY physfs)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PhysFS DEFAULT_MSG PhysFS_INCLUDE_DIR PhysFS_LIBRARY)
set(PhysFS_LIBRARIES ${PhysFS_LIBRARY})
set(PhysFS_INCLUDE_DIRS ${PhysFS_INCLUDE_DIR})
