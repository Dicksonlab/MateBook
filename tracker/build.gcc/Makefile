#MB_DIR=

INCLUDEDIRS = ${MB_DIR}/usr/include
LIBDIRS = ${MB_DIR}/usr/lib

APPNAME = tracker
LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale

all:
	g++ -DMATEBOOK_CLUSTER -std=c++98 -O3 -I ${INCLUDEDIRS} -L ${LIBDIRS} -Wl,-rpath=${LIBDIRS} ${LIBS} -o ${APPNAME} ../source/*.cpp ../../common/source/*.cpp ../../mediawrapper/source/*.cpp
