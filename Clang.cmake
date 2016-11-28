# Run cmake:
# cmake -DCMAKE_TOOLCHAIN_FILE=~/programming/Tools/Clang.cmake  ../

SET (CMAKE_C_COMPILER   "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER "/usr/bin/clang++")
SET (CMAKE_AR           "/usr/bin/llvm-ar")
SET (CMAKE_LINKER       "/usr/bin/llvm-ld")
SET (CMAKE_NM           "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP      "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB       "/usr/bin/llvm-ranlib")
