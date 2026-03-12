@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64
set VCPKG_ROOT=C:\vcpkg
cd /d %~dp0
cmake --preset default && cmake --build build && python tools\gen_esp.py && python tools\gen_pex.py
