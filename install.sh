# this is not meant to be executed. rather,
# cut & paste depending on mac/pc/unix

# BUILD THE EXTERNAL DEPENDENCIES
deps/build.sh


# BUILD THE GUI
# on a mac:
cd gui
./update.sh
make

# on a mac, package it all into the .app bundle
# putting Qt into MateBook.app/ doesn't work, not sure why not

mkdir -p gui/Matebook.app/Contents/Frameworks

dylibs=(
  usr/lib/libmp3lame.0.dylib
  usr/lib/libpostproc.52.dylib
  usr/lib/libopencv_core.2.4.dylib
  usr/lib/libopencv_highgui.2.4.dylib
  usr/lib/libopencv_imgproc.2.4.dylib
  usr/lib/libboost_system.dylib
  usr/lib/libboost_filesystem.dylib
  usr/lib/libavdevice.55.dylib
  usr/lib/libavfilter.3.dylib
  usr/lib/libavformat.55.dylib
  usr/lib/libavutil.52.dylib
  usr/lib/libavcodec.55.dylib
  usr/lib/libswresample.0.dylib
  usr/lib/libswscale.2.dylib)
#  /usr/local/opt/qt/lib/phonon.framework
#  /usr/local/lib/QtGui.framework
#  /usr/local/lib/QtCore.framework
#  /usr/local/opt/qt/lib/QtXml.framework
#  /usr/local/opt/qt/lib/QtOpenGL.framework
#  /usr/local/lib/QtNetwork.framework
for x in ${dylibs[*]}; do
  cp -RL $x gui/Matebook.app/Contents/Frameworks
done
chmod -R u+w gui/Matebook.app/Contents/Frameworks

dylibs=(
  lib/libopencv_core.2.4.dylib
  lib/libopencv_highgui.2.4.dylib
  lib/libopencv_imgproc.2.4.dylib
  libboost_system.dylib
  libboost_filesystem.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavdevice.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavfilter.3.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavformat.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswresample.0.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswscale.2.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -id @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/$(basename $x)
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/MacOS/MateBook
done
install_name_tool -id @executable_path/../Frameworks/libmp3lame.0.dylib gui/MateBook.app/Contents/Frameworks/libmp3lame.0.dylib
install_name_tool -id @executable_path/../Frameworks/libpostproc.52.dylib gui/MateBook.app/Contents/Frameworks/libpostproc.52.dylib

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libpostproc.52.dylib
done

dylibs=(
  lib/libopencv_core.2.4.dylib
  lib/libopencv_imgproc.2.4.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavformat.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswscale.2.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libopencv_highgui.2.4.dylib
done

install_name_tool -change lib/libopencv_core.2.4.dylib @executable_path/../Frameworks/libopencv_core.2.4.dylib gui/MateBook.app/Contents/Frameworks/libopencv_imgproc.2.4.dylib

install_name_tool -change libboost_system.dylib @executable_path/../Frameworks/libboost_system.dylib gui/MateBook.app/Contents/Frameworks/libboost_filesystem.dylib

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libavcodec.55.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavfilter.3.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavformat.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libavdevice.55.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libswresample.0.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavformat.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libpostproc.52.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswscale.2.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libavfilter.3.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libavformat.55.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libavutil.52.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libswresample.0.dylib
done

dylibs=(
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr/lib/libmp3lame.0.dylib
  /Users/arthurb/src/MateBook/usr/lib/libz.1.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) gui/MateBook.app/Contents/Frameworks/libswscale.2.dylib
done

#dylibs=(
#  /usr/local/opt/qt/lib/phonon.framework/Versions/4/phonon
#  /usr/local/lib/QtGui.framework/Versions/4/QtGui
#  /usr/local/lib/QtCore.framework/Versions/4/QtCore
#  /usr/local/opt/qt/lib/QtXml.framework/Versions/4/QtXml
#  /usr/local/opt/qt/lib/QtOpenGL.framework/Versions/4/QtOpenGL
#  /usr/local/lib/QtNetwork.framework/Versions/4/QtNetwork)
#for x in ${dylibs[*]}; do
#  y=${x#${x%/*/*/*/*}}  # last four directories
#  install_name_tool -id @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/$y
#  install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/MacOS/MateBook
#done

#dylibs=(
#  /usr/local/Cellar/qt/4.8.5/lib/QtGui.framework/Versions/4/QtGui
#  /usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/QtCore)
#for x in ${dylibs[*]}; do
#  y=${x#${x%/*/*/*/*}}  # last four directories
#  install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/phonon.framework/Versions/4/phonon
#done

#x=/usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/QtCore
#y=${x#${x%/*/*/*/*}}  # last four directories
#install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui

#x=/usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/QtCore
#y=${x#${x%/*/*/*/*}}  # last four directories
#install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/QtXml.framework/Versions/4/QtXml

#dylibs=(
#  /usr/local/Cellar/qt/4.8.5/lib/QtGui.framework/Versions/4/QtGui
#  /usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/QtCore)
#for x in ${dylibs[*]}; do
#  y=${x#${x%/*/*/*/*}}  # last four directories
#  install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL
#done

#x=/usr/local/Cellar/qt/4.8.5/lib/QtCore.framework/Versions/4/QtCore
#y=${x#${x%/*/*/*/*}}  # last four directories
#install_name_tool -change $x @executable_path/../Frameworks/$y gui/MateBook.app/Contents/Frameworks/QtNetwork.framework/Versions/4/QtNetwork

#otool -L gui/MateBook.app/Contents/MacOS/MateBook


# BUILD THE TRACKER
cd tracker/build.gcc
make
cd ../..
#bindir=usr/bin/tracker/2141 # on the cluster
bindir=gui/MateBook.app/Contents/MacOS/ # on a mac
mkdir -p ${bindir}
cp tracker/build.gcc/tracker ${bindir}
cp tracker/source/*.sh ${bindir}

dylibs=(
  lib/libopencv_core.2.4.dylib
  lib/libopencv_highgui.2.4.dylib
  lib/libopencv_imgproc.2.4.dylib
  libboost_system.dylib
  libboost_filesystem.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavdevice.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavfilter.3.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavformat.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavutil.52.dylib
  /Users/arthurb/src/MateBook/usr//lib/libavcodec.55.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswresample.0.dylib
  /Users/arthurb/src/MateBook/usr//lib/libswscale.2.dylib)
for x in ${dylibs[*]}; do
  install_name_tool -change $x @executable_path/../Frameworks/$(basename $x) ${bindir}/tracker
done
