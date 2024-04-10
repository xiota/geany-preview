if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-tree builds are a terrible mistake.")
endif()

if(NOT DEFINED ENV{CFLAGS} AND "${CMAKE_C_FLAGS}" STREQUAL "")
  set(CMAKE_C_FLAGS "-march=native -O3")
endif()

if(NOT DEFINED ENV{CXXFLAGS} AND "${CMAKE_CXX_FLAGS}" STREQUAL "")
  set(CMAKE_CXX_FLAGS "-march=native -O3")
endif()

