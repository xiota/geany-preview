
target_sources(preview
  PUBLIC
    pv_main.cc
  PRIVATE
    auxiliary.cc
    fountain.cc
    process.cc
    pv_formats.cc
    pv_settings.cc
)

## Generate header
configure_file(cmake.h.in cmake.h)
target_include_directories(
  preview PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}"
)

## Install
install(TARGETS preview DESTINATION "${GEANY_INSTALL_PLUGIN}")
