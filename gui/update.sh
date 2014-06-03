#!/bin/bash

if [ "$1" == "Darwin" ] ; then
  OS=MACOS
  QTDIR=/usr/local/Qt4.8
else
  OS=LINUX 
  QTDIR=/usr/lib64/qt4
fi

cat<<END_OF_HEAD>MateBook.pro
QT += core gui network xml opengl phonon
TEMPLATE = app
TARGET = MateBook
DEFINES += ${OS} GL_GLEXT_PROTOTYPES
CONFIG += debug console
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale -lGLU
INCLUDEPATH += ${MB_DIR}/usr/include
LIBPATH += ${MB_DIR}/usr/lib
END_OF_HEAD

if [ "$1" == "Linux" ] ; then
cat<<END_OF_HEAD>>MateBook.pro
INCLUDEPATH += /usr/include/QtCore
INCLUDEPATH += /usr/include/QtGui
INCLUDEPATH += /usr/include/QtOpenGL
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

if [ "$1" == "Darwin" ] ; then
  QMAKESPEC=${QTDIR}/mkspecs/macx-g++ ${QTDIR}/bin/qmake
else
  QMAKESPEC=${QTDIR}/mkspecs/linux-g++ ${QTDIR}/bin/qmake
fi
