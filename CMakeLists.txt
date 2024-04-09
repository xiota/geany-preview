cmake_minimum_required(VERSION 3.21...3.45)

project(preview
  VERSION 0.1.0
  DESCRIPTION "Geany plugin to show preview of lightweight markup languages"
  HOMEPAGE_URL "https://github.com/xiota/geany-preview"
)

add_library(preview SHARED)

set_target_properties(preview PROPERTIES 
  PREFIX ""
  OUTPUT_NAME "preview"
  SUFFIX ".so"
  ENABLE_EXPORTS ON
)

## Options
option(PREVIEW_PDF_EXPORT "Export Fountain documents to PDF" ON)

## Build options (respecting user settings)
if(NOT DEFINED ENV{CFLAGS} AND NOT CMAKE_C_FLAGS)
  set(CMAKE_C_FLAGS "-march=native -O3")
endif()

if(NOT DEFINED ENV{CXXFLAGS}  AND NOT CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS "-march=native -O3")
else()
endif()

## Depends
find_package(PkgConfig REQUIRED)

# Required
pkg_check_modules(PC_DEPS REQUIRED IMPORTED_TARGET
  geany
  libcmark-gfm
  webkit2gtk-4.1
)
target_link_libraries(preview PUBLIC PkgConfig::PC_DEPS)

# Optional
pkg_check_modules(podofo IMPORTED_TARGET libpodofo>=0.10.0)

if (PREVIEW_PDF_EXPORT)
  if (NOT podofo_FOUND)
    message(SEND_ERROR "  PoDoFo not found.  Cannot export PDF.")
  elseif(podofo_VERSION VERSION_LESS 0.10.0)
    message(SEND_ERROR "  Unsupported PoDoFo version ${podofo_VERSION}")
  elseif(podofo_VERSION VERSION_GREATER_EQUAL 0.11.0)
    message(STATUS "  Unsupported PoDoFo version ${podofo_VERSION}")
  else()
    target_link_libraries(preview PUBLIC PkgConfig::podofo)
  endif()
else()
    message(STATUS "  - PREVIEW_PDF_EXPORT=OFF.  PDF export disabled.")
endif()

## Install paths
include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif()

if(CMAKE_INSTALL_LIBDIR_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "..." FORCE)
endif()

# /usr/lib/geany
if (NOT DEFINED GEANY_PLUGIN_INSTALL)
  set(GEANY_PLUGIN_INSTALL "${CMAKE_INSTALL_LIBDIR}/geany")
endif()

# /usr/share/geany-plugins
if (NOT DEFINED GEANY_PLUGIN_DATA)
  set(GEANY_PLUGIN_DATA "${CMAKE_INSTALL_DATADIR}/geany-plugins")
endif()

# /usr/share/geany-plugins/preview
if (NOT DEFINED PREVIEW_INSTALL_DATA)
  set(PREVIEW_INSTALL_DATA "${CMAKE_INSTALL_PREFIX}/${GEANY_PLUGIN_DATA}/${PROJECT_NAME}")
endif()

# /usr/share/doc/preview
if (NOT DEFINED PREVIEW_INSTALL_DOCDIR)
  set(PREVIEW_INSTALL_DOCDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")
endif()

## Sources
add_subdirectory(data)
add_subdirectory(docs)
add_subdirectory(src)
