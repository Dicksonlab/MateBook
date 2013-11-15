call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
cd ..\source
call bootstrap.bat
echo using msvc : 10.0 ; >> project-config.jam
b2.exe --build-type=complete --stagedir=../binaries/Win32 -j%NUMBER_OF_PROCESSORS%
