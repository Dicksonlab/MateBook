#MB_DIR=/Users/arthurb/src/MateBook
MB_DIR=/groups/dickson/dicksonlab/MateBook/MateBook
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
QT_VER=4.8.6
#GSTREAMER_VER=0.10.3

LIB_DIR=$(MB_DIR)/usr/lib
BIN_DIR=$(MB_DIR)/usr/bin
INCLUDE_DIR=$(MB_DIR)/usr/include

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
$(LIB_DIR)/libopencv_ts.a

YASM_LIBS = $(LIB_DIR)/libyasm.a

ZLIB = $(LIB_DIR)/libz.$(LIB_EXT)

QT_LIBS = \
$(LIB_DIR)/libQtCore.$(LIB_EXT) \
$(LIB_DIR)/libQtGui.$(LIB_EXT) \
$(LIB_DIR)/libQtOpenGL.$(LIB_EXT)

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
cleantracker :
	rm -rf $(MB_DIR)/tracker/build.gcc/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/*.sh
	rm -rf $(BIN_DIR)/tracker/${MB_VER}
clean : cleandeps cleangui cleantracker
	rm -rf $(MB_DIR)/gui/MateBook.app $(MB_DIR)/usr $(MB_DIR)/deps/*
.PHONY : cleancmake cleanffmpeg cleanboost cleanlame cleanopencv cleanyasm cleanzlib cleanqt cleandeps cleangui cleantracker cleanall

#$(GSTREAMER_LIB) :
#	mkdir -p ${MB_DIR}/deps
#	curl -L http://gstreamer.freedesktop.org/src/qt-gstreamer/qt-gstreamer-${GSTREAMER_VER}.tar.gz > ${MB_DIR}/deps/qt-gstreamer-0.10.3.tar.gz
#	cd ${MB_DIR}/deps && tar xvzf qt-gstreamer-${GSTREAMER_VER}.tar.gz

$(QT_LIBS) : ${MB_DIR}/deps/qt-${QT_VER}.tar.gz
${MB_DIR}/deps/qt-${QT_VER}.tar.gz :
	curl -L http://download.qt-project.org/official_releases/qt/4.8/${QT_VER}/qt-everywhere-opensource-src-${QT_VER}.tar.gz > ${MB_DIR}/deps/qt-${QT_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf qt-${QT_VER}.tar.gz
	cd ${MB_DIR}/deps/qt-everywhere-opensource-src-${QT_VER} && ./configure --prefix=${MB_DIR}/usr/ -opensource <<<yes
	cd ${MB_DIR}/deps/qt-everywhere-opensource-src-${QT_VER} && make && make install

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
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:${PATH} ./configure --prefix=${MB_DIR}/usr/ --extra-cflags="-I${MB_DIR}/usr/include" --extra-ldflags="-L${MB_DIR}/usr/lib" --enable-libmp3lame --enable-gpl --enable-pthreads --arch=x86_64 --enable-ssse3 --disable-debug --disable-static --enable-shared  # --cc=clang
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

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook : $(DEPS) $(QT_LIBS) $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && MB_DIR=${MB_DIR} ./update.sh && QTDIR=$(MB_DIR)/deps/qt-everywhere-opensource-src-${QT_VER} make
	
ifeq ($(OS), Darwin)
installgui : 
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

else
installgui : 
	cp $(MB_DIR)/gui/MateBook $(BIN_DIR)
endif

$(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker : $(DEPS) $(MB_DIR)/tracker/source/*.cpp $(MB_DIR)/tracker/source/*.hpp
	cd $(MB_DIR)/tracker/build.gcc && MB_DIR=${MB_DIR} make

ifeq ($(OS), Darwin)
installtracker : 
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

else
installtracker : 
	mkdir -p $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/build.gcc/tracker $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/source/*.sh $(BIN_DIR)/tracker/${MB_VER}
	chmod a+x $(BIN_DIR)/tracker/${MB_VER}/*
endif
.PHONY : installtracker
