MB_DIR=/home/arthurb/projects/dickson/MateBook
#MB_DIR=/Users/arthurb/src/MateBook

APPNAME=tracker
INCLUDEDIRS=-I ${MB_DIR}/usr/include/
LIBDIRS=-L ${MB_DIR}/usr/lib/
LIBS=-lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale

all:
	LD_LIBRARY_PATH=${MB_DIR}/usr/lib:${LD_LIBRARY_PATH} g++ -DMATEBOOK_CLUSTER -std=c++98 -O3 ${INCLUDEDIRS} ${LIBDIRS} ${LIBS} -o ${MB_DIR}/usr/bin/${APPNAME} ../source/*.cpp ../../common/source/*.cpp ../../mediawrapper/source/*.cpp
