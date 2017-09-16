cmake_minimum_required(VERSION 2.8)
project(cpp-fun)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

GET_FILENAME_COMPONENT( SOURCE_ROOT . ABSOLUTE )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
ELSE()
	ADD_COMPILE_OPTIONS(--std=c++14)
	ADD_COMPILE_OPTIONS(-Wall)
	ADD_COMPILE_OPTIONS(-Werror)
	LINK_LIBRARIES(stdc++)
ENDIF()

# Currently, we need some headers only
FIND_PACKAGE(Boost 1.61.0 REQUIRED)

find_package(Threads REQUIRED)   # We require thread support
include(ExternalProject)         # Enable ExternalProject module

ExternalProject_Add(
    gmock
    URL https://github.com/google/googletest/archive/release-1.8.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gmock
    INSTALL_COMMAND "" # Disable install step
)
ExternalProject_Get_Property(gmock source_dir binary_dir)
SET(GMOCK_SOURCE_DIR ${source_dir})
SET(GMOCK_BINARY_DIR ${binary_dir})

add_library(libgtest IMPORTED STATIC GLOBAL)
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${GMOCK_BINARY_DIR}/gtest/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

add_library(libgmock IMPORTED STATIC GLOBAL)
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${GMOCK_BINARY_DIR}/libgmock.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

SET(GMOCK_INCLUDE_DIRECTORIES
   "${GMOCK_SOURCE_DIR}/googlemock/include"
   "${GMOCK_SOURCE_DIR}/googletest/include")

SET(GMOCK_ALL_LIBRARIES 
   "${GMOCK_BINARY_DIR}/googlemock/gtest/libgtest.a" 
   "${GMOCK_BINARY_DIR}/googlemock/gtest/libgtest_main.a" 
   "${GMOCK_BINARY_DIR}/googlemock/libgmock.a" 
   "${GMOCK_BINARY_DIR}/googlemock/libgmock_main.a"
   pthread)
   
ExternalProject_Add(
    gbench
    URL https://github.com/google/benchmark/archive/v1.2.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gbench
    INSTALL_COMMAND "" # Disable install step
)
ExternalProject_Get_Property(gbench source_dir binary_dir)
SET(GBENCH_SOURCE_DIR ${source_dir})
SET(GBENCH_BINARY_DIR ${binary_dir})

add_library(libgbench IMPORTED STATIC GLOBAL)
set_target_properties(libgbench PROPERTIES "IMPORTED_LOCATION" 
	"${GBENCH_BINARY_DIR}/src/libbenchmark.a")
    
SET(GBENCH_INCLUDE_DIRECTORIES "${GBENCH_SOURCE_DIR}/gbench/include/")

SET(GBENCH_ALL_LIBRARIES "${GBENCH_BINARY_DIR}/src/libbenchmark.a")

INCLUDE_DIRECTORIES(${SOURCE_ROOT})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

ADD_SUBDIRECTORY(Processor)
ADD_SUBDIRECTORY(Monad)
ADD_SUBDIRECTORY(Semaphore)
ADD_SUBDIRECTORY(TypeChecksum)
