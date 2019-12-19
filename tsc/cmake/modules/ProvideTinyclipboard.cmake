if (USE_SYSTEM_TINYCLIPBOARD)
  find_path(Tinyclipboard_INCLUDE_DIRS tinyclipboard.h)
  find_library(Tinyclipboard_LIBRARIES tinyclipboard)

  if(NOT Tinyclipboard_LIBRARIES)
    message(FATAL_ERROR "-- Failed to find system tinyclipboard")
  endif()

  message("-- Found tinyclipboard at ${Tinyclipboard_LIBRARIES}")

  add_library(Tinyclipboard::Tinyclipboard UNKNOWN IMPORTED)
  set_target_properties(Tinyclipboard::Tinyclipboard PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${Tinyclipboard_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${Tinyclipboard_LIBRARIES}"
                        IMPORTED_LINK_INTERFACE_LANGUAGES "C")
else()
  message("-- Building tinyclipboard statically")
  add_library(tinyclipboard STATIC
    "${TSC_SOURCE_DIR}/../tinyclipboard/src/tinyclipboard.c"
    "${TSC_SOURCE_DIR}/../tinyclipboard/include/tinyclipboard.h")

  set(Tinyclipboard_LIBRARIES tinyclipboard)
  set(Tinyclipboard_INCLUDE_DIRS "${TSC_SOURCE_DIR}/../tinyclipboard/include")
  set_target_properties(tinyclipboard PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${Tinyclipboard_INCLUDE_DIRS}")

  add_library(Tinyclipboard::Tinyclipboard ALIAS tinyclipboard)
endif()
