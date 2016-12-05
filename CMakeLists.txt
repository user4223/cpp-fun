cmake_minimum_required(VERSION 2.8.11)
project(cpp-fun)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

GET_FILENAME_COMPONENT( SOURCE_ROOT . ABSOLUTE )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin)

add_compile_options(--std=c++14)
add_compile_options(-Wall)
add_compile_options(-Werror)

FIND_PACKAGE(Boost 1.45.0 REQUIRED) # COMPONENTS ...)

include_directories(${SOURCE_ROOT})
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(Monad)
