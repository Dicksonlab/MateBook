#!/bin/bash

cat<<END_OF_HEAD>MateBook.pro
QT += core gui network xml opengl phonon
TEMPLATE = app
TARGET = MateBook
DEFINES += LINUX GL_GLEXT_PROTOTYPES
CONFIG += debug console
LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale -lGLU
INCLUDEPATH += ${MB_DIR}/usr/include
INCLUDEPATH += ${MB_DIR}/usr/include/QtCore
INCLUDEPATH += ${MB_DIR}/usr/include/QtGui
INCLUDEPATH += ${MB_DIR}/usr/include/QtOpenGL
END_OF_HEAD

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

QMAKESPEC=${MB_DIR}/usr/mkspecs/linux-g++ ${MB_DIR}/usr/bin/qmake
#qmake -spec macx-g++ MateBook.pro
