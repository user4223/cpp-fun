#!/bin/bash
DIR="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_DIR=$DIR/build/gcc
SOURCE_DIR=$DIR

mkdir -p $BUILD_DIR
pushd $BUILD_DIR
   cmake \
   -DCMAKE_BUILD_TYPE=Debug\
   -G"CodeLite - Unix Makefiles"\
    $SOURCE_DIR 
popd

cmake --build $BUILD_DIR --config Debug
