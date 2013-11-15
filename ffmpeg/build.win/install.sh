set -u
set -e
OLD_PATH="$PATH"

rm -rf "ffmpeg-$1-install-i686"
cd "ffmpeg-$1-build-i686"
export PATH="/projects/dikarchive/user/machacek/ffmpeg/build.win/mingw-w64-i686/bin:$OLD_PATH"
make install
cd "../ffmpeg-$1-install-i686"
rm -f ../../win/i386/lib/*
cp bin/*.lib ../../win/i386/lib/
rm -f ../../win/i386/bin/*
cp bin/*.dll ../../win/i386/bin/
cp bin/*.exe ../../win/i386/bin/
cd ..

rm -rf "ffmpeg-$1-install-x86_64"
cd "ffmpeg-$1-build-x86_64"
export PATH="/projects/dikarchive/user/machacek/ffmpeg/build.win/mingw-w64-x86_64/bin:$OLD_PATH"
make install
cd "../ffmpeg-$1-install-x86_64"
rm -f ../../win/x64/lib/*
cp bin/*.lib ../../win/x64/lib/
rm -f ../../win/x64/bin/*
cp bin/*.dll ../../win/x64/bin/
cp bin/*.exe ../../win/x64/bin/
cd ..
