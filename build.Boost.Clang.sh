#!/bin/bash
DIR="$( cd "$(dirname "$0")" ; pwd -P )"

pushd $DIR/../lib/boost/clang/boost_1_65_0
   ./bootstrap.sh --with-toolset=clang
   ./b2 clean
   ./b2	\
		toolset=clang\
		cxxflags="-std=c++14 -stdlib=libc++ -I/usr/include/libcxxabi"\
		linkflags="-stdlib=libc++"\
		define=BOOST_SYSTEM_NO_DEPRECATED\
		-j2\
 	stage release
popd
