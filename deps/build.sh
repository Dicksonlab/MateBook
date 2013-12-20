#!/bin/bash

#MB_DIR=/home/arthurb/projects/dickson/MateBook
MB_DIR=/Users/arthurb/src/MateBook
#export DYLD_LIBRARY_PATH=${MB_DIR}/usr/lib:${MB_DIR}/usr:${LD_LIBRARY_PATH}

LAME_VER=3.99.5
YASM_VER=1.2.0
FFMPEG_VER=2.1
CMAKE_VER=2.8.12.1
OPENCV_VER=2.4.7
BOOST_VER=1_54_0
ZLIB_VER=1.2.8


cd ${MB_DIR}/deps
curl -L http://sourceforge.net/projects/lame/files/lame/3.99/lame-${LAME_VER}.tar.gz/download > lame-${LAME_VER}.tar.gz
tar xvzf lame-${LAME_VER}.tar.gz
cd lame-${LAME_VER}
./configure --prefix=${MB_DIR}/usr/
make
make install

cd ${MB_DIR}/deps
curl -L http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VER}.tar.gz > yasm-${YASM_VER}.tar.gz
tar xvzf yasm-${YASM_VER}.tar.gz
cd yasm-${YASM_VER}
./configure --prefix=${MB_DIR}/usr/
make
make install

cd ${MB_DIR}/deps
curl -L http://zlib.net/zlib-${ZLIB_VER}.tar.gz > zlib-${ZLIB_VER}.tar.gz
tar xvzf zlib-${ZLIB_VER}.tar.gz
cd zlib-${ZLIB_VER}
./configure --prefix=${MB_DIR}/usr
make
make install

cd ${MB_DIR}/deps
curl -L http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VER}.tar.gz > ffmpeg-${FFMPEG_VER}.tar.gz
tar xvzf ffmpeg-${FFMPEG_VER}.tar.gz
cd ffmpeg-${FFMPEG_VER}
PATH=${MB_DIR}/usr/bin:$PATH
./configure --prefix=${MB_DIR}/usr/ --extra-cflags="-I${MB_DIR}/usr/include" --extra-ldflags="-L${MB_DIR}/usr/lib" --enable-libmp3lame --enable-gpl --enable-pthreads --arch=x86_64 --enable-ssse3 --disable-debug --enable-shared  # --cc=clang
make
make install

cd ${MB_DIR}/deps
curl -L http://www.cmake.org/files/v2.8/cmake-${CMAKE_VER}.tar.gz > cmake-${CMAKE_VER}.tar.gz
tar xvzf cmake-${CMAKE_VER}.tar.gz
cd cmake-${CMAKE_VER}
./bootstrap --prefix=${MB_DIR}/usr/
make
make install

cd ${MB_DIR}/deps
curl -L http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/${OPENCV_VER}/opencv-${OPENCV_VER}.tar.gz > opencv-${OPENCV_VER}.tar.gz
tar xvzf opencv-${OPENCV_VER}.tar.gz  # 2.4.7 has a bug with MD5 on linux
cd opencv-${OPENCV_VER}
export PKG_CONFIG_PATH=${MB_DIR}/usr/lib/pkgconfig:$PKG_CONFIG_PATH
export CMAKE_INCLUDE_PATH=${MB_DIR}/usr/include:$CMAKE_INCLUDE_PATH
export CMAKE_LIBRARY_PATH=${MB_DIR}/usr/lib:$CMAKE_LIBRARY_PATH
mkdir release
cd release
${MB_DIR}/usr/bin/cmake -D WITH_TIFF=OFF -D WITH_JASPER=OFF -D WITH_OPENEXR=OFF -D ZLIB_LIBRARY=${MB_DIR}/usr/lib/libz.so.${ZLIB_VER} -D CMAKE_INSTALL_PREFIX=${MB_DIR}/usr/ ..  #  -D BUILD_SHARED_LIBS=OFF -G "Xcode"
make
make install

cd ${MB_DIR}/deps
curl -L http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_${BOOST_VER}.tar.gz/download > boost_${BOOST_VER}.tar.gz
tar xvzf boost_${BOOST_VER}.tar.gz
cd boost_${BOOST_VER}
./bootstrap.sh --prefix=${MB_DIR}/usr/
./b2 --stagedir=${MB_DIR}/usr/
ln -s ${MB_DIR}/deps/boost_${BOOST_VER}/boost ${MB_DIR}/usr/include/boost
