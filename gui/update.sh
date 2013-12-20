#!/bin/bash
#set -u
#set -e
#
#rm -rf MateBook.xcodeproj.old
#mv MateBook.xcodeproj MateBook.xcodeproj.old

MB_DIR=/Users/arthurb/src/MateBook

cat<<END_OF_HEAD>MateBook.pro
QT += core gui network xml opengl phonon
TEMPLATE = app
TARGET = MateBook
CONFIG += debug console
#LIBS += -L"../../mediawrapper/binaries/OSX/Debug" -lmediawrapper
LIBS += -L${MB_DIR}/usr/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lboost_filesystem -lavdevice -lavfilter -lavformat -lavutil -lavcodec -lswresample -lswscale
#LIBS += ${MB_DIR}/usr/lib/libopencv_core.dylib
#LIBS += ${MB_DIR}/usr/lib/libopencv_highgui.dylib
#LIBS += ${MB_DIR}/usr/lib/libopencv_imgproc.dylib
#LIBS += ${MB_DIR}/usr/lib/libboost_system.dylib
#LIBS += ${MB_DIR}/usr/lib/libboost_filesystem.dylib
#LIBS += ${MB_DIR}/usr/lib/libswresample.dylib
#LIBS += ${MB_DIR}/usr/lib/libswscale.dylib
#LIBS += ${MB_DIR}/usr/lib/libmp3lame.dylib
#LIBS += ${MB_DIR}/usr/lib/libz.dylib
#LIBS += ${MB_DIR}/usr/lib/libavdevice.dylib
#LIBS += ${MB_DIR}/usr/lib/libavfilter.dylib
#LIBS += ${MB_DIR}/usr/lib/libavformat.dylib
#LIBS += ${MB_DIR}/usr/lib/libavutil.dylib
#LIBS += ${MB_DIR}/usr/lib/libavcodec.dylib
INCLUDEPATH = ${MB_DIR}/usr/include
#ICON = icon.icns
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
#echo "../modeltest/source/modeltest.h \\">>MateBook.pro
#echo "../modeltest/source/dynamictreemodel.h \\">>MateBook.pro
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
#echo "../../modeltest/source/modeltest.cpp \\">>MateBook.pro
#echo "../../modeltest/source/dynamictreemodel.cpp \\">>MateBook.pro
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

#qmake -spec macx-xcode MateBook.pro
qmake -spec macx-g++ MateBook.pro

#cp MateBook.xcodeproj/qt_makeqmake.mak MateBook.xcodeproj.old/
#cp MateBook.xcodeproj/qt_preprocess.mak MateBook.xcodeproj.old/
#
#rm -rf MateBook.xcodeproj
#
#cp -R MateBook.xcodeproj.old MateBook.xcodeproj
