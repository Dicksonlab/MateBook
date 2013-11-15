set -u
set -e

rm -rf MateBook.xcodeproj.old
mv MateBook.xcodeproj MateBook.xcodeproj.old

cat<<END_OF_HEAD>MateBook.pro
QT += core gui network xml opengl phonon
TEMPLATE = app
TARGET = MateBook
CONFIG += debug console
LIBS += -L"../../mediawrapper/binaries/OSX/Debug" -lmediawrapper
ICON = icon.icns
END_OF_HEAD

echo "HEADERS += \\">>MateBook.pro
echo "../../modeltest/source/modeltest.h \\">>MateBook.pro
echo "../../modeltest/source/dynamictreemodel.h \\">>MateBook.pro
echo "../../tracker/source/FrameAttributes.hpp \\">>MateBook.pro
echo "../../tracker/source/FlyAttributes.hpp \\">>MateBook.pro
echo "../../tracker/source/PairAttributes.hpp \\">>MateBook.pro
find ../source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../common/source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../common/source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../grapher/source -name "*.h" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../grapher/source -name "*.hpp" | sed -e 's/.*/& \\/'>>MateBook.pro
echo>>MateBook.pro

echo "SOURCES += \\">>MateBook.pro
echo "../../modeltest/source/modeltest.cpp \\">>MateBook.pro
echo "../../modeltest/source/dynamictreemodel.cpp \\">>MateBook.pro
echo "../../tracker/source/FrameAttributes.cpp \\">>MateBook.pro
echo "../../tracker/source/FlyAttributes.cpp \\">>MateBook.pro
echo "../../tracker/source/PairAttributes.cpp \\">>MateBook.pro
find ../source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../common/source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../common/source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../grapher/source -name "*.c" | sed -e 's/.*/& \\/'>>MateBook.pro
find ../../grapher/source -name "*.cpp" | sed -e 's/.*/& \\/'>>MateBook.pro
echo>>MateBook.pro

cat<<END_OF_TAIL>>MateBook.pro
RESOURCES += ../qt/matebook.qrc
END_OF_TAIL

qmake -spec macx-xcode MateBook.pro

cp MateBook.xcodeproj/qt_makeqmake.mak MateBook.xcodeproj.old/
cp MateBook.xcodeproj/qt_preprocess.mak MateBook.xcodeproj.old/

rm -rf MateBook.xcodeproj

cp -R MateBook.xcodeproj.old MateBook.xcodeproj
