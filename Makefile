MB_DIR=/Users/arthurb/src/MateBook

LAME_VER=3.99.5
YASM_VER=1.2.0
FFMPEG_VER=2.1
CMAKE_VER=2.8.12.1
OPENCV_VER=2.4.7# 2.4.7 has a bug with MD5 on linux
BOOST_VER=1_54_0
ZLIB_VER=1.2.8

LIB_DIR=$(MB_DIR)/usr/lib
BIN_DIR=$(MB_DIR)/usr/bin
INCLUDE_DIR=$(MB_DIR)/usr/include

CMAKE_BIN = $(BIN_DIR)/cmake

FFMPEG_LIBS = \
$(LIB_DIR)/libavcodec.dylib \
$(LIB_DIR)/libavdevice.dylib \
$(LIB_DIR)/libavfilter.dylib \
$(LIB_DIR)/libavformat.dylib \
$(LIB_DIR)/libavutil.dylib \
$(LIB_DIR)/libswresample.dylib \
$(LIB_DIR)/libswscale.dylib

BOOST_LIBS = \
$(LIB_DIR)/libboost_filesystem.dylib \
$(LIB_DIR)/libboost_system.dylib

LAME_LIBS = $(LIB_DIR)/libmp3lame.dylib

OPENCV_LIBS = \
$(LIB_DIR)/libopencv_calib3d.dylib \
$(LIB_DIR)/libopencv_contrib.dylib \
$(LIB_DIR)/libopencv_core.dylib \
$(LIB_DIR)/libopencv_features2d.dylib \
$(LIB_DIR)/libopencv_flann.dylib \
$(LIB_DIR)/libopencv_gpu.dylib \
$(LIB_DIR)/libopencv_highgui.dylib \
$(LIB_DIR)/libopencv_imgproc.dylib \
$(LIB_DIR)/libopencv_legacy.dylib \
$(LIB_DIR)/libopencv_ml.dylib \
$(LIB_DIR)/libopencv_nonfree.dylib \
$(LIB_DIR)/libopencv_objdetect.dylib \
$(LIB_DIR)/libopencv_ocl.dylib \
$(LIB_DIR)/libopencv_photo.dylib \
$(LIB_DIR)/libopencv_stitching.dylib \
$(LIB_DIR)/libopencv_superres.dylib \
$(LIB_DIR)/libopencv_video.dylib \
$(LIB_DIR)/libopencv_videostab.dylib \
$(LIB_DIR)/libopencv_ts.a

YASM_LIBS = $(LIB_DIR)/libyasm.a

ZLIB = $(LIB_DIR)/libz.dylib

DEPS = $(CMAKE_BIN) $(FFMPEG_LIBS) $(BOOST_LIBS) $(LAME_LIBS) $(OPENCV_LIBS) $(YASM_LIBS) $(ZLIB)

all : gui tracker
gui : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
tracker : $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
.PHONY : all gui tracker

cleanzlib :
	rm -rf $(MB_DIR)/deps/zlib-* $(LIB_DIR)/libz* $(INCLUDE_DIR)/zlib.h
cleanyasm :
	rm -rf $(MB_DIR)/deps/yasm-* $(LIB_DIR)/libyasm* $(BIN_DIR)/*asm $(INCLUDE_DIR)/libyasm*
cleanopencv :
	rm -rf $(MB_DIR)/deps/opencv-* $(LIB_DIR)/libopencv* $(BIN_DIR)/opencv* $(INCLUDE_DIR)/opencv*
cleanlame :
	rm -rf $(MB_DIR)/deps/lame-* $(LIB_DIR)/libmp3lame* $(BIN_DIR)/lame $(INCLUDE_DIR)/lame
cleanboost :
	rm -rf $(MB_DIR)/deps/boost-* $(LIB_DIR)/libboost* $(INCLUDE_DIR)/boost
cleanffmpeg :
	rm -rf $(MB_DIR)/deps/ffmpeg-* $(LIB_DIR)/libav* $(LIB_DIR)/libsw* $(BIN_DIR)/ff* $(INCLUDE_DIR)/libav* $(INCLUDE_DIR)/libsw*
cleancmake :
	rm -rf $(MB_DIR)/deps/cmake-* $(BIN_DIR)/c*
cleandeps : cleancmake cleanffmpeg cleanboost cleanlame cleanopencv cleanyasm cleanzlib
cleangui :
	rm -rf $(MB_DIR)/gui/*.o
	rm -rf $(MB_DIR)/gui/moc_*
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
cleantracker :
	rm -rf $(MB_DIR)/tracker/build.gcc/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/*.sh
cleanall : cleandeps cleangui cleantracker
	rm -rf $(MB_DIR)/gui/MateBook.app
.PHONY : cleancmake cleanffmpeg cleanboost cleanlame cleanopencv cleanyasm cleanzlib cleandeps cleangui cleantracker cleanall

$(CMAKE_BIN) :
	curl -L http://www.cmake.org/files/v2.8/cmake-${CMAKE_VER}.tar.gz > ${MB_DIR}/deps/cmake-${CMAKE_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf cmake-${CMAKE_VER}.tar.gz
	cd ${MB_DIR}/deps/cmake-${CMAKE_VER} && ./bootstrap --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/cmake-${CMAKE_VER} && make && make install

$(FFMPEG_LIBS) :
	curl -L http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VER}.tar.gz > ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf ffmpeg-${FFMPEG_VER}.tar.gz
	cd ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:$PATH ./configure --prefix=${MB_DIR}/usr/ --extra-cflags="-I${MB_DIR}/usr/include" --extra-ldflags="-L${MB_DIR}/usr/lib" --enable-libmp3lame --enable-gpl --enable-pthreads --arch=x86_64 --enable-ssse3 --disable-debug --disable-static --enable-shared  # --cc=clang
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && make && make install

$(BOOST_LIBS) :
	curl -L http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_${BOOST_VER}.tar.gz/download > ${MB_DIR}/deps/boost_${BOOST_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf boost_${BOOST_VER}.tar.gz
	cd ${MB_DIR}/deps/boost_${BOOST_VER} && ./bootstrap.sh --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/boost_${BOOST_VER} && ./b2 --stagedir=${MB_DIR}/usr/
	ln -s ${MB_DIR}/deps/boost_${BOOST_VER}/boost ${MB_DIR}/usr/include/boost

$(LAME_LIBS) :
	curl -L http://sourceforge.net/projects/lame/files/lame/3.99/lame-${LAME_VER}.tar.gz/download > ${MB_DIR}/deps/lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps/lame-${LAME_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/lame-${LAME_VER} && make && make install

$(OPENCV_LIBS) :
	curl -L http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/${OPENCV_VER}/opencv-${OPENCV_VER}.tar.gz > ${MB_DIR}/deps/opencv-${OPENCV_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf opencv-${OPENCV_VER}.tar.gz
	# edit release/CMakeCache.txt, set VERBOSE=ON/TRUE
	sed s+\$${MB_DIR}+${MB_DIR}+ < patch_opencv.diff | patch ${MB_DIR}/deps/opencv-${OPENCV_VER}/modules/highgui/CMakeLists.txt
	mkdir ${MB_DIR}/deps/opencv-${OPENCV_VER}/release
	cd ${MB_DIR}/deps/opencv-${OPENCV_VER}/release && PKG_CONFIG_PATH=${MB_DIR}/usr/lib/pkgconfig CMAKE_INCLUDE_PATH=${MB_DIR}/usr/include CMAKE_LIBRARY_PATH=${MB_DIR}/usr/lib ${MB_DIR}/usr/bin/cmake -D WITH_TIFF=OFF -D WITH_JASPER=OFF -D WITH_OPENEXR=OFF -D ZLIB_LIBRARY=${MB_DIR}/usr/lib/libz.so.${ZLIB_VER} -D CMAKE_INSTALL_PREFIX=${MB_DIR}/usr/ ..  #  -D BUILD_SHARED_LIBS=OFF -G "Xcode"
	cd ${MB_DIR}/deps/opencv-${OPENCV_VER}/release && make && make install

$(YASM_LIBS) :
	curl -L http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VER}.tar.gz > ${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && make && make install

$(ZLIB) :
	curl -L http://zlib.net/zlib-${ZLIB_VER}.tar.gz > ${MB_DIR}/deps/zlib-${ZLIB_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf zlib-${ZLIB_VER}.tar.gz
	cd ${MB_DIR}/deps/zlib-${ZLIB_VER} && ./configure --prefix=${MB_DIR}/usr
	cd ${MB_DIR}/deps/zlib-${ZLIB_VER} && make && make install

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook : $(DEPS) $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && ./update.sh && make
	
	mkdir -p $(MB_DIR)/gui/Matebook.app/Contents/Frameworks
	
	dylibs=( \
	  libmp3lame.0.dylib \
	  libpostproc.52.dylib \
	  libopencv_core.2.4.dylib \
	  libopencv_highgui.2.4.dylib \
	  libopencv_imgproc.2.4.dylib \
	  libboost_system.dylib \
	  libboost_filesystem.dylib \
	  libavdevice.55.dylib \
	  libavfilter.3.dylib \
	  libavformat.55.dylib \
	  libavutil.52.dylib \
	  libavcodec.55.dylib \
	  libswresample.0.dylib \
	  libswscale.2.dylib) && \
	for x in $${dylibs[*]}; do \
	  cp -RL $(LIB_DIR)/$$x $(MB_DIR)/gui/Matebook.app/Contents/Frameworks ; \
	done
	chmod -R u+w $(MB_DIR)/gui/Matebook.app/Contents/Frameworks
	
	dylibs=( \
	  lib/libopencv_core.2.4.dylib \
	  lib/libopencv_highgui.2.4.dylib \
	  lib/libopencv_imgproc.2.4.dylib \
	  libboost_system.dylib \
	  libboost_filesystem.dylib \
	  $(LIB_DIR)/libavdevice.55.dylib \
	  $(LIB_DIR)/libavfilter.3.dylib \
	  $(LIB_DIR)/libavformat.55.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libswresample.0.dylib \
	  $(LIB_DIR)/libswscale.2.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -id @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/$$(basename $$x) ; \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook ; \
	done
	install_name_tool -id @executable_path/../Frameworks/libmp3lame.0.dylib \
      $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libmp3lame.0.dylib
	install_name_tool -id @executable_path/../Frameworks/libpostproc.52.dylib \
      $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libpostproc.52.dylib
	
	dylibs=( \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libpostproc.52.dylib ; \
	done
	
	dylibs=( \
	  lib/libopencv_core.2.4.dylib \
	  lib/libopencv_imgproc.2.4.dylib \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libavformat.55.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libswscale.2.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libopencv_highgui.2.4.dylib ; \
	done
	
	install_name_tool -change lib/libopencv_core.2.4.dylib @executable_path/../Frameworks/libopencv_core.2.4.dylib \
      $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libopencv_imgproc.2.4.dylib
	
	install_name_tool -change libboost_system.dylib @executable_path/../Frameworks/libboost_system.dylib \
      $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libboost_filesystem.dylib
	
	dylibs=( \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libavcodec.55.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libavfilter.3.dylib \
	  $(LIB_DIR)/libavformat.55.dylib \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libavdevice.55.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libswresample.0.dylib \
	  $(LIB_DIR)/libavformat.55.dylib \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libpostproc.52.dylib \
	  $(LIB_DIR)/libswscale.2.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libavfilter.3.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libavformat.55.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libavutil.52.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libswresample.0.dylib ; \
	done
	
	dylibs=( \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libmp3lame.0.dylib \
	  $(LIB_DIR)/libz.1.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) \
        $(MB_DIR)/gui/MateBook.app/Contents/Frameworks/libswscale.2.dylib ; \
	done

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker : $(DEPS) $(MB_DIR)/tracker/source/*.cpp $(MB_DIR)/tracker/source/*.hpp
	cd $(MB_DIR)/tracker/build.gcc && make
	# bindir=usr/bin/tracker/2141 # on the cluster
	# bindir=$(MB_DIR)/gui/MateBook.app/Contents/MacOS/ # on a mac
	mkdir -p $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/build.gcc/tracker $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/source/*.sh $(MB_DIR)/gui/MateBook.app/Contents/MacOS
	
	dylibs=( \
	  lib/libopencv_core.2.4.dylib \
	  lib/libopencv_highgui.2.4.dylib \
	  lib/libopencv_imgproc.2.4.dylib \
	  libboost_system.dylib \
	  libboost_filesystem.dylib \
	  $(LIB_DIR)/libavdevice.55.dylib \
	  $(LIB_DIR)/libavfilter.3.dylib \
	  $(LIB_DIR)/libavformat.55.dylib \
	  $(LIB_DIR)/libavutil.52.dylib \
	  $(LIB_DIR)/libavcodec.55.dylib \
	  $(LIB_DIR)/libswresample.0.dylib \
	  $(LIB_DIR)/libswscale.2.dylib) && \
	for x in $${dylibs[*]}; do \
	  install_name_tool -change $$x @executable_path/../Frameworks/$$(basename $$x) $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker ; \
	done
