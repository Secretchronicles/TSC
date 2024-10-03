if (USE_SYSTEM_MRUBY)
  find_path(MRuby_INCLUDE_DIR mruby.h)
  find_library(MRuby_LIBRARIES mruby mruby_core)

  message("-- Scripting engine enabled; found mruby at ${MRuby_LIBRARIES}")
else()
  message("-- Scripting engine enabled: building mruby statically")

  # mruby requires Bison and ruby to compile.
  find_package(BISON REQUIRED)
  find_package(Ruby REQUIRED)


  set(MRuby_LIBRARIES "${TSC_BINARY_DIR}/mruby/build/host/lib/libmruby.a" "${TSC_BINARY_DIR}/mruby/build/host/lib/libmruby_core.a")

  ExternalProject_Add(
    mruby
    DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory "${TSC_SOURCE_DIR}/../mruby/mruby" "${TSC_BINARY_DIR}/mruby"
    PATCH_COMMAND patch -p1 < "${TSC_SOURCE_DIR}/../mruby-werror.patch"
    SOURCE_DIR "${TSC_BINARY_DIR}/mruby"
    CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ./minirake MRUBY_CONFIG=${TSC_SOURCE_DIR}/mruby_tsc_build_config.rb TSC_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    BUILD_BYPRODUCTS ${MRuby_LIBRARIES}
    INSTALL_COMMAND "")

  set(MRuby_INCLUDE_DIR ${TSC_SOURCE_DIR}/../mruby/mruby/include)
endif()
