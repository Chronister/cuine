@echo off

:: call "%VC%\vcvarsall.bat" amd64

if NOT EXIST build mkdir build
pushd build
cl /nologo /ZI /TC ..\code\main.c /link /OUT:metachr.exe
:: cl /nologo /ZI ..\code\parser.c /link /OUT:parser.exe
popd
