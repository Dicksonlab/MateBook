set -u
set -e
OLD_PATH="$PATH"
rm -rf "ffmpeg-$1"
tar xzf "../source/ffmpeg-$1.tar.gz"

rm -rf "ffmpeg-$1-build-i686"
mkdir "ffmpeg-$1-build-i686"
cd "ffmpeg-$1-build-i686"
export PATH="/projects/dikarchive/user/machacek/ffmpeg/build.win/mingw-w64-i686/bin:/projects/dikarchive/user/machacek/ffmpeg/build.win/lib.exe-intercept:$OLD_PATH"
../"ffmpeg-$1"/configure --enable-gpl --enable-version3 --enable-nonfree --prefix=../"ffmpeg-$1-install-i686" --enable-runtime-cpudetect --arch=x86 --target-os=mingw32 --cross-prefix=i686-w64-mingw32- --enable-w32threads --enable-postproc --enable-shared --disable-static --extra-cflags="-DSTRSAFE_NO_DEPRECATE" --disable-debug
make
cd ..

rm -rf "ffmpeg-$1-build-x86_64"
mkdir "ffmpeg-$1-build-x86_64"
cd "ffmpeg-$1-build-x86_64"
export PATH="/projects/dikarchive/user/machacek/ffmpeg/build.win/mingw-w64-x86_64/bin:/projects/dikarchive/user/machacek/ffmpeg/build.win/lib.exe-intercept:$OLD_PATH"
../"ffmpeg-$1"/configure --enable-gpl --enable-version3 --enable-nonfree --prefix=../"ffmpeg-$1-install-x86_64" --enable-runtime-cpudetect --arch=x86_64 --target-os=mingw32 --cross-prefix=x86_64-w64-mingw32- --enable-w32threads --enable-postproc --enable-shared --disable-static --extra-cflags="-DSTRSAFE_NO_DEPRECATE" --disable-debug
make
cd ..
