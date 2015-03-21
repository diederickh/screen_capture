#!/bin/sh

my_build_dir=${PWD}

# Checkout the dependencies module
if [ ! -d ${my_build_dir}/dependencies ] ; then
    git clone https://github.com/roxlu/dependencies.git
fi

# Set environment variables
vs="2013"
source ./dependencies/build.sh

if [ ! -d ${my_build_dir}/build.release ] ; then
    mkdir ${my_build_dir}/build.release
fi

cd ${my_build_dir}
cd build.release

cmake \
    -DCMAKE_BUILD_TYPE=${cmake_build_type} \
    -DCMAKE_INSTALL_PREFIX=${install_path} \
    -DEXTERN_LIB_DIR=${extern_path}/lib \
    -DEXTERN_INC_DIR=${extern_path}/include \
    -DEXTERN_SRC_DIR=${extern_path}/src \
    -DTINYLIB_DIR=${my_build_dir}/sources/tinylib \
    ..

rc=$?; 
if [ $rc != 0 ]; then
    echo ""
    echo ""
    echo "CMAKE SETUP ERROR"
    echo ""
    echo ""
    exit
fi

cmake --build . --target install --config ${cmake_build_type}

rc=$?; 
if [ $rc != 0 ]; then
    echo ""
    echo ""
    echo "BUILD ERROR"
    echo ""
    echo ""
    exit
fi

cd ${install_path}/bin

#${debugger} ./test_mac_screencapture_console${debug_flag}
#${debugger} ./test_mac_api_research${debug_flag}
${debugger} ./test_opengl${debug_flag}
#${debugger} ./test_win_api_directx_research${debug_flag}
#${debugger} ./test_win_directx${debug_flag}
#${debugger} ./test_api${debug_flag}
#${debugger} ./test_win_api${debug_flag}

