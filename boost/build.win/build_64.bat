call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64
cd ..\source
call bootstrap.bat
echo using msvc : 10.0 ; >> project-config.jam
b2.exe address-model=64 --build-type=complete --stagedir=../binaries/Win64 -j%NUMBER_OF_PROCESSORS%
