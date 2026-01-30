@echo off

set CommonCompilerFlags=-MT -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4244 -wd4996 -wd4456 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref tmx_impl.obj raylib.lib user32.lib gdi32.lib winmm.lib shell32.lib kernel32.lib msvcrt.lib /NODEFAULTLIB:LIBCMT

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM delete pdb
del *.pdb > NUL 2> NUL

REM build tmx_impl.c
cl /c /TC ..\plata\code\tmx_impl.c 

REM echo Building s3mail.exe
cl %CommonCompilerFlags% ..\plata\code\win32_plata.cpp -Fmwin32_plata.map /link  %CommonLinkerFlags%

popd
