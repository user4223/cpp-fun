#!/bin/bash
DIR="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_DIR=$DIR/build
SOURCE_DIR=$DIR

mkdir -p $BUILD_DIR
pushd $BUILD_DIR
   cmake \
   -DCMAKE_BUILD_TYPE=Debug\
   -G"CodeLite - Unix Makefiles"\
   -DCMAKE_TOOLCHAIN_FILE=./Clang.cmake\
    $SOURCE_DIR 
popd

