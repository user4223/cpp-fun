#!/bin/bash
DIR="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_DIR=$DIR/build/CodeLite/Clang/Debug
SOURCE_DIR=$DIR

mkdir -p $BUILD_DIR
pushd $BUILD_DIR
   cmake \
   -DBIN_PATH_POSTFIX=Clang/Debug\
   -DCMAKE_BUILD_TYPE=Debug\
   -G"CodeLite - Unix Makefiles"\
   -DCMAKE_TOOLCHAIN_FILE=./Clang.cmake\
   -DBOOST_ROOT=$DIR/../lib/boost_1_65_0\
    $SOURCE_DIR 
popd

cmake --build $BUILD_DIR --config Debug
