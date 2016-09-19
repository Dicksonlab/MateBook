MB_DIR=$(abspath .)

MB_VER=2143

OS := $(shell uname)
SHELL := $(shell which bash)

ifeq ($(OS), Darwin)
	LIB_EXT=dylib
	GUI=$(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
	NJOBS=$(shell sysctl hw.ncpu | awk '{print $$2}')
else
	LIB_EXT=so
	GUI=$(MB_DIR)/gui/MateBook
	NJOBS=$(shell cat /proc/cpuinfo | grep processor | wc -l)
endif
TRACKER=$(MB_DIR)/tracker/build.gcc/tracker

LAME_VER=3.99.5
YASM_VER=1.2.0
FFMPEG_VER=2.1

LIB_DIR=${MB_DIR}/usr/lib
BIN_DIR=${MB_DIR}/usr/bin
INCLUDE_DIR=${MB_DIR}/usr/include
SHARE_DIR=${MB_DIR}/usr/share

FFMPEG_LIBS = \
$(LIB_DIR)/libavcodec.$(LIB_EXT) \
$(LIB_DIR)/libavdevice.$(LIB_EXT) \
$(LIB_DIR)/libavfilter.$(LIB_EXT) \
$(LIB_DIR)/libavformat.$(LIB_EXT) \
$(LIB_DIR)/libavutil.$(LIB_EXT) \
$(LIB_DIR)/libswresample.$(LIB_EXT) \
$(LIB_DIR)/libswscale.$(LIB_EXT)

LAME_LIBS = $(LIB_DIR)/libmp3lame.$(LIB_EXT)

YASM_LIBS = $(LIB_DIR)/libyasm.a

DEPS = $(FFMPEG_LIBS) $(LAME_LIBS) $(YASM_LIBS)

all : gui tracker
gui : $(GUI)
tracker : $(TRACKER)
install : installgui installtracker
.PHONY : all gui tracker install

$(FFMPEG_LIBS) : ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz
${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz : $(YASM_LIBS) $(LAME_LIBS)
	make cleanffmpeg
	curl -L http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VER}.tar.gz > ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf ffmpeg-${FFMPEG_VER}.tar.gz
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:${PATH} ./configure \
	--prefix=${MB_DIR}/usr/ \
	--extra-cflags="-I${MB_DIR}/usr/include" \
	--extra-ldflags="-L${MB_DIR}/usr/lib" \
	--enable-libmp3lame --enable-gpl --enable-pthreads --arch=x86_64 \
	--enable-ssse3 --disable-debug --enable-shared # --disable-static  --cc=clang
	cd ${MB_DIR}/deps/ffmpeg-${FFMPEG_VER} && PATH=${MB_DIR}/usr/bin:${PATH} make -j ${NJOBS} && make install

$(LAME_LIBS) : ${MB_DIR}/deps/lame-${LAME_VER}.tar.gz
${MB_DIR}/deps/lame-${LAME_VER}.tar.gz :
	make cleanlame
	curl -L http://sourceforge.net/projects/lame/files/lame/3.99/lame-${LAME_VER}.tar.gz/download > ${MB_DIR}/deps/lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf lame-${LAME_VER}.tar.gz
	cd ${MB_DIR}/deps/lame-${LAME_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/lame-${LAME_VER} && make -j ${NJOBS} && make install

$(YASM_LIBS) : ${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz
${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz :
	make cleanyasm
	curl -L http://www.tortall.net/projects/yasm/releases/yasm-${YASM_VER}.tar.gz > ${MB_DIR}/deps/yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps && tar xvzf yasm-${YASM_VER}.tar.gz
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && ./configure --prefix=${MB_DIR}/usr/
	cd ${MB_DIR}/deps/yasm-${YASM_VER} && make -j ${NJOBS} && make install

ifeq ($(OS), Darwin)
$(GUI) : $(DEPS) $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && MB_DIR=${MB_DIR} BIN_DIR=${BIN_DIR} ./update.sh $(OS) && make -j ${NJOBS}
	
installgui :
	rsync -r ${MB_DIR}/gui/MateBook.app ${BIN_DIR}
	LIB_DIR=${LIB_DIR} ./bundle_libs_in_app.sh ${BIN_DIR}/MateBook.app

else
$(GUI) : $(DEPS) $(MB_DIR)/gui/source/*.cpp $(MB_DIR)/gui/source/*.hpp
	cd gui && MB_DIR=${MB_DIR} BIN_DIR=${BIN_DIR} ./update.sh $(OS) && make -j ${NJOBS}
	
installgui :
	cp $(MB_DIR)/gui/MateBook $(BIN_DIR)
	chrpath -r '$${ORIGIN}/../lib' $(BIN_DIR)/MateBook  # why is this needed now?  yet not for tracker
endif
.PHONY : installgui

$(TRACKER) : $(DEPS) $(MB_DIR)/tracker/source/*.cpp $(MB_DIR)/tracker/source/*.hpp
	cd $(MB_DIR)/tracker/build.gcc && MB_DIR=${MB_DIR} make -j ${NJOBS}

ifeq ($(OS), Darwin)
installtracker : 
	mkdir -p $(BIN_DIR)/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/build.gcc/tracker $(BIN_DIR)/MateBook.app/Contents/MacOS
	cp $(MB_DIR)/tracker/source/*.sh $(BIN_DIR)/MateBook.app/Contents/MacOS
	LIB_DIR=${LIB_DIR} ./bundle_libs_in_app.sh $(BIN_DIR)/MateBook.app

else
installtracker : 
	mkdir -p $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/build.gcc/tracker $(BIN_DIR)/tracker/${MB_VER}
	cp $(MB_DIR)/tracker/source/*.sh $(BIN_DIR)/tracker/${MB_VER}
	chmod a+x $(BIN_DIR)/tracker/${MB_VER}/*
endif
.PHONY : installtracker

cleanyasm :
	rm -rf $(MB_DIR)/deps/yasm-*
	rm -rf $(LIB_DIR)/libyasm*
	rm -rf $(BIN_DIR)/*asm
	rm -rf $(INCLUDE_DIR)/libyasm*
cleanlame :
	rm -rf $(MB_DIR)/deps/lame-*
	rm -rf $(LIB_DIR)/libmp3lame*
	rm -rf $(BIN_DIR)/lame
	rm -rf $(INCLUDE_DIR)/lame
cleanffmpeg :
	rm -rf $(MB_DIR)/deps/ffmpeg-*
	rm -rf $(LIB_DIR)/libav*
	rm -rf $(LIB_DIR)/libsw*
	rm -rf $(BIN_DIR)/ff*
	rm -rf $(INCLUDE_DIR)/libav*
	rm -rf $(INCLUDE_DIR)/libsw*
	rm -rf $(SHARE_DIR)/ffmpeg

cleandeps : cleanffmpeg cleanlame cleanyasm

cleangui : cleangui2
	rm -rf $(MB_DIR)/gui/*.o
	rm -rf $(MB_DIR)/gui/moc_*
ifeq ($(OS), Darwin)
cleangui2 :
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/MateBook
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/Frameworks
	rm -rf $(BIN_DIR)/MateBook.app/Contents/MacOS/MateBook
	rm -rf $(BIN_DIR)/MateBook.app/Contents/Frameworks
else
cleangui2 :
	rm -rf $(MB_DIR)/gui/MateBook
endif

cleantracker : cleantracker2
	rm -rf $(MB_DIR)/tracker/build.gcc/tracker
ifeq ($(OS), Darwin)
cleantracker2 :
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/tracker
	rm -rf $(MB_DIR)/gui/MateBook.app/Contents/MacOS/*.sh
	rm -rf $(BIN_DIR)/MateBook.app/Contents/MacOS/tracker
	rm -rf $(BIN_DIR)/MateBook.app/Contents/MacOS/*.sh
else
cleantracker2 :
	rm -rf $(BIN_DIR)/tracker/${MB_VER}
endif

clean : cleandeps cleangui cleantracker
	rm -rf $(MB_DIR)/gui/MateBook.app
	rm -rf $(MB_DIR)/usr
	rm -rf $(MB_DIR)/deps/*
.PHONY : cleanffmpeg cleanlame cleanyasm cleandeps cleangui cleantracker cleanall
