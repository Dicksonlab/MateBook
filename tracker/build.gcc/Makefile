#MB_DIR=

INCLUDEDIRS = ${MB_DIR}/usr/include
LIBDIRS = ${MB_DIR}/usr/lib

ifeq ($(shell uname), Linux)
	LD_FLAGS=-Wl,-rpath=${LIBDIRS}
endif
ifeq ($(shell uname), Darwin)
	LD_FLAGS=-headerpad_max_install_names
endif

APPNAME = tracker
LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale

all:
	g++ -DMATEBOOK_CLUSTER -std=c++98 -O3 -I ${INCLUDEDIRS} -L ${LIBDIRS} ${LD_FLAGS} ${LIBS} -o ${APPNAME} ../source/*.cpp ../../common/source/*.cpp ../../mediawrapper/source/*.cpp
