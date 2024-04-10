cmake_minimum_required(VERSION 3.21...3.45)

project(preview
  VERSION 0.1.0
  DESCRIPTION "Geany plugin to show preview of lightweight markup languages"
  HOMEPAGE_URL "https://github.com/xiota/geany-preview"
)

add_library(preview SHARED)

set_target_properties(preview
  PROPERTIES
    PREFIX ""
    OUTPUT_NAME "preview"
    SUFFIX ".so"
    ENABLE_EXPORTS ON
)

## Modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(build-flags)
include(default-paths)

include(tristate-option)

find_package(PkgConfig REQUIRED)


## Options
tristate_option(ENABLE_PDF_EXPORT "AUTO" "Feature: Export Fountain screenplays to PDF")


## Depends
# required
pkg_check_modules(PC_DEPS REQUIRED IMPORTED_TARGET
  geany
  libcmark-gfm
  webkit2gtk-4.1
)
target_link_libraries(preview PUBLIC PkgConfig::PC_DEPS)

# podofo
pkg_check_modules(podofo ${ENABLE_PDF_EXPORT_REQUIRED} IMPORTED_TARGET libpodofo>=0.10.0)
if(podofo_FOUND AND NOT ENABLE_PDF_EXPORT_OFF)
  target_link_libraries(preview PUBLIC PkgConfig::podofo)
  add_definitions(-DENABLE_PDF_EXPORT=1)
endif()
if(ENABLE_PDF_EXPORT_OFF)
  set(podofo_INCLUDE_DIRS "")
  set(podofo_LIBRARIES "")
  message(STATUS "  PDF export is disabled.")
endif()


## Sources
add_subdirectory(data)
add_subdirectory(docs)
add_subdirectory(src)
