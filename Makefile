MB_DIR=/Users/arthurb/src/MateBook
#MB_DIR=/groups/dickson/dicksonlab/MateBook/MateBook
#MB_DIR=/home/arthurb/src/MateBook

MB_VER=2141

OS := $(shell uname)

ifeq ($(OS), Darwin)
	LIB_EXT=dylib
else
	LIB_EXT=so
endif

LAME_VER=3.99.5
YASM_VER=1.2.0
FFMPEG_VER=2.1
CMAKE_VER=2.8.12.1
OPENCV_VER=2.4.7# 2.4.7 has a bug with MD5 on linux
BOOST_VER=1_54_0
ZLIB_VER=1.2.8

LIB_DIR=${MB_DIR}/usr/lib
BIN_DIR=${MB_DIR}/usr/bin
INCLUDE_DIR=${MB_DIR}/usr/include

CMAKE_BIN = $(BIN_DIR)/cmake

FFMPEG_LIBS = \
$(LIB_DIR)/libavcodec.$(LIB_EXT) \
$(LIB_DIR)/libavdevice.$(LIB_EXT) \
$(LIB_DIR)/libavfilter.$(LIB_EXT) \
$(LIB_DIR)/libavformat.$(LIB_EXT) \
$(LIB_DIR)/libavutil.$(LIB_EXT) \
$(LIB_DIR)/libswresample.$(LIB_EXT) \
$(LIB_DIR)/libswscale.$(LIB_EXT)

BOOST_LIBS = \
$(LIB_DIR)/libboost_filesystem.$(LIB_EXT) \
$(LIB_DIR)/libboost_system.$(LIB_EXT)

LAME_LIBS = $(LIB_DIR)/libmp3lame.$(LIB_EXT)

OPENCV_LIBS = \
$(LIB_DIR)/libopencv_calib3d.$(LIB_EXT) \
$(LIB_DIR)/libopencv_contrib.$(LIB_EXT) \
$(LIB_DIR)/libopencv_core.$(LIB_EXT) \
$(LIB_DIR)/libopencv_features2d.$(LIB_EXT) \
$(LIB_DIR)/libopencv_flann.$(LIB_EXT) \
$(LIB_DIR)/libopencv_gpu.$(LIB_EXT) \
$(LIB_DIR)/libopencv_highgui.$(LIB_EXT) \
$(LIB_DIR)/libopencv_imgproc.$(LIB_EXT) \
$(LIB_DIR)/libopencv_legacy.$(LIB_EXT) \
$(LIB_DIR)/libopencv_ml.$(LIB_EXT) \
$(LIB_DIR)/libopencv_nonfree.$(LIB_EXT) \
$(LIB_DIR)/libopencv_objdetect.$(LIB_EXT) \
$(LIB_DIR)/libopencv_ocl.$(LIB_EXT) \
$(LIB_DIR)/libopencv_photo.$(LIB_EXT) \
$(LIB_DIR)/libopencv_stitching.$(LIB_EXT) \
$(LIB_DIR)/libopencv_superres.$(LIB_EXT) \
$(LIB_DIR)/libopencv_video.$(LIB_EXT) \
$(LIB_DIR)/libopencv_videostab.$(LIB_EXT) \
$(LIB_DIR)/libopencv_ts.$(LIB_EXT)

YASM_LIBS = $(LIB_DIR)/libyasm.a

ZLIB = $(LIB_DIR)/libz.$(LIB_EXT)

DEPS = $(CMAKE_BIN) $(FFMPEG_LIBS) $(BOOST_LIBS) $(LAME_LIBS) $(OPENCV_LIBS) $(YASM_LIBS) $(ZLIB)

all : gui tracker
gui : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
tracker : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
install : installgui installtracker
.PHONY : all gui tracker install

cleanzlib :
	rm -rf $(MB_DIR)/deps/zlib-* $(LIB_DIR)/libz* $(INCLUDE_DIR)/zlib.h
cleanyasm :
	rm -rf $(MB_DIR)/deps/yasm-* $(LIB_DIR)/libyasm* $(BIN_DIR)/*asm $(INCLUDE_DIR)/libyasm*
cleanopencv :
	rm -rf $(MB_DIR)/deps/opencv-* $(LIB_DIR)/libopencv* $(BIN_DIR)/opencv* $(INCLUDE_DIR)/opencv*
cleanlame :
	rm -rf $(MB_DIR)/deps/lame-* $(LIB_DIR)/libmp3lame* $(BIN_DIR)/lame $(INCLUDE_DIR)/lame
cleanboost :
	rm -rf $(MB_DIR)/deps/boost_* $(LIB_DIR)/libboost* $(INCLUDE_DIR)/boost
cleanffmpeg :
	rm -rf $(MB_DIR)/deps/ffmpeg-* $(LIB_DIR)/libav* $(LIB_DIR)/libsw* $(BIN_DIR)/ff* $(INCLUDE_DIR)/libav* $(INCLUDE_DIR)/libsw*
cleancmake :
	rm -rf $(MB_DIR)/deps/cmake-* $(BIN_DIR)/c*
cleanqt :
	rm -rf $(MB_DIR)/deps/qt-* $(LIB_DIR)/libQt* $(INCLUDE_DIR)/Qt*
cleandeps : cleancmake cleanffmpeg cleanboost cleanlame cleanopencv cleanyasm cleanzlib cleanqt
cleangui :
	rm -rf $(MB_DIR)/gui/*.o $(MB_DIR)/gui/moc_*
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/Frameworks
cleantracker :
	rm -rf $(MB_DIR)/tracker/build.gcc/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/*.sh
	rm -rf $(BIN_DIR)/tracker/${MB_VER}
clean : cleandeps cleangui cleantracker
	rm -rf $(MB_DIR)/gui/MateBook.app $(MB_DIR)/usr $(MB_DIR)/deps/*
.PHONY : cleancmake cleanffmpeg cleanboost cleanlame cleanopencv cleanyasm cleanzlib cleanqt cleandeps cleangui cleantracker cleanall

$(CMAKE_BIN) : ${MB_DIR}/deps/cmake-${CMAKE_VER}.tar.gz
${MB_DIR}/deps/cmake-${CMAKE_VER}.tar.gz :
	curl -L http://www.cmake.org/files/v2.8/cmake-${CMAKE_VER}.tar.gz > ${MB_DIR}/deps/cmake-${CMAKE_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf cmake-${CMAKE_VER}.tar.gz
	cd ${MB_DIR}/deps/cmake-${CMAKE_VER} && ./bootstrap --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/cmake-${CMAKE_VER} && make && make install

$(FFMPEG_LIBS) : ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz
${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz : $(YASM_LIBS) $(LAME_LIBS)
	curl -L http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VER}.tar.gz > ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf ffmpeg-${FFMPEG_VER}.tar.gz
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:${PATH} ./configure --prefix=${MB_DIR}/usr/ --extra-cflags="-I${MB_DIR}/usr/include" --extra-ldflags="-L${MB_DIR}/usr/lib" --enable-libmp3lame --enable-gpl --enable-pthreads --arch=x86_64 --enable-ssse3 --disable-debug --enable-shared # --disable-static  --cc=clang
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:${PATH} make && make install

$(BOOST_LIBS) : ${MB_DIR}/deps/boost_${BOOST_VER}.tar.gz
${MB_DIR}/deps/boost_${BOOST_VER}.tar.gz :
	curl -L http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_${BOOST_VER}.tar.gz/download > ${MB_DIR}/deps/boost_${BOOST_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf boost_${BOOST_VER}.tar.gz
	cd ${MB_DIR}/deps/boost_${BOOST_VER} && ./bootstrap.sh --prefix=${MB_DIR}/usr/ --with-libraries=filesystem,system
	cd ${MB_DIR}/deps/boost_${BOOST_VER} && ./b2 --stagedir=${MB_DIR}/usr/
	ln -s ${MB_DIR}/deps/boost_${BOOST_VER}/boost ${MB_DIR}/usr/include/boost

$(LAME_LIBS) : ${MB_DIR}/deps/lame-${LAME_VER}.tar.gz
${MB_DIR}/deps/lame-${LAME_VER}.tar.gz :
	curl -L http://sourceforge.net/projects/lame/files/lame/3.99/lame-${LAME_VER}.tar.gz/download > ${MB_DIR}/deps/lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps/lame-${LAME_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/lame-${LAME_VER} && make && make install

$(OPENCV_LIBS) : ${MB_DIR}/deps/opencv-${OPENCV_VER}.tar.gz
${MB_DIR}/deps/opencv-${OPENCV_VER}.tar.gz : $(CMAKE_BIN) $(ZLIB)
	curl -L http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/${OPENCV_VER}/opencv-${OPENCV_VER}.tar.gz > ${MB_DIR}/deps/opencv-${OPENCV_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf opencv-${OPENCV_VER}.tar.gz
	# edit release/CMakeCache.txt, set VERBOSE=ON/TRUE
	sed s+\$${MB_DIR}+${MB_DIR}+ < patch_opencv.diff | patch ${MB_DIR}/deps/opencv-${OPENCV_VER}/modules/highgui/CMakeLists.txt
	mkdir ${MB_DIR}/deps/opencv-${OPENCV_VER}/release
	cd ${MB_DIR}/deps/opencv-${OPENCV_VER}/release && PKG_CONFIG_PATH=${MB_DIR}/usr/lib/pkgconfig CMAKE_INCLUDE_PATH=${MB_DIR}/usr/include CMAKE_LIBRARY_PATH=${MB_DIR}/usr/lib ${MB_DIR}/usr/bin/cmake -D WITH_TIFF=OFF -D WITH_JASPER=OFF -D WITH_OPENEXR=OFF -D ZLIB_LIBRARY=${MB_DIR}/usr/lib/libz.so.${ZLIB_VER} -D CMAKE_INSTALL_PREFIX=${MB_DIR}/usr/ ..  #  -D BUILD_SHARED_LIBS=OFF -G "Xcode"
	cd ${MB_DIR}/deps/opencv-${OPENCV_VER}/release && make && make install

$(YASM_LIBS) : ${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz
${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz :
	curl -L http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VER}.tar.gz > ${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && make && make install

$(ZLIB) : ${MB_DIR}/deps/zlib-${ZLIB_VER}.tar.gz
${MB_DIR}/deps/zlib-${ZLIB_VER}.tar.gz :
	curl -L http://zlib.net/zlib-${ZLIB_VER}.tar.gz > ${MB_DIR}/deps/zlib-${ZLIB_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf zlib-${ZLIB_VER}.tar.gz
	cd ${MB_DIR}/deps/zlib-${ZLIB_VER} && ./configure --prefix=${MB_DIR}/usr
	cd ${MB_DIR}/deps/zlib-${ZLIB_VER} && make && make install

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook : $(DEPS) $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && MB_DIR=${MB_DIR} ./update.sh $(OS) && make
	
ifeq ($(OS), Darwin)
installgui :
	LIB_DIR=${LIB_DIR} ./bundle_libs_in_app.sh ${MB_DIR}/gui/Matebook.app
	cp -R ${MB_DIR}/gui/Matebook.app ${BIN_DIR}

else
installgui :
	cp $(MB_DIR)/gui/MateBook $(BIN_DIR)
endif
.PHONY : installgui

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker : $(DEPS) $(MB_DIR)/tracker/source/*.cpp $(MB_DIR)/tracker/source/*.hpp
	cd $(MB_DIR)/tracker/build.gcc && MB_DIR=${MB_DIR} make

ifeq ($(OS), Darwin)
installtracker : 
	mkdir -p $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/build.gcc/tracker $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/source/*.sh $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	LIB_DIR=${LIB_DIR} ./bundle_libs_in_app.sh gui/Matebook.app

else
installtracker : 
	mkdir -p $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/build.gcc/tracker $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/source/*.sh $(BIN_DIR)/tracker/${MB_VER}
	chmod a+x $(BIN_DIR)/tracker/${MB_VER}/*
endif
.PHONY : installtracker
