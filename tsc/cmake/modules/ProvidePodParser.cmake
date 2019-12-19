if (USE_SYSTEM_PODPARSER)
  find_path(PodParser_INCLUDE_DIRS pod.hpp)
  find_library(PodParser_LIBRARIES pod-cpp)
  message("-- Found pod-cpp at ${PodParser_LIBRARIES}")

  add_library(PodParser::PodParser UNKNOWN IMPORTED)
  set_target_properties(PodParser::PodParser PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${PodParser_INCLUDE_DIRS}"
                        IMPORTED_LOCATION "${PodParser_LIBRARIES}"
                        IMPORTED_LINK_INTERFACE_LANGUAGES "C++")
else()
  message("-- Building pod-cpp statically")
  add_library(podparser STATIC
    "${TSC_SOURCE_DIR}/../pod-parser/pod.cpp"
    "${TSC_SOURCE_DIR}/../pod-parser/pod.hpp")

  set(PodParser_LIBRARIES podparser)
  set(PodParser_INCLUDE_DIRS "${TSC_SOURCE_DIR}/../pod-parser")
  set_target_properties(podparser PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${PodParser_INCLUDE_DIRS}")

  add_library(PodParser::PodParser ALIAS podparser)
endif()
