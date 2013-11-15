REVISIONSCRIPT= svn info | grep -E "^Revision: [0-9]+" | sed "s/Revision: //"
REVISION=$(shell $(REVISIONSCRIPT))
SYSTEM=$(shell uname -s)

APPNAME=tracker
INCLUDEDIRS=-I /biosw/debian5-x86_64/ffmpeg/0.11.1/include/ -I /biosw/debian5-x86_64/opencv/2.4.1/include/ -I /sw/lenny/arch/x86_64/boost-1.47.0/include/
LIBDIRS=-L /biosw/debian5-x86_64/ffmpeg/0.11.1/lib/ -L /biosw/debian5-x86_64/opencv/2.4.1/lib/ -L /sw/lenny/arch/x86_64/boost-1.47.0/lib/
LIBS=-l opencv_core -l opencv_highgui -l opencv_imgproc -l boost_system -l boost_filesystem

all: clean run 

echoRevision:
	@echo $(REVISION)

compile: echoRevision
	mkdir -p ../binaries/Linux/Release
	LD_LIBRARY_PATH=/biosw/debian5-x86_64/ffmpeg/0.11.1/lib/:/sw/lenny/arch/x86_64/boost-1.47.0/lib/:${LD_LIBRARY_PATH} g++ -DMATEBOOK_CLUSTER -std=c++0x -O3 ${INCLUDEDIRS} ${LIBDIRS} ${LIBS} -o ../binaries/Linux/Release/${APPNAME} ../source/*.cpp ../../common/source/*.cpp ../../mediawrapper/source/*.cpp

run: compile
	../binaries/Linux/Release/${APPNAME}

installMoonWalk: compile
	mkdir -p ~/moonwalk/tracker/${REVISION}
	cp -P ../../mediawrapper/binaries/Linux/Release/* ~/moonwalk/tracker/${REVISION}/
	cp ../binaries/Linux/Release/${APPNAME} ~/moonwalk/tracker/${REVISION}/
	ln -sfn ${REVISION} ~/moonwalk/tracker/current

installMateBook: compile
	mkdir -p ~/MateBook/tracker/${REVISION}
	cp ../binaries/Linux/Release/${APPNAME} ~/MateBook/tracker/${REVISION}/
	ln -sfn ${REVISION} ~/MateBook/tracker/current

clean:
	rm ../binaries/Linux/Release/*
