message("-- Scripting engine enabled: building mruby statically")

# mruby requires Bison and ruby to compile.
find_package(BISON REQUIRED)
find_package(Ruby REQUIRED)

set(MRuby_LIBRARIES "${TSC_BINARY_DIR}/mruby/build/host/lib/libmruby.a" "${TSC_BINARY_DIR}/mruby/build/host/lib/libmruby_core.a")

ExternalProject_Add(
  mruby
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory "${TSC_SOURCE_DIR}/../mruby/mruby" "${TSC_BINARY_DIR}/mruby"
  SOURCE_DIR "${TSC_BINARY_DIR}/mruby"
  CONFIGURE_COMMAND ""
  BUILD_IN_SOURCE 1
  BUILD_COMMAND
    ./minirake
      CC=${CMAKE_C_COMPILER} LD=${CMAKE_C_COMPILER}
      MRUBY_CONFIG=${TSC_SOURCE_DIR}/mruby_tsc_build_config.rb
  BUILD_BYPRODUCTS ${MRuby_LIBRARIES}
  INSTALL_COMMAND "")

set(MRuby_INCLUDE_DIR ${TSC_SOURCE_DIR}/../mruby/mruby/include)

add_library(MRuby::MRuby INTERFACE IMPORTED)
set_target_properties(MRuby::MRuby PROPERTIES
                      INTERFACE_INCLUDE_DIRECTORIES "${MRuby_INCLUDE_DIR}"
                      INTERFACE_LINK_LIBRARIES "${MRuby_LIBRARIES}")
