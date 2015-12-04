#!/bin/bash

if [ "$1" == "Darwin" ] ; then
  OS=MACOS
  MKSPEC=macx-g++
  QMAKE=/usr/local/bin/qmake
  QTDIR=/usr/local/Cellar/qt/${QT_VER}
else
  OS=LINUX 
  MKSPEC=linux-g++
  QMAKE=${MB_DIR}/usr/bin/qmake
  QTDIR=${MB_DIR}/deps/qt-everywhere-opensource-src-${QT_VER}
fi
cat<<END_OF_HEAD>MateBook.pro
QT += core gui network xml opengl phonon
TEMPLATE = app
TARGET = MateBook
BIN_DIR = \"${BIN_DIR}\"
DEFINES += ${OS} GL_GLEXT_PROTOTYPES "'BIN_DIR=\$\${BIN_DIR}'"
CONFIG += debug console
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale
INCLUDEPATH += ${MB_DIR}/usr/include
QMAKE_LIBDIR += ${MB_DIR}/usr/lib
END_OF_HEAD

if [ "$1" == "Linux" ] ; then
cat<<END_OF_HEAD>>MateBook.pro
LIBS += -lGLU
QMAKE_RPATHDIR += ${MB_DIR}/usr/lib64
QMAKE_RPATHDIR += ${MB_DIR}/usr/lib/x86_64-linux-gnu
END_OF_HEAD
fi

echo "HEADERS += \\">>MateBook.pro
find source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../common/source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../common/source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../grapher/source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../grapher/source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../mediawrapper/source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../mediawrapper/source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
echo "../tracker/source/FrameAttributes.hpp \\">>MateBook.pro
echo "../tracker/source/FlyAttributes.hpp \\">>MateBook.pro
echo "../tracker/source/PairAttributes.hpp \\">>MateBook.pro
echo "../tracker/source/TrackedFrame.hpp \\">>MateBook.pro
echo "../tracker/source/Fly.hpp \\">>MateBook.pro
echo "../tracker/source/hungarian.hpp \\">>MateBook.pro
echo>>MateBook.pro

echo "SOURCES += \\">>MateBook.pro
find source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../common/source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../common/source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../grapher/source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../grapher/source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../mediawrapper/source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../mediawrapper/source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
echo "../tracker/source/FrameAttributes.cpp \\">>MateBook.pro
echo "../tracker/source/FlyAttributes.cpp \\">>MateBook.pro
echo "../tracker/source/PairAttributes.cpp \\">>MateBook.pro
echo "../tracker/source/TrackedFrame.cpp \\">>MateBook.pro
echo "../tracker/source/Fly.cpp \\">>MateBook.pro
echo "../tracker/source/hungarian.cpp \\">>MateBook.pro
echo>>MateBook.pro

cat<<END_OF_TAIL>>MateBook.pro
RESOURCES += qt/matebook.qrc
END_OF_TAIL

QMAKESPEC=${QTDIR}/mkspecs/${MKSPEC} ${QMAKE}
