cmake_minimum_required(VERSION 2.8)
project(cpp-fun)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

GET_FILENAME_COMPONENT( SOURCE_ROOT . ABSOLUTE )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SOURCE_ROOT}/bin/${BIN_PATH_POSTFIX})

IF(NOT MSVC)
	ADD_COMPILE_OPTIONS(--std=c++14)
	ADD_COMPILE_OPTIONS(-Wall)
	ADD_COMPILE_OPTIONS(-Werror)
	LINK_LIBRARIES(stdc++)
ENDIF()

# Currently, we need some headers only
# FIND_PACKAGE(Boost 1.45.0 REQUIRED)

# For the following gtest/gmock stuff see: http://www.kaizou.org/2014/11/gtest-cmake/
# This downloads, builds and provides gtest/gmock
find_package(Threads REQUIRED)   # We need thread support
include(ExternalProject)         # Enable ExternalProject CMake module

ExternalProject_Add(
    gmock
    URL https://github.com/google/googletest/archive/release-1.8.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gmock
    # Disable install step
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gmock source_dir binary_dir)

add_library(libgtest IMPORTED STATIC GLOBAL)
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/gtest/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

add_library(libgmock IMPORTED STATIC GLOBAL)
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/libgmock.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

SET(GMOCK_INCLUDE_DIRECTORIES
   "${source_dir}/googlemock/include"
   "${source_dir}/googletest/include")
   
SET(GMOCK_ALL_LIBRARIES 
   "${binary_dir}/googlemock/gtest/libgtest.a" 
   "${binary_dir}/googlemock/gtest/libgtest_main.a" 
   "${binary_dir}/googlemock/libgmock.a" 
   "${binary_dir}/googlemock/libgmock_main.a"
   pthread)

INCLUDE_DIRECTORIES(${SOURCE_ROOT})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

ADD_SUBDIRECTORY(Processor)
ADD_SUBDIRECTORY(Monad)
